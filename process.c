#include "logged_perfect_link.h"
#include <stdio.h>
#include <stdlib.h>

void process_deliveries(LplDeliver *deliveries, size_t len) {
  printf("Indication received\n");
  printf("(%ld) Deliveries:\n", len);
  for (size_t i = 0; i < len; i++)
    printf("[%d / %ld]: %s\n", deliveries[i].base.sender, deliveries[i].id,
           deliveries[i].base.msg);
  printf("\n");
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);

  struct LoggedPerfectLink *pl = lpl_init(id, 2);
  lpl_set_callback(pl, &process_deliveries);

  int recipient = id == 1 ? 2 : 1;
  LplSend e = {.base.recipient = recipient};
  snprintf(e.base.msg, MAX_MSG_LEN, "Hello from %d", id);
  lpl_send(pl, &e);

  snprintf(e.base.msg, MAX_MSG_LEN, "Hello again! from %d", id);
  lpl_send(pl, &e);

  struct timeval to = {.tv_sec = 10, .tv_usec = 0};
  lpl_consume(pl, &to);
  lpl_free(pl);
}
