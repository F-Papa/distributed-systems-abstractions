#ifndef HANDLER_H
#define HANDLER_H

#include <bits/types/struct_timeval.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>

typedef struct Handler {
  void (*callback)(fd_set *reads, fd_set *writes, void *context);
  void *context;
} handler_t;

inline static handler_t *
handler_new(void (*callback)(fd_set *, fd_set *, void *), void *context) {
  handler_t *t = calloc(1, sizeof(handler_t));
  if (t == NULL)
    return NULL;

  t->context = context;
  t->callback = callback;
  return t;
}

#endif // !HANDLER_H
