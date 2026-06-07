#ifndef RELIABLE_BROADCAST_H
#define RELIABLE_BROADCAST_H

#include "link/perfect_link.h"

typedef struct reliable_broadcast_send {
  int sender;
  char msg[MAX_MSG_LEN];
} RbSend;

typedef struct reliable_broadcast_delivery {
  int sender;
  char msg[MAX_MSG_LEN];
} RbDelivery;

typedef struct ReliableBroadcast Rb;

Rb *rb_init(int local_rank, int max_rank, int base_port,
            int retransmission_period);

int rb_broadcast(Rb *rb, RbSend *e);

void *rb_consume(Rb *rb, struct timeval *timeout);

void *rb_set_callback(Rb *rb, void (*cb)(void *, RbDelivery *), void *ctx);

Rb *rb_free(Rb *rb);

#endif
