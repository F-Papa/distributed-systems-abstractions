#ifndef HANDLER_INTERNAL_H
#define HANDLER_INTERNAL_H

#include "orchestration/handler.h"

struct Handler {
  void (*callback)(fd_set *reads, fd_set *writes, void *context);
  void *context;
  bool should_free_ctx;
};

#endif
