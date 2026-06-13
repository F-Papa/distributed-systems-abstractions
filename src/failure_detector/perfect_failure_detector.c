#include "failure_detector/perfect_failure_detector.h"
#include "constants.h"
#include "link/perfect_link.h"
#include "utils/list.h"
#include "utils/logging.h"
#include "utils/parsing.h"
#include "utils/timeout.h"
#include <bits/types/struct_timeval.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define HEALTHCHECK_INTERVAL_SEC 4

struct PerfectFailureDetector {
  struct PerfectLink *perfect_link;
  void (*on_crash_cb)(void *, Crash *);
  void *ctx;
  int max_rank;
  int local_rank;
  list_t *alive_peers;
  list_t *faulty_peers;
};

static int send_im_alive(struct PerfectLink *pl, int recipient) {
  PlSend im_alive = {.base.msg = "IA", .base.recipient = recipient};
  return pl_send(pl, &im_alive);
}

static int send_heartbeat(struct PerfectLink *pl, int recipient) {
  PlSend heartbeat = {.base.msg = "HB", .base.recipient = recipient};
  return pl_send(pl, &heartbeat);
}

void pfd_handle_timeout(Pfd *pfd) {
  debug("Timer done\n");
  for (size_t peer_rank = 1; peer_rank <= pfd->max_rank; peer_rank++) {
    if (peer_rank == pfd->local_rank)
      continue;

    int is_alive = 0, is_faulty = 0;
    for (int i = 0; i < pfd->alive_peers->count; i++) {
      int *j = list_get(pfd->alive_peers, i);
      if (*j == peer_rank) {
        is_alive = 1;
        break;
      }
    }

    for (int i = 0; i < pfd->faulty_peers->count; i++) {
      int *j = list_get(pfd->faulty_peers, i);
      if (*j == peer_rank) {
        is_faulty = 1;
        break;
      }
    }

    if (!is_alive && !is_faulty) {
      int *peer_rank_cpy = calloc(1, sizeof(int));
      *peer_rank_cpy = peer_rank;
      list_add(pfd->faulty_peers, peer_rank_cpy);

      Crash crash_indication = {.peer_id = peer_rank};
      pfd->on_crash_cb(pfd->ctx, &crash_indication);
    }

    send_heartbeat(pfd->perfect_link, peer_rank);
  }

  while (pfd->alive_peers->count > 0) {
    list_remove(pfd->alive_peers, 0);
  }
}

int pfd_register_fd_sets(struct PerfectFailureDetector *pfd, fd_set *reads,
                         fd_set *writes) {
  return pl_register_fd_sets(pfd->perfect_link, reads, writes);
}

void pfd_handle_fd_sets(struct PerfectFailureDetector *pfd, fd_set *reads,
                        fd_set *writes) {
  pl_handle_fd_sets(pfd->perfect_link, reads, writes);
}

static void pfd_callback(void *ctx, PlDeliver *e) {
  struct PerfectFailureDetector *pfd = ctx;
  char msg[MAX_MSG_LEN];
  int *sender = calloc(1, sizeof(int));
  *sender = e->base.sender;

  if (try_parse_message(e->base.msg, "HB", msg, MAX_MSG_LEN) == 0) {
    debug("HB from %d\n", *sender);
    send_im_alive(pfd->perfect_link, e->base.sender);

  } else if (try_parse_message(e->base.msg, "IA", msg, MAX_MSG_LEN) == 0) {
    debug("IA from %d\n", *sender);
    list_add(pfd->alive_peers, sender);

  } else {
    printf("UNKNOW MESSAGE FROM %d: %s\n", *sender, e->base.msg);
    free(sender);
  }
}

void pfd_start(struct PerfectFailureDetector *pfd,
               struct timeval *external_timeout) {
  struct timeval healthcheck_deadline, external_deadline;
  struct timeval healtheck_timeout = {.tv_sec = HEALTHCHECK_INTERVAL_SEC,
                                      .tv_usec = 0};

  tv_reset_deadline(&healthcheck_deadline, &healtheck_timeout);

  if (external_timeout) {
    gettimeofday(&external_deadline, NULL);
    timeradd(&external_deadline, external_timeout, &external_deadline);
  }

  for (int peer_rank = 1; peer_rank <= pfd->max_rank; peer_rank++) {
    if (peer_rank == pfd->local_rank)
      continue;

    send_heartbeat(pfd->perfect_link, peer_rank);
  }

  int done = 0;
  while (!done) {
    struct timeval *next_timeout = tv_min(&healtheck_timeout, external_timeout);
    debug("Waiting for next timer\n");
    pl_consume(pfd->perfect_link, next_timeout);

    struct timeval now;
    gettimeofday(&now, NULL);

    if (timercmp(&now, &healthcheck_deadline, >=)) {
      debug("Timer done\n");
      pfd_handle_timeout(pfd);
      healtheck_timeout.tv_sec = HEALTHCHECK_INTERVAL_SEC;
      healtheck_timeout.tv_usec = 0;
      tv_reset_deadline(&healthcheck_deadline, &healtheck_timeout);
    }

    done = external_timeout && timercmp(&now, &external_deadline, >=);
  }
}

Pfd *pfd_init(int local_rank, int max_rank, int base_port,
              int retransmission_period) {
  struct PerfectLink *pl =
      pl_init(local_rank, base_port, retransmission_period);
  if (pl == NULL) {
    return NULL;
  }

  list_t *alive_peers = list_init();
  if (alive_peers == NULL) {
    pl_free(pl);
    return NULL;
  }

  list_t *faulty_peers = list_init();
  if (faulty_peers == NULL) {
    pl_free(pl);
    list_free(alive_peers);
    return NULL;
  }

  Pfd *pfd = calloc(1, sizeof(Pfd) + max_rank * sizeof(int));
  if (pfd == NULL) {
    pl_free(pl);
    list_free(alive_peers);
    list_free(faulty_peers);
    return NULL;
  }

  pl_set_callback(pl, &pfd_callback, pfd);

  pfd->max_rank = max_rank;
  pfd->local_rank = local_rank;
  pfd->perfect_link = pl;
  pfd->alive_peers = alive_peers;
  pfd->faulty_peers = faulty_peers;

  for (int i = 1; i < max_rank; i++) {
    if (i == local_rank)
      continue;
    int *peer_rank = calloc(1, sizeof(int));
    list_add(pfd->alive_peers, peer_rank);
  }
  return pfd;
}

void pfd_set_oncrash(struct PerfectFailureDetector *pfd,
                     void (*cb)(void *ctx, Crash *e), void *ctx) {
  pfd->on_crash_cb = cb;
  pfd->ctx = ctx;
}

void pfd_free(struct PerfectFailureDetector *pfd) {
  pl_free(pfd->perfect_link);
  list_free(pfd->alive_peers);
  list_free(pfd->faulty_peers);
  free(pfd);
}

wset_t *pfd_get_watch_set(struct PerfectFailureDetector *pfd) {
  return pl_get_watch_set(pfd->perfect_link);
}
