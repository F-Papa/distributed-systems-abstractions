#ifndef PERFECT_FAILURE_DETECTOR_H
#define PERFECT_FAILURE_DETECTOR_H

#include "orchestration/handler.h"
#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <sys/select.h>

typedef struct {
  int peer_id;
} Crash;

typedef struct PerfectFailureDetector Pfd;

Pfd *pfd_init(int local_rank, int max_rank, int base_port,
              int retransmission_period);

void pfd_set_oncrash(struct PerfectFailureDetector *pfd,
                     void (*cb)(void *ctx, Crash *e), void *ctx);

void pfd_start(struct PerfectFailureDetector *pfd, struct timeval *timeout);

void pfd_free(struct PerfectFailureDetector *pfd);

void pfd_handle_timeout(struct PerfectFailureDetector *pfd);

wset_t *pfd_get_watch_set(struct PerfectFailureDetector *pfd);

handler_t *pfd_get_handler(Pfd *pfd);

#endif
