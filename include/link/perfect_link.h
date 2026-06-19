#ifndef PERFECT_LINK_H
#define PERFECT_LINK_H

#include "link/common.h"
#include "orchestration/handler.h"
#include "orchestration/task.h"
#include "watch_set.h"

struct PerfectLink;

struct PerfectLink *pl_init(int id, int base_port, int retransmission_period);

int pl_send(struct PerfectLink *pl, Send *e);

void pl_consume(struct PerfectLink *pl, struct timeval *timeout);

void pl_set_callback(struct PerfectLink *pl, void (*cb)(void *, Deliver *e),
                     void *ctx);

void pl_free(struct PerfectLink *pl);

void pl_handle_timeout(struct PerfectLink *pl);

wset_t *pl_get_watch_set(struct PerfectLink *pl);

handler_t *pl_get_handler(struct PerfectLink *pl);

task_t **pl_get_tasks(struct PerfectLink *pl, int *count);

#endif
