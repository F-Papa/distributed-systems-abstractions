#ifndef RELIABLE_BROADCAST_H
#define RELIABLE_BROADCAST_H

#include "constants.h"
#include "orchestration/handler.h"
#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <sys/select.h>

typedef struct reliable_broadcast_send {
  char msg[MAX_MSG_LEN];
} RbSend;

typedef struct reliable_broadcast_delivery {
  int sender;
  char msg[MAX_MSG_LEN];
} RbDelivery;

typedef struct ReliableBroadcast Rb;

typedef struct reliable_broadcast_config {
  int local_rank;
  int max_rank;
  int data_base_port;
  int control_base_port;
  int data_retransmission_period;
  int control_retransmission_period;
} RbConfig;

Rb *rb_init(RbConfig config);

int rb_broadcast(Rb *rb, RbSend *e);

void rb_set_callback(Rb *rb, void (*cb)(void *, RbDelivery *), void *ctx);

void rb_free(Rb *rb);

void rb_consume(Rb *rb, struct timeval *timeout);

void rb_handle_timeout(Rb *rb);

void rb_start(Rb *rb);

wset_t *rb_get_watch_set(Rb *rb);

handler_t *rb_get_handler(Rb *rb);

#endif
