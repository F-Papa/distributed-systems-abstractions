#include <bits/types/struct_timeval.h>

typedef struct LeaderIndication {
  int peer_rank;
} Leader;

typedef struct MonarchicalLeaderElector Mle;

void mle_set_on_new_leader(Mle *mle, void (*cb)(void *, Leader *), void *ctx);

void mle_start(Mle *mle, struct timeval *timeout);

Mle *mle_init(int local_rank, int max_rank, int base_port, int retransmission_period);

void mle_free(Mle *mle);
