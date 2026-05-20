#include "stubborn_link.h"

typedef struct {
  SblSend base;
  long id;
} PlSend;

typedef struct {
  SblDeliver base;
  long id;
} PlDeliver;

struct PerfectLink {
  struct StubbornLink *stubborn_link;
};

struct PerfectLink *pl_init(int id, int retransmission_period);

int pl_send(struct PerfectLink *pl, PlSend *e);
void pl_consume(struct PerfectLink *pl, struct timeval *timeout);
void pl_set_callback(struct PerfectLink *pl, void (*cb)(PlDeliver *e));
void pl_free(struct PerfectLink *pl);
