#include "leader_election/monarchical_leader_election.h"
#include "failure_detector/perfect_failure_detector.h"
#include "orchestration/handler.h"
#include "utils/list.h"
#include "utils/logging.h"
#include <bits/types/struct_timeval.h>
#include <stdlib.h>

struct MonarchicalLeaderElector {
  Pfd *perfect_failure_detector;
  list_t *crashed_peers;
  void (*cb)(void *, Leader *);
  void *ctx;
  int leader;
  int max_rank;
};

void mle_set_on_new_leader(Mle *mle, void (*cb)(void *, Leader *), void *ctx) {
  mle->cb = cb;
  mle->ctx = ctx;
}

static void mle_callback_on_crash(void *ctx, Crash *e) {
  Mle *mle = ctx;
  int *new_crashed_peer_rank = calloc(1, sizeof(int));
  *new_crashed_peer_rank = e->peer_id;
  list_add(mle->crashed_peers, new_crashed_peer_rank);

  if (*new_crashed_peer_rank != mle->leader) {
    debug("%d Crashed (leader is: %d)\n", *new_crashed_peer_rank, mle->leader);
    return;
  } else {
    debug("Leader crashed (%d)\n", mle->leader);
  }

  int candidate;
  debug("Electing new candidate\n");

  for (candidate = mle->max_rank; candidate > 0; candidate--) {
    int has_crashed = 0;

    for (int j = 0; j < mle->crashed_peers->count; j++) {
      int *crashed_peer_rank = list_get(mle->crashed_peers, j);

      if (*crashed_peer_rank == candidate) {
        has_crashed = 1;
        debug("%d is not a valid candidate\n", candidate);
        break;
      }
    }

    if (!has_crashed) {
      debug("%d is the best candidate\n");
      break;
    }
  }

  mle->leader = candidate;
  Leader leader = {.peer_rank = candidate};
  mle->cb(mle->ctx, &leader);
}

Mle *mle_init(int local_rank, int max_rank, int base_port,
              int retransmission_period) {
  Pfd *pfd = pfd_init(local_rank, max_rank, base_port, retransmission_period);
  if (pfd == NULL) {
    return NULL;
  }

  list_t *crashed_peers = list_init();
  if (crashed_peers == NULL) {
    pfd_free(pfd);
    return NULL;
  }

  Mle *mle = calloc(1, sizeof(Mle));
  if (mle == NULL) {
    pfd_free(pfd);
    list_free(crashed_peers);
  }

  pfd_set_oncrash(pfd, &mle_callback_on_crash, mle);
  mle->perfect_failure_detector = pfd;
  mle->crashed_peers = crashed_peers;
  mle->max_rank = max_rank;
  mle->leader = max_rank;

  return mle;
}

void mle_start(Mle *mle, struct timeval *timeout) {
  Leader leader = {.peer_rank = mle->leader};
  mle->cb(mle->ctx, &leader);
  pfd_start(mle->perfect_failure_detector, timeout);
}

void mle_handle_timeout(Mle *mle) {
  pfd_handle_timeout(mle->perfect_failure_detector);
}

void mle_free(Mle *mle) {
  pfd_free(mle->perfect_failure_detector);
  list_free(mle->crashed_peers);
}

wset_t *mle_get_watch_set(Mle *mle) {
  return pfd_get_watch_set(mle->perfect_failure_detector);
}

handler_t *mle_get_handler(Mle *mle) {
  return pfd_get_handler(mle->perfect_failure_detector);
}

task_t **mle_get_tasks(Mle *mle, int *count) {
  return pfd_get_tasks(mle->perfect_failure_detector, count);
}
