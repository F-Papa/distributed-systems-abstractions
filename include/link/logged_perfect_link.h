#ifndef LOGGED_PERFECT_LINK_H
#define LOGGED_PERFECT_LINK_H

#include "constants.h"
#include "link/stubborn_link.h"
#include "utils/list.h"

typedef struct {
  SblSend base;
  char id[UUID_STR_LEN];
} LplSend;

typedef struct {
  SblDeliver base;
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

int lpl_register_fd_sets(struct LoggedPerfectLink *lpl, fd_set *reads,
                         fd_set *writes);

void lpl_handle_fd_sets(struct LoggedPerfectLink *lpl, fd_set *reads,
                        fd_set *writes);

void lpl_handle_timeout(struct LoggedPerfectLink *lpl);

wset_t *lpl_get_watch_set(struct LoggedPerfectLink *lpl);

handler_t *lpl_get_handler(struct LoggedPerfectLink *lpl);

#endif
