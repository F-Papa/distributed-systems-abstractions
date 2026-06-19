#ifndef UNIFORM_RELIABLE_BROADCAST_H
#define UNIFORM_RELIABLE_BROADCAST_H

#include "broadcast/common.h"
#include "link/common.h"
#include "orchestration/handler.h"
#include "orchestration/task.h"
#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <sys/select.h>

typedef struct UniformReliableBroadcast Urb;

typedef struct UrbConfig {
  int local_rank;
  int max_rank;
  int base_port;
  int retransmission_period;
} UrbConfig;

Urb *urb_init(UrbConfig config);

int urb_broadcast(Urb *urb, Broadcast *e);

void urb_set_callback(Urb *urb, void (*cb)(void *, Deliver *), void *ctx);

void urb_free(Urb *urb);

wset_t *urb_get_watch_set(Urb *urb);

handler_t *urb_get_handler(Urb *urb);

task_t **urb_get_tasks(Urb *urb, int *count);

#endif
