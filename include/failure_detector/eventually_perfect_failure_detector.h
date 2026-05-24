#include "link/perfect_link.h"
#include <bits/types/struct_timeval.h>

typedef struct {
  int peer_rank;
} Suspect;

typedef struct {
  int peer_rank;
} Restore;

typedef struct EventuallyPerfectFailureDetector Epfd;

Epfd *epfd_init(int local_rank, int max_rank, int base_port, int retransmission_period);

void epfd_set_on_restore(Epfd *epfd, void (*cb)(void *ctx, Restore *e),
                         void *ctx);

void epfd_set_on_suspect(Epfd *epfd, void (*cb)(void *ctx, Suspect *e),
                         void *ctx);

void epfd_start(Epfd *, struct timeval *timeout);

void epfd_free(Epfd *epfd);
