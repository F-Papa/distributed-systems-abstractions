#include "fair_loss_link.h"
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define MSG_CAPACITY 1000

struct StubbornLink {
  int retransmission_period;
  struct FairLossLink *fair_loss_link;
  int n_outbox;
  FllSend outbox[MSG_CAPACITY];
};

typedef FllSend SblSend;
typedef FllDeliver SblDeliver;

struct StubbornLink *sbl_init(int id, int retransmission_period);

int sbl_send(struct StubbornLink *sbl, SblSend *e);
void sbl_consume(struct StubbornLink *sbl, struct timeval *timeout);
void sbl_set_callback(struct StubbornLink *sbl, void (*cb)(SblDeliver *e));
void sbl_free(struct StubbornLink *sbl);
