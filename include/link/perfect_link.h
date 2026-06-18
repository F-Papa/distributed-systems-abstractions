#ifndef PERFECT_LINK_H
#define PERFECT_LINK_H

#include "constants.h"
#include "link/stubborn_link.h"
#include "orchestration/task.h"

typedef struct {
  int recipient;
  char msg[MAX_MSG_LEN];
  char id[UUID_STR_LEN];
} PlSend;

typedef struct {
  int sender;
  char msg[MAX_MSG_LEN];
  char id[UUID_STR_LEN];
} PlDeliver;

struct PerfectLink;

struct PerfectLink *pl_init(int id, int base_port, int retransmission_period);

int pl_send(struct PerfectLink *pl, PlSend *e);

void pl_consume(struct PerfectLink *pl, struct timeval *timeout);

void pl_set_callback(struct PerfectLink *pl, void (*cb)(void *, PlDeliver *e),
                     void *ctx);

void pl_free(struct PerfectLink *pl);

void pl_handle_timeout(struct PerfectLink *pl);

wset_t *pl_get_watch_set(struct PerfectLink *pl);

handler_t *pl_get_handler(struct PerfectLink *pl);

task_t **pl_get_tasks(struct PerfectLink *pl, int *count);

#endif
