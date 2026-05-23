#include "perfect_failure_detector.h"
#include <stdio.h>
#include <stdlib.h>

struct CustomCtx {
  char *name;
};

void on_crash(void *ctx, Crash *e) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Peer %d Crashed!\n", cc->name, e->peer_id);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);
  int max_rank = atoi(argv[2]);

  Pfd *pfd = pfd_init(id, max_rank, 2);
  struct CustomCtx ctx = {.name = "Franco!"};
  pfd_set_oncrash(pfd, &on_crash, &ctx);

  struct timeval to = {.tv_sec = 15, .tv_usec = 0};
  pfd_start(pfd, &to);
}
