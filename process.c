#include "perfect_link.h"
#include <stdio.h>
#include <stdlib.h>

void process_msg(struct PerfectLink *pl, PlDeliver *e) {
  printf("[%p] Received from %d: %s\n", pl, e->base.sender, e->base.msg);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);

  struct PerfectLink *pl = pl_init(id, 2);
  pl_set_callback(pl, &process_msg);

  PlSend *e = calloc(1, sizeof(PlSend));
  e->base.recipient = id == 1 ? 2 : 1;
  snprintf(e->base.msg, MAX_MSG_LEN, "Hello from %d", id);

  pl_send(pl, e);

  struct timeval to = {.tv_sec = 10, .tv_usec = 0};
  pl_consume(pl, &to);
  pl_free(pl);
}
