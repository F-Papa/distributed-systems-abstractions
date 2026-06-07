#include "link/perfect_link.h"
#include <bits/types/struct_timeval.h>

typedef struct best_effort_broadcast_send {
  char msg[MAX_MSG_LEN];
} BebSend;

typedef struct best_effort_broadcast_delivery {
  int sender;
  char msg[MAX_MSG_LEN];
} BebDelivery;

typedef struct BestEffortBroadcast Beb;

Beb *beb_init(int local_rank, int max_rank, int base_port,
              int retransmission_period);

int beb_broadcast(Beb *beb, BebSend *e);

void *beb_consume(Beb *beb, struct timeval *timeout);

void *beb_set_callback(Beb *beb, void (*cb)(void *, BebDelivery *), void *ctx);

void beb_free(Beb *beb);
