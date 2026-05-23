#include "logged_perfect_link.h"
#include <stdio.h>
#include <stdlib.h>

struct CustomCtx {
  char *name;
};

void pl_process_msg(void *ctx, const list_t *deliveries) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Received indication:\n", cc->name);
  for (size_t i = 0; i < deliveries->count; i++) {
    LplDeliver *d = list_get(deliveries, i);
    printf("Delivery [%lu]: %s\n", i, d->base.msg);
  }
}

void fll_process_msg(void *ctx, FllDeliver *e) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Received from %d: %s\n", cc->name, e->sender, e->msg);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);
  struct LoggedPerfectLink *lpl = lpl_init(id, 2);
  struct CustomCtx ctx = {.name = "Franco!"};
  lpl_set_callback(lpl, &pl_process_msg, &ctx);

  LplSend *e = calloc(1, sizeof(LplSend));
  e->base.recipient = id == 1 ? 2 : 1;
  snprintf(e->base.msg, MAX_MSG_LEN, "Hello from %d", id);

  lpl_send(lpl, e);

  snprintf(e->base.msg, MAX_MSG_LEN, "Hello again from %d", id);
  lpl_send(lpl, e);

  struct timeval to = {.tv_sec = 10, .tv_usec = 0};
  lpl_consume(lpl, &to);
  lpl_free(lpl);
}
