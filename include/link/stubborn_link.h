#ifndef STUBBORN_LINK_H
#define STUBBORN_LINK_H

#include "link/common.h"
#include "orchestration/handler.h"
#include "orchestration/task.h"
#include "watch_set.h"
#include <sys/time.h>

struct StubbornLink;

struct StubbornLink *sbl_init(int id, int base_port, int retransmission_period);

int sbl_send(struct StubbornLink *sbl, Send *e);

void sbl_consume(struct StubbornLink *sbl, struct timeval *timeout);

void sbl_set_callback(struct StubbornLink *sbl, void (*cb)(void *, Deliver *),
                      void *ctx);

void sbl_free(struct StubbornLink *sbl);

void sbl_handle_timeout(struct StubbornLink *sbl);

wset_t *sbl_get_watch_set(struct StubbornLink *sbl);

handler_t *sbl_get_handler(struct StubbornLink *sbl);

task_t **sbl_get_tasks(struct StubbornLink *sbl, int *count);

#endif
