#include "watch_set.h"
#include <stdlib.h>
#include <sys/select.h>

struct WatchSet {
  int fd_count;
  int fds[];
};

wset_t *watch_set_new(int *fds, int fd_count) {
  wset_t *t = calloc(1, sizeof(wset_t));
  if (t == NULL)
    return NULL;

  t->fd_count = fd_count;
  for (int i = 0; i < fd_count; i++) {
    t->fds[i] = fds[i];
  }

  return t;
}

void watch_set_free(wset_t *watch_set) { free(watch_set); }

void watch_set_register(wset_t *watch_set, fd_set *reads, int *nfds) {
  for (int i = 0; i < watch_set->fd_count; i++) {
    int fd = watch_set->fds[i];
    FD_SET(fd, reads);
    if (fd > *nfds)
      *nfds = fd;
  }
}

void watch_set_clear(wset_t *watch_set, fd_set *reads) {
  for (int i = 0; i < watch_set->fd_count; i++) {
    FD_CLR(watch_set->fds[i], reads);
  }
}
