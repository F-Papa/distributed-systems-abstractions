#include "perfect_link.h"
#include <stdio.h>
#include <stdlib.h>

struct CustomCtx {
  char *name;
};

void pl_process_msg(void *ctx, PlDeliver *e) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Received from %d: %s\n", cc->name, e->base.sender,
         e->base.msg);
}

void fll_process_msg(void *ctx, FllDeliver *e) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Received from %d: %s\n", cc->name, e->sender, e->msg);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);
  struct PerfectLink *pl = pl_init(id, 2);
  struct CustomCtx ctx = {.name = "Franco!"};
  pl_set_callback(pl, &pl_process_msg, &ctx);

  PlSend *e = calloc(1, sizeof(PlSend));
  e->base.recipient = id == 1 ? 2 : 1;
  snprintf(e->base.msg, MAX_MSG_LEN, "Hello from %d", id);

  pl_send(pl, e);

  struct timeval to = {.tv_sec = 10, .tv_usec = 0};
  pl_consume(pl, &to);
  pl_free(pl);
}
