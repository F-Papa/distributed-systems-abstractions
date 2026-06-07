#ifndef FAIR_LOSS_LINK_H
#define FAIR_LOSS_LINK_H

#include "constants.h"
#include <bits/types/struct_timeval.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct fair_loss_link_send {

  int recipient;
  char msg[MAX_MSG_LEN];
} FllSend;

typedef struct fair_loss_link_deliver {
  int sender;
  char msg[MAX_MSG_LEN];

} FllDeliver;

struct FairLossLink;

struct FairLossLink *fll_init(int id, int base_port);

int fll_send(struct FairLossLink *fll, FllSend *e);

void fll_set_callback(struct FairLossLink *fll,
                      void (*cb)(void *ctx, FllDeliver *e), void *ctx);

void fll_consume(struct FairLossLink *fll, struct timeval *timeout);

void fll_free(struct FairLossLink *fll);

#endif
