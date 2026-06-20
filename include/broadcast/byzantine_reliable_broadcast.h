#ifndef BYZANTINE_BROADCAST_H
#define BYZANTINE_BROADCAST_H

#include "broadcast/common.h"
#include "link/auth_perfect_link.h"
#include "link/common.h"
#include "orchestration/handler.h"
#include "orchestration/task.h"
#include "watch_set.h"

typedef struct ByzantineReliableBroadcast Brb;

typedef struct BrbConfig {
  int sender_rank;
  int max_faulty_nodes;
  AplConfig aplConfig;
} BrbConfig;

Brb *brb_init(BrbConfig config);

int brb_broadcast(Brb *brb, Broadcast *e);

void brb_set_callback(Brb *brb, void (*cb)(void *, Deliver *), void *ctx);

void brb_free(Brb *brb);

wset_t *brb_get_watch_set(Brb *brb);

handler_t *brb_get_handler(Brb *brb);

task_t **brb_get_tasks(Brb *brb, int *count);

#endif
