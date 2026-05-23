#include "leader_election/eventual_leader_election.h"
#include <stdio.h>
#include <stdlib.h>

struct CustomCtx {
  char *name;
};

void on_leader(void *ctx, Trust *e) {
  struct CustomCtx *cc = ctx;
  printf("[CTX: %s] Peer %d Is Trusted!\n", cc->name, e->peer_rank);
}

int main(int argc, char **argv) {
  int id = atoi(argv[1]);
  int max_rank = atoi(argv[2]);

  Ele *ele = ele_init(id, max_rank, 2);
  struct CustomCtx ctx = {.name = "Franco!"};
  ele_set_on_new_trust(ele, &on_leader, &ctx);

  struct timeval to = {.tv_sec = 45, .tv_usec = 0};
  ele_start(ele, &to);
  ele_free(ele);
}
