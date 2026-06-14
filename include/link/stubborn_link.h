#ifndef STUBBORN_LINK_H
#define STUBBORN_LINK_H

#include "link/fair_loss_link.h"
#include <sys/time.h>

typedef FllSend SblSend;
typedef FllDeliver SblDeliver;

struct StubbornLink;

struct StubbornLink *sbl_init(int id, int base_port, int retransmission_period);

int sbl_send(struct StubbornLink *sbl, SblSend *e);

void sbl_consume(struct StubbornLink *sbl, struct timeval *timeout);

void sbl_set_callback(struct StubbornLink *sbl,
                      void (*cb)(void *, SblDeliver *), void *ctx);

void sbl_free(struct StubbornLink *sbl);

void sbl_handle_timeout(struct StubbornLink *sbl);

wset_t *sbl_get_watch_set(struct StubbornLink *sbl);

handler_t *sbl_get_handler(struct StubbornLink *sbl);

#endif
