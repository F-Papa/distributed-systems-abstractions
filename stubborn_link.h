#include "fair_loss_link.h"
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

typedef FllSend SblSend;
typedef FllDeliver SblDeliver;

struct StubbornLink;

struct StubbornLink *sbl_init(int id, int retransmission_period);

int sbl_send(struct StubbornLink *sbl, SblSend *e);

void sbl_consume(struct StubbornLink *sbl, struct timeval *timeout);

void sbl_set_callback(struct StubbornLink *sbl,
                      void (*cb)(struct StubbornLink *, SblDeliver *));

void sbl_free(struct StubbornLink *sbl);
