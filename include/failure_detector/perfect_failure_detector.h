#include "link/perfect_link.h"
#include <bits/types/struct_timeval.h>

typedef struct {
  int peer_id;
} Crash;

typedef struct PerfectFailureDetector Pfd;

Pfd *pfd_init(int local_rank, int max_rank, int base_port, int retransmission_period);

void pfd_set_oncrash(struct PerfectFailureDetector *pfd,
                     void (*cb)(void *ctx, Crash *e), void *ctx);

void pfd_start(struct PerfectFailureDetector *pfd, struct timeval *timeout);

void pfd_free(struct PerfectFailureDetector *pfd);
