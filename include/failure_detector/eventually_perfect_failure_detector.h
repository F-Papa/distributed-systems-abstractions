#ifndef EVENTUALLY_PERFECT_FAILURE_DETECTOR_H
#define EVENTUALLY_PERFECT_FAILURE_DETECTOR_H

#include <bits/types/struct_timeval.h>
#include <sys/select.h>

typedef struct {
  int peer_rank;
} Suspect;

typedef struct {
  int peer_rank;
} Restore;

typedef struct EventuallyPerfectFailureDetector Epfd;

Epfd *epfd_init(int local_rank, int max_rank, int base_port,
                int retransmission_period);

void epfd_set_on_restore(Epfd *epfd, void (*cb)(void *ctx, Restore *e),
                         void *ctx);

void epfd_set_on_suspect(Epfd *epfd, void (*cb)(void *ctx, Suspect *e),
                         void *ctx);

void epfd_start(Epfd *, struct timeval *timeout);

void epfd_free(Epfd *epfd);

int epfd_register_fd_sets(Epfd *epfd, fd_set *reads, fd_set *writes);

void epfd_handle_fd_sets(Epfd *epfd, fd_set *reads, fd_set *writes);

void epfd_handle_timeout(Epfd *epfd);

#endif
