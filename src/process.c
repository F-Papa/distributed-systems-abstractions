#include "monarchical_leader_election.h"
#include "perfect_failure_detector.h"
#include <stdio.h>
#include <stdlib.h>

struct CustomCtx {
  char *name;
};

void on_leader(void *ctx, Leader *e) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Peer %d Is Leader!\n", cc->name, e->peer_rank);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);
  int max_rank = atoi(argv[2]);

  Mle *mle = mle_init(id, max_rank, 2);
  struct CustomCtx ctx = {.name = "Franco!"};
  mle_set_on_new_leader(mle, &on_leader, &ctx);

  struct timeval to = {.tv_sec = 45, .tv_usec = 0};
  mle_start(mle, &to);
  mle_free(mle);
}
