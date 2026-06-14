#include "link/stubborn_link.h"
#include "syscall.h"
#include <bits/types/struct_timeval.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define BASE_PORT 3000

void callback(void *ctx, SblDeliver *delivery) {
  printf("%s\n", delivery->msg);
}

int main(int argc, char **argv) {
  printf("Initializing...\n");
  int local_rank = atoi(argv[1]);
  int max_rank = atoi(argv[2]);
  printf("Local Rank: %d, Max Rank: %d\n", local_rank, max_rank);

  struct StubbornLink *stubborn_link = sbl_init(local_rank, BASE_PORT, 3);
  SblSend s = {.recipient = local_rank == 1 ? 2 : 1, .msg = "Hello my friend!"};
  sbl_set_callback(stubborn_link, &callback, NULL);
  sbl_send(stubborn_link, &s);

  struct timeval timeout = {.tv_sec = 10, .tv_usec = 0};
  sbl_consume(stubborn_link, &timeout);
  sbl_free(stubborn_link);

  return 0;
}
