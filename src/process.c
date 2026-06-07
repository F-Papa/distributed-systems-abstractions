#include "broadcast/best_effort_broadcast.h"
#include <stdio.h>
#include <stdlib.h>

#define BASE_PORT 3000

struct CustomCtx {
  char *name;
};

void on_delivery(void *ctx, BebDelivery *e) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Message from %d: %s\n", cc->name, e->sender, e->msg);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);
  int max_rank = atoi(argv[2]);

  Beb *beb = beb_init(id, max_rank, BASE_PORT, 2);
  struct CustomCtx ctx = {.name = "Franco!"};
  beb_set_callback(beb, &on_delivery, &ctx);

  struct timeval to = {.tv_sec = 45, .tv_usec = 0};
  BebSend msg = {.msg = "Hello everyone!"};
  beb_broadcast(beb, &msg);
  beb_consume(beb, &to);
  beb_free(beb);
}
