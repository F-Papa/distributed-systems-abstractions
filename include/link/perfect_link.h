#ifndef PERFECT_LINK_H
#define PERFECT_LINK_H

#include "constants.h"
#include "link/stubborn_link.h"

typedef struct {
  SblSend base;
  char id[UUID_STR_LEN];
} PlSend;

typedef struct {
  SblDeliver base;
  char id[UUID_STR_LEN];
} PlDeliver;

struct PerfectLink;

struct PerfectLink *pl_init(int id, int base_port, int retransmission_period);

int pl_send(struct PerfectLink *pl, PlSend *e);

void pl_consume(struct PerfectLink *pl, struct timeval *timeout);

void pl_set_callback(struct PerfectLink *pl, void (*cb)(void *, PlDeliver *e),
                     void *ctx);

void pl_free(struct PerfectLink *pl);

int pl_register_fd_sets(struct PerfectLink *pl, fd_set *reads, fd_set *writes);

void pl_handle_fd_sets(struct PerfectLink *pl, fd_set *reads, fd_set *writes);

#endif
