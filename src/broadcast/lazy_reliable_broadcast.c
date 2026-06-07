#include "broadcast/reliable_broadcast.h"

struct ReliableBroadcast {
  int local_rank;
  int max_rank;
  struct PerfectLink *perfect_link;
};
