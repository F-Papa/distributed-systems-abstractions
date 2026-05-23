#include "eventually_perfect_failure_detector.h"
#include <stdio.h>
#include <stdlib.h>

struct CustomCtx {
  char *name;
};

void on_suspect(void *ctx, Suspect *e) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Peer %d Suspected!\n", cc->name, e->peer_rank);
}
void on_restore(void *ctx, Restore *e) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Peer %d Restored!\n", cc->name, e->peer_rank);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);
  int max_rank = atoi(argv[2]);

  Epfd *epfd = epfd_init(id, max_rank, 2);
  struct CustomCtx ctx = {.name = "Franco!"};
  epfd_set_on_suspect(epfd, &on_suspect, &ctx);
  epfd_set_on_restore(epfd, &on_restore, &ctx);

  struct timeval to = {.tv_sec = 45, .tv_usec = 0};
  epfd_start(epfd, &to);
}
