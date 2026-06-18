#ifndef LOGGED_PERFECT_LINK_H
#define LOGGED_PERFECT_LINK_H

#include "constants.h"
#include "link/stubborn_link.h"
#include "orchestration/task.h"
#include "utils/list.h"

typedef struct {
  int recipient;
  char msg[MAX_MSG_LEN];
} LplSend;

typedef struct {
  int sender;
  char msg[MAX_MSG_LEN];
  char id[UUID_STR_LEN];
} LplDeliver;

struct LoggedPerfectLink;

struct LoggedPerfectLink *lpl_init(int id, int base_port,
                                   int retransmission_period);

void get_deliveries(LplDeliver **deliveries, size_t *len);

int lpl_send(struct LoggedPerfectLink *lpl, LplSend *e);

void lpl_consume(struct LoggedPerfectLink *lpl, struct timeval *timeout);

void lpl_set_callback(struct LoggedPerfectLink *lpl,
                      void (*cb)(void *ctx, const list_t *deliveries),
                      void *ctx);

void lpl_free(struct LoggedPerfectLink *lpl);

void lpl_handle_timeout(struct LoggedPerfectLink *lpl);

wset_t *lpl_get_watch_set(struct LoggedPerfectLink *lpl);

handler_t *lpl_get_handler(struct LoggedPerfectLink *lpl);

task_t **lpl_get_tasks(struct LoggedPerfectLink *lpl, int *count);

#endif
