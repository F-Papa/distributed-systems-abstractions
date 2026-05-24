#include <bits/types/struct_timeval.h>
#include <sodium.h>

typedef struct ByzantineLeaderElector Ble;

typedef struct byzantine_trust {
  int peer_rank;
} ByzTrust;

typedef struct byzantine_complain {
  int round;
  int peer_rank;
} ByzComplain;

Ble *ble_init(int rank, int base_port, int retransmission_period, int max_faulty_peers,
              const unsigned char private_key[crypto_sign_SECRETKEYBYTES],
              int max_rank,
              const unsigned char public_keys[][crypto_sign_PUBLICKEYBYTES]);

void ble_set_on_trust_callback(Ble *ble, void (*cb)(void *, ByzTrust *),
                               void *ctx);

void ble_start(Ble *ble, struct timeval *timeout);

void ble_free(Ble *ble);
