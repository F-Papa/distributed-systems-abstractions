#include "failure_detector/eventually_perfect_failure_detector.h"
#include "constants.h"
#include "link/perfect_link.h"
#include "utils/list.h"
#include "utils/parsing.h"
#include "utils/timeout.h"
#include <bits/types/struct_timeval.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define HEALTHCHECK_INTERVAL_SEC 4

typedef struct {
  int peer_rank;
} Crash;

struct EventuallyPerfectFailureDetector {
  struct PerfectLink *perfect_link;
  void (*on_suspect_cb)(void *, Suspect *);
  void *on_suspect_ctx;
  void (*on_restore_cb)(void *, Restore *);
  void *on_restore_ctx;
  int max_rank;
  int local_rank;
  list_t *alive_peers;
  list_t *suspected_peers;
};

static int send_im_alive(struct PerfectLink *pl, int recipient) {
  PlSend im_alive = {.base.msg = "IA", .base.recipient = recipient};
  return pl_send(pl, &im_alive);
}

static int send_heartbeat(struct PerfectLink *pl, int recipient) {
  PlSend im_alive = {.base.msg = "HB", .base.recipient = recipient};
  return pl_send(pl, &im_alive);
}

static void pfd_callback(void *ctx, PlDeliver *e) {
  Epfd *pfd = ctx;
  char msg[MAX_MSG_LEN];
  int *sender = calloc(1, sizeof(int));
  *sender = e->base.sender;
  if (try_parse_message(e->base.msg, "HB", msg, MAX_MSG_LEN) == 0) {
    send_im_alive(pfd->perfect_link, *sender);

  } else if (try_parse_message(e->base.msg, "IA", msg, MAX_MSG_LEN) == 0) {
    list_add(pfd->alive_peers, sender);
  } else {
    printf("UNKNOW MESSAGE FROM %d: %s\n", *sender, e->base.msg);
    free(sender);
  }
}

int epfd_register_fd_sets(Epfd *epfd, fd_set *reads, fd_set *writes) {
  return pl_register_fd_sets(epfd->perfect_link, reads, writes);
}

void epfd_handle_fd_sets(Epfd *epfd, fd_set *reads, fd_set *writes) {
  pl_handle_fd_sets(epfd->perfect_link, reads, writes);
}

void epfd_handle_timeout(Epfd *epfd) {
  for (size_t peer_rank = 1; peer_rank <= epfd->max_rank; peer_rank++) {
    if (peer_rank == epfd->local_rank)
      continue;

    int is_alive = 0, is_faulty = 0;
    for (int i = 0; i < epfd->alive_peers->count; i++) {
      int *j = list_get(epfd->alive_peers, i);
      if (*j == peer_rank) {
        is_alive = 1;
        break;
      }
    }
    for (int i = 0; i < epfd->suspected_peers->count; i++) {
      int *j = list_get(epfd->suspected_peers, i);
      if (*j == peer_rank) {
        is_faulty = 1;
        break;
      }
    }

    if (!is_alive && !is_faulty) {
      int *peer_rank_cpy = calloc(1, sizeof(int));
      *peer_rank_cpy = peer_rank;
      list_add(epfd->suspected_peers, peer_rank_cpy);

      Suspect suspect_indication = {.peer_rank = peer_rank};
      epfd->on_suspect_cb(epfd->on_suspect_ctx, &suspect_indication);

    } else if (is_alive && is_faulty) {
      for (int i = 0; i < epfd->suspected_peers->count; i++) {
        int *j = list_get(epfd->suspected_peers, i);
        if (*j == peer_rank) {
          list_remove(epfd->suspected_peers, i);
          break;
        }
      }

      Restore restore_indication = {.peer_rank = peer_rank};
      epfd->on_restore_cb(epfd->on_restore_ctx, &restore_indication);
    }

    send_heartbeat(epfd->perfect_link, peer_rank);
  }

  while (epfd->alive_peers->count > 0) {
    list_remove(epfd->alive_peers, 0);
  }
}

void epfd_start(Epfd *epfd, struct timeval *external_timeout) {
  struct timeval healthcheck_deadline, external_deadline;
  struct timeval healtheck_timeout = {.tv_sec = HEALTHCHECK_INTERVAL_SEC,
                                      .tv_usec = 0};

  tv_reset_deadline(&healthcheck_deadline, &healtheck_timeout);

  if (external_timeout) {
    gettimeofday(&external_deadline, NULL);
    timeradd(&external_deadline, external_timeout, &external_deadline);
  }

  for (int peer_rank = 0; peer_rank <= epfd->max_rank; peer_rank++) {
    if (peer_rank == epfd->local_rank)
      continue;

    send_heartbeat(epfd->perfect_link, peer_rank);
  }

  int done = 0;
  while (!done) {
    struct timeval *next_timeout = tv_min(&healtheck_timeout, external_timeout);

    pl_consume(epfd->perfect_link, next_timeout);

    struct timeval now;
    gettimeofday(&now, NULL);

    if (timercmp(&now, &healthcheck_deadline, >=)) {
      epfd_handle_timeout(epfd);
      healtheck_timeout.tv_sec = HEALTHCHECK_INTERVAL_SEC;
      healtheck_timeout.tv_usec = 0;
      tv_reset_deadline(&healthcheck_deadline, &healtheck_timeout);
    }

    done = external_timeout && timercmp(&now, &external_deadline, >=);
  }
}

Epfd *epfd_init(int local_rank, int max_rank, int base_port,
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

  list_t *suspected_peers = list_init();
  if (suspected_peers == NULL) {
    pl_free(pl);
    list_free(alive_peers);
    return NULL;
  }

  Epfd *epfd = calloc(1, sizeof(Epfd) + max_rank * sizeof(int));
  if (epfd == NULL) {
    pl_free(pl);
    list_free(alive_peers);
    list_free(suspected_peers);
    return NULL;
  }

  pl_set_callback(pl, &pfd_callback, epfd);

  epfd->max_rank = max_rank;
  epfd->local_rank = local_rank;
  epfd->perfect_link = pl;
  epfd->alive_peers = alive_peers;
  epfd->suspected_peers = suspected_peers;

  for (int i = 1; i < max_rank; i++) {
    if (i == local_rank)
      continue;
    int *peer_rank = calloc(1, sizeof(int));
    list_add(epfd->alive_peers, peer_rank);
  }
  return epfd;
}

void epfd_set_on_suspect(Epfd *epfd, void (*cb)(void *ctx, Suspect *e),
                         void *ctx) {
  epfd->on_suspect_cb = cb;
  epfd->on_suspect_ctx = ctx;
}

void epfd_set_on_restore(Epfd *epfd, void (*cb)(void *ctx, Restore *e),
                         void *ctx) {
  epfd->on_restore_cb = cb;
  epfd->on_restore_ctx = ctx;
}

void epfd_free(Epfd *epfd) {
  pl_free(epfd->perfect_link);
  list_free(epfd->alive_peers);
  list_free(epfd->suspected_peers);
  free(epfd);
}

wset_t *epfd_get_watch_set(Epfd *epfd) {
  return pl_get_watch_set(epfd->perfect_link);
}
