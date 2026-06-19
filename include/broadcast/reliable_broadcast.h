#ifndef RELIABLE_BROADCAST_H
#define RELIABLE_BROADCAST_H

#include "broadcast/common.h"
#include "link/common.h"
#include "orchestration/handler.h"
#include "orchestration/task.h"
#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <sys/select.h>

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

int rb_broadcast(Rb *rb, Broadcast *e);

void rb_set_callback(Rb *rb, void (*cb)(void *, Deliver *), void *ctx);

void rb_free(Rb *rb);

void rb_consume(Rb *rb, struct timeval *timeout);

void rb_handle_timeout(Rb *rb);

void rb_start(Rb *rb);

wset_t *rb_get_watch_set(Rb *rb);

handler_t *rb_get_handler(Rb *rb);

task_t **rb_get_tasks(Rb *rb, int *count);

#endif
