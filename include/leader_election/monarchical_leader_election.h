#ifndef MONARCHICAL_LEADER_ELECTION_H
#define MONARCHICAL_LEADER_ELECTION_H

#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <sys/select.h>

typedef struct LeaderIndication {
  int peer_rank;
} Leader;

typedef struct MonarchicalLeaderElector Mle;

void mle_set_on_new_leader(Mle *mle, void (*cb)(void *, Leader *), void *ctx);

void mle_start(Mle *mle, struct timeval *timeout);

Mle *mle_init(int local_rank, int max_rank, int base_port,
              int retransmission_period);

void mle_free(Mle *mle);

int mle_register_fd_sets(Mle *mle, fd_set *reads, fd_set *writes);

void mle_handle_fd_sets(Mle *mle, fd_set *reads, fd_set *writes);

void mle_handle_timeout(Mle *mle);

wset_t *mle_get_watch_set(Mle *mle);

#endif
