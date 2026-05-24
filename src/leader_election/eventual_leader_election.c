#include "leader_election/eventual_leader_election.h"
#include "failure_detector/eventually_perfect_failure_detector.h"
#include "utils/list.h"
#include "utils/logging.h"
#include "utils/parsing.h"
#include "utils/timeout.h"
#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <stdlib.h>

#define HEARTHBEAT_INTERVAL_SEC 4
#define DELIM_LEN 1

int HEARTBEAT_INCREMENT_SEC = 2;

typedef struct Heartbeat {
  FllDeliver base;
  int epoch;
} Heartbeat;

struct EventualLeaderElector {
  struct FairLossLink *fair_loss_link;
  list_t *candidates;
  void (*cb)(void *, Trust *);
  void *ctx;
  int leader;
  int max_rank;
  int local_rank;
  int local_epoch;
  int period_duration_secs;
};

static void wrapper(void *ctx, FllDeliver *e) {
  Ele *ele = ctx;
  char msg[MAX_MSG_LEN];
  int *sender = calloc(1, sizeof(int));

  if (try_parse_message(e->msg, "HE", msg, MAX_MSG_LEN) == 0) {
    int peer_epoch = atoi(msg);

    for (int i = 0; i < ele->candidates->count; i++) {
      Heartbeat *past_hb = list_get(ele->candidates, i);

      if (past_hb->base.sender == e->sender) {
        if (past_hb->epoch < peer_epoch) {
          list_remove(ele->candidates, i);
        }
        return;
      }
    }

    Heartbeat *heartbeat_copy = calloc(1, sizeof(Heartbeat));
    heartbeat_copy->base = *e;
    heartbeat_copy->epoch = peer_epoch;
    list_add(ele->candidates, heartbeat_copy);

  } else {
    printf("UNKNOW MESSAGE FROM %d: %s\n", *sender, e->msg);
    free(sender);
  }
}

static int send_heartbeat(struct FairLossLink *fll, int peer_rank, int epoch) {
  FllSend heartbeat = {.recipient = peer_rank};
  snprintf(heartbeat.msg, MAX_MSG_LEN, "HE,%d", epoch);

  return fll_send(fll, &heartbeat);
}

static void store_epoch(int rank, int epoch) {
  char path[256];
  snprintf(path, sizeof(path), "state/%d.epoch", rank);
  FILE *f = fopen(path, "w");
  if (!f)
    return;
  fprintf(f, "%d", epoch);
  fclose(f);
}

static int retrieve_epoch(int rank) {
  char path[256];
  snprintf(path, sizeof(path), "state/%d.epoch", rank);
  FILE *f = fopen(path, "r");
  if (!f)
    return -1;
  int epoch;
  if (fscanf(f, "%d", &epoch) != 1) {
    fclose(f);
    return -1;
  }
  fclose(f);
  return epoch;
}

Ele *ele_init(int local_rank, int max_rank, int base_port, int retransmission_period) {
  struct FairLossLink *fll = fll_init(local_rank, base_port);
  if (fll == NULL) {
    return NULL;
  }

  list_t *candidates = list_init();
  if (candidates == NULL) {
    fll_free(fll);
    return NULL;
  }

  Ele *ele = calloc(1, sizeof(Ele));
  if (ele == NULL) {
    fll_free(fll);
    list_free(candidates);
  }

  int epoch = retrieve_epoch(local_rank);
  if (epoch == -1) {
    printf("Couldn't retrieve epoch. Defaulting to 0\n");
    epoch = 0;
  }
  store_epoch(local_rank, ++epoch);

  fll_set_callback(fll, &wrapper, ele);
  ele->fair_loss_link = fll;
  ele->local_epoch = epoch;
  ele->candidates = candidates;
  ele->max_rank = max_rank;
  ele->local_rank = local_rank;
  ele->leader = max_rank;
  ele->period_duration_secs = HEARTHBEAT_INTERVAL_SEC;

  return ele;
}

void ele_set_on_new_trust(Ele *ele, void (*cb)(void *, Trust *), void *ctx) {
  ele->cb = cb;
  ele->ctx = ctx;
}

void ele_start(Ele *ele, struct timeval *external_timeout) {
  Trust leader = {.peer_rank = ele->leader};
  ele->cb(ele->ctx, &leader);

  struct timeval heartbeat_deadline, external_deadline;
  struct timeval heartbeat_timeout = {.tv_sec = ele->period_duration_secs,
                                      .tv_usec = 0};

  tv_reset_deadline(&heartbeat_deadline, &heartbeat_timeout);

  if (external_timeout) {
    gettimeofday(&external_deadline, NULL);
    timeradd(&external_deadline, external_timeout, &external_deadline);
  }

  for (int peer_rank = 1; peer_rank <= ele->max_rank; peer_rank++) {
    if (peer_rank == ele->local_rank)
      continue;

    send_heartbeat(ele->fair_loss_link, peer_rank, ele->local_epoch);
  }

  int done = 0;
  while (!done) {
    struct timeval *next_timeout = tv_min(&heartbeat_timeout, external_timeout);

    debug("Waiting for next timer\n");
    fll_consume(ele->fair_loss_link, next_timeout);

    struct timeval now;
    gettimeofday(&now, NULL);

    if (timercmp(&now, &heartbeat_deadline, >=)) {
      debug("Timer done\n");
      Heartbeat best_candidate = {.base = {.sender = ele->local_rank},
                                  .epoch = ele->local_epoch};

      debug("Local epoch: %d | Leader: %d\n", ele->local_epoch, ele->leader);
      for (int i = 0; i < ele->candidates->count; i++) {
        Heartbeat *last_hb = list_get(ele->candidates, i);
        debug("Peer %d Epoch %d\n", last_hb->base.sender, last_hb->epoch);
      }

      for (int i = 0; i < ele->candidates->count; i++) {
        Heartbeat *last_hb = list_get(ele->candidates, i);
        if (last_hb->epoch < best_candidate.epoch ||
            (last_hb->base.sender > best_candidate.base.sender &&
             last_hb->epoch == best_candidate.epoch)) {
          debug("New best candidate %d\n", last_hb->base.sender);
          best_candidate = *last_hb;

        } else {
          debug("Peer %d with epoch %d is not a better candidate\n",
                last_hb->base.sender, last_hb->epoch);
        }
      }

      if (best_candidate.base.sender != ele->leader) {
        debug("New leader\n");
        ele->leader = best_candidate.base.sender;
        ele->period_duration_secs += HEARTBEAT_INCREMENT_SEC;

        Trust trust_indication = {.peer_rank = ele->leader};
        ele->cb(ele->ctx, &trust_indication);
      }

      heartbeat_timeout.tv_sec = ele->period_duration_secs;
      heartbeat_timeout.tv_usec = 0;
      tv_reset_deadline(&heartbeat_deadline, &heartbeat_timeout);

      while (ele->candidates->count > 0)
        list_remove(ele->candidates, 0);

      for (int peer_rank = 1; peer_rank <= ele->max_rank; peer_rank++) {
        if (peer_rank == ele->local_rank)
          continue;

        send_heartbeat(ele->fair_loss_link, peer_rank, ele->local_epoch);
      }
    }

    done = external_timeout && timercmp(&now, &external_deadline, >=);
  }
}

void ele_free(Ele *mle) {
  fll_free(mle->fair_loss_link);
  list_free(mle->candidates);
}
