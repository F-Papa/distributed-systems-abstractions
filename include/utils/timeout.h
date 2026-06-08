#ifndef TIMEOUT_UTILS_H
#define TIMEOUT_UTILS_H

#include <stdlib.h>
#include <sys/time.h>

static inline struct timeval *tv_min(struct timeval *a, struct timeval *b) {
  return (!b || timercmp(a, b, <)) ? a : b;
}

static inline void tv_reset_deadline(struct timeval *deadline,
                                     const struct timeval *timeout) {
  gettimeofday(deadline, NULL);
  timeradd(deadline, timeout, deadline);
}

static inline void tv_time_to_deadline(struct timeval *deadline,
                                       struct timeval *time) {
  gettimeofday(time, NULL);
  timersub(deadline, time, time);
}

#endif
