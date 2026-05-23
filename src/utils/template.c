#include "leader_election/eventual_leader_election.h"
#include "failure_detector/eventually_perfect_failure_detector.h"
#include "utils/list.h"
#include "utils/logging.h"
#include <bits/types/struct_timeval.h>
#include <stdlib.h>

#define HEARTHBEAT_INTERVAL_SEC 4
#define DELIM_LEN 1

struct EventualLeaderElector {
  struct FairLossLink *fair_loss_link;
  list_t *crashed_peers;
  void (*cb)(void *, Trust *);
  void *ctx;
  int leader;
  int max_rank;
  int local_rank;
};

static void wrapper(void *ctx, FllDeliver *e) {}
static void send_heartbeat(struct FairLossLink *fll, int peer_rank) {}

Ele *ele_init(int local_rank, int max_rank, int retransmission_period) {
  struct FairLossLink *fll = fll_init(local_rank);
  if (fll == NULL) {
    return NULL;
  }

  list_t *crashed_peers = list_init();
  if (crashed_peers == NULL) {
    fll_free(fll);
    return NULL;
  }

  Ele *ele = calloc(1, sizeof(Ele));
  if (ele == NULL) {
    fll_free(fll);
    list_free(crashed_peers);
  }

  fll_set_callback(fll, &wrapper, ele);
  ele->fair_loss_link = fll;
  ele->crashed_peers = crashed_peers;
  ele->max_rank = max_rank;
  ele->local_rank = local_rank;
  ele->leader = max_rank;

  return ele;
}

void ele_start(Ele *ele, struct timeval *external_timeout) {
  Trust leader = {.peer_rank = ele->leader};
  ele->cb(ele->ctx, &leader);

  struct timeval heartbeat_deadline, external_deadline;
  struct timeval heartbeat_timeout = {.tv_sec = HEARTHBEAT_INTERVAL_SEC,
                                      .tv_usec = 0};

  gettimeofday(&heartbeat_deadline, NULL);
  timeradd(&heartbeat_deadline, &heartbeat_timeout, &heartbeat_deadline);

  if (external_timeout) {
    gettimeofday(&external_deadline, NULL);
    timeradd(&external_deadline, external_timeout, &external_deadline);
  }

  for (int peer_rank = 1; peer_rank <= ele->max_rank; peer_rank++) {
    if (peer_rank == ele->local_rank)
      continue;

    send_heartbeat(ele->fair_loss_link, peer_rank);
  }

  int done = 0;
  while (!done) {
    struct timeval *next_timeout;
    if (!external_timeout ||
        timercmp(&heartbeat_timeout, external_timeout, <)) {
      next_timeout = &heartbeat_timeout;
    } else {
      next_timeout = external_timeout;
    }
    debug("Waiting for next timer\n");
    fll_consume(ele->fair_loss_link, next_timeout);
    struct timeval now;
    gettimeofday(&now, NULL);
    if (timercmp(&now, &heartbeat_deadline, >=)) {
      debug("Timer done\n");
      heartbeat_timeout.tv_sec = HEARTHBEAT_INTERVAL_SEC;
      heartbeat_timeout.tv_usec = 0;
      gettimeofday(&heartbeat_deadline, NULL);
      timeradd(&heartbeat_deadline, &heartbeat_timeout, &heartbeat_deadline);
    }
  }
}

void ele_free(Ele *mle) {
  fll_free(mle->fair_loss_link);
  list_free(mle->crashed_peers);
}
