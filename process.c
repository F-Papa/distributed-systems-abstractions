#include "stubborn_link.h"
#include <stdio.h>
#include <stdlib.h>

void process_msg(SblDeliver *e) {
  printf("Received from %d: %s\n", e->sender, e->msg);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);

  struct StubbornLink *sbl = sbl_init(id, 2);
  sbl_set_callback(sbl, &process_msg);

  SblSend *e = calloc(1, sizeof(SblSend));
  e->recipient = id == 1 ? 2 : 1;
  snprintf(e->msg, MAX_MSG_LEN, "Hello from %d", id);
  sbl_send(sbl, e);

  struct timeval to = {.tv_sec = 10, .tv_usec = 0};
  sbl_consume(sbl, &to);
  sbl_free(sbl);
}
