#ifndef EVENTUAL_LEADER_ELECTION_H
#define EVENTUAL_LEADER_ELECTION_H

#include <bits/types/struct_timeval.h>

typedef struct {
  int peer_rank;
} Trust;

typedef struct EventualLeaderElector Ele;

void ele_set_on_new_trust(Ele *ele, void (*cb)(void *, Trust *), void *ctx);

void ele_start(Ele *ele, struct timeval *timeout);

Ele *ele_init(int local_rank, int max_rank, int base_port, int retransmission_period);

void ele_free(Ele *ele);

#endif
