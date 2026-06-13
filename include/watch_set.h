#ifndef WATCH_SET_H
#define WATCH_SET_H

#include <sys/select.h>
typedef struct WatchSet wset_t;

typedef enum FdEvent {
  READ = 1 << 0,
  WRITE = 1 << 1,
} event_t;

wset_t *watch_set_new(int *fds, int fd_count);

void watch_set_free(wset_t *w_set);

void watch_set_register(wset_t *w_set, fd_set *reads, int *nfds);

void watch_set_clear(wset_t *w_set, fd_set *reads);

#endif
