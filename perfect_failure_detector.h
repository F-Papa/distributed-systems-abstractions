#include "perfect_link.h"

typedef struct {
  int peer_id;
} Crash;

typedef struct PerfectFailureDetector {
  struct PerfectLink *perfect_link;
  void (*pl_delivery_callback)(Crash *);
  int peers_alive[];
} Pfd;

Pfd *pfd_init(int max_rank, int rank, int retransmission_period);
void pfd_set_oncrash(Pfd *pfd, Crash *e);
