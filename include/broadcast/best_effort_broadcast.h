#ifndef BEST_EFFORT_BROADCAST_H
#define BEST_EFFORT_BROADCAST_H

#include "constants.h"
#include "orchestration/handler.h"
#include "orchestration/task.h"
#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <sys/select.h>

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

void beb_consume(Beb *beb, struct timeval *timeout);

void beb_set_callback(Beb *beb, void (*cb)(void *, BebDelivery *), void *ctx);

void beb_free(Beb *beb);

void beb_handle_timeout(Beb *beb);

wset_t *beb_get_watch_set(Beb *beb);

handler_t *beb_get_handler(Beb *beb);

task_t **beb_get_tasks(Beb *beb, int *count);

#endif
