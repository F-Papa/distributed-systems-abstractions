#ifndef BYZANTINE_LEADER_ELECTION_H
#define BYZANTINE_LEADER_ELECTION_H

#include "orchestration/handler.h"
#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <sodium.h>
#include <sys/select.h>

typedef struct ByzantineLeaderElector Ble;

typedef struct byzantine_trust {
  int peer_rank;
} ByzTrust;

typedef struct byzantine_complain {
  int round;
  int peer_rank;
} ByzComplain;

Ble *ble_init(int rank, int base_port, int retransmission_period,
              int max_faulty_peers,
              const unsigned char private_key[crypto_sign_SECRETKEYBYTES],
              int max_rank,
              const unsigned char public_keys[][crypto_sign_PUBLICKEYBYTES]);

void ble_set_on_trust_callback(Ble *ble, void (*cb)(void *, ByzTrust *),
                               void *ctx);

void ble_start(Ble *ble, struct timeval *timeout);

void ble_free(Ble *ble);

int ble_register_fd_sets(Ble *ble, fd_set *reads, fd_set *writes);

void ble_handle_fd_sets(Ble *ble, fd_set *reads, fd_set *writes);

void ble_handle_timeout(Ble *ble);

wset_t *ble_get_watch_set(Ble *ble);

handler_t *ble_get_handler(Ble *ble);

#endif
