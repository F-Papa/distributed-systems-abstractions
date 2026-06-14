#include "leader_election/byzantine_leader_election.h"
#include "link/auth_perfect_link.h"
#include "utils/list.h"
#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/logging.h>
#include <utils/parsing.h>

typedef struct ByzantineLeaderElector {
  Apl *auth_perfect_link;
  list_t *complains;
  int round;
  int complained;
  int max_rank;
  int local_rank;
  int max_faulty_peers;
  void (*cb)(void *, ByzTrust *);
  void *ctx;
} Ble;

int get_round_leader(Ble *ble) { return ble->round % ble->max_rank + 1; }

void ble_complain(Ble *ble) {
  AuthPlSend r = {};
  snprintf(r.base.base.msg, MAX_MSG_LEN, "CO,%d", ble->round);

  for (int i = 1; i <= ble->max_rank; i++) {
    if (i == ble->local_rank)
      continue;
    r.base.base.recipient = i;
    apl_send(ble->auth_perfect_link, &r);
  }

  ble->complained = 1;
}

void ble_set_on_trust_callback(Ble *ble, void (*cb)(void *, ByzTrust *),
                               void *ctx) {
  ble->cb = cb;
  ble->ctx = ctx;
}

static void ble_callback(void *ctx, AuthPlDeliver *deliver) {
  Ble *ble = ctx;
  char body[MAX_MSG_LEN];
  if (try_parse_message(deliver->base.base.msg, "CO", body, MAX_MSG_LEN) == 0) {
    int round = atoi(body);

    if (round != ble->round) {
      debug("Received complain from %d for wrong round (%d)\n",
            deliver->base.base.sender, round);
      return;
    }

    for (int i = 0; i < ble->complains->count; i++) {
      ByzComplain *past_complain = list_get(ble->complains, i);
      if (past_complain->peer_rank == deliver->base.base.sender) {
        debug("Duplicated complain from %d\n", deliver->base.base.sender,
              round);
        return;
      }
    }
    ByzComplain *new_complain = calloc(1, sizeof(ByzComplain));
    if (new_complain == NULL)
      return;

    new_complain->peer_rank = deliver->base.base.sender;
    new_complain->round = round;
    list_add(ble->complains, new_complain);

    if (ble->complains->count > ble->max_faulty_peers && !ble->complained) {
      ble_complain(ble);
    }

    if (ble->complains->count > 2 * ble->max_faulty_peers) {
      ble->round++;
      ble->complained = 0;
      while (ble->complains->count > 0) {
        list_remove(ble->complains, 0);
      }

      ByzTrust i = {.peer_rank = get_round_leader(ble)};
      ble->cb(ble->ctx, &i);
    }

  } else {
    printf("UNKNOWN MESSAGE FROM %d: %s\n", deliver->base.base.sender,
           deliver->base.base.msg);
  }
}

void ble_handle_timeout(Ble *ble) {
  apl_handle_timeout(ble->auth_perfect_link);
}

void ble_free(Ble *ble) {
  list_free(ble->complains);
  apl_free(ble->auth_perfect_link);
  free(ble);
}

void ble_start(Ble *ble, struct timeval *timeout) {
  ByzTrust i = {.peer_rank = get_round_leader(ble)};
  ble->cb(ble->ctx, &i);
  apl_consume(ble->auth_perfect_link, timeout);
}

Ble *ble_init(int rank, int base_port, int retransmission_period,
              int max_faulty_peers,
              const unsigned char private_key[crypto_sign_SECRETKEYBYTES],
              int max_rank,
              const unsigned char public_keys[][crypto_sign_PUBLICKEYBYTES]) {

  if ((float)max_rank / (float)3 <= (float)max_faulty_peers) {
    fprintf(stderr,
            "Byzantine faults of upto %d peers cannot be tolerated when total "
            "is %d.\n",
            max_faulty_peers, max_rank);
    return NULL;
  }

  Apl *apl = apl_init(rank, base_port, retransmission_period, private_key,
                      max_rank, public_keys);

  if (apl == NULL) {
    return NULL;
  }

  list_t *complains = list_init();
  if (complains == NULL) {
    apl_free(apl);
    return NULL;
  }

  Ble *ble = calloc(1, sizeof(Ble));
  if (ble == NULL) {
    apl_free(apl);
    list_free(complains);
    return NULL;
  }

  ble->complains = complains;
  ble->auth_perfect_link = apl;
  ble->local_rank = rank;
  ble->max_rank = max_rank;
  ble->max_faulty_peers = max_faulty_peers;
  apl_set_callback(apl, ble_callback, ble);
  return ble;
}

wset_t *ble_get_watch_set(Ble *ble) {
  return apl_get_watch_set(ble->auth_perfect_link);
}

handler_t *ble_get_handler(Ble *ble) {
  return apl_get_handler(ble->auth_perfect_link);
}
