#include "fair_loss_link.h"
#include <stdio.h>
#include <stdlib.h>

void process_msg(FllDeliver *e) {
  printf("Received from %d: %s\n", e->sender, e->msg);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);

  struct FairLossLink *fll = fll_init(id);
  fll_set_callback(fll, &process_msg);

  FllSend *e = calloc(1, sizeof(FllSend));
  e->recipient = id == 1 ? 2 : 1;
  snprintf(e->msg, MAX_MSG_LEN, "Hello from %d", id);
  fll_send(fll, e);

  struct timeval to = {.tv_sec = 3, .tv_usec = 0};
  fll_consume(fll, NULL);
}
