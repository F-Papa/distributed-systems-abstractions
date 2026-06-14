#include "orchestration/handler.h"
#include "handler_internal.h"
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

struct HandlerList {
  int count;
  handler_t handlers[];
};

handler_t *handler_new(void (*callback)(fd_set *, fd_set *, void *),
                       void *context) {
  handler_t *handler = calloc(1, sizeof(handler_t));
  if (handler == NULL)
    return NULL;

  handler->context = context;
  handler->callback = callback;
  handler->should_free_ctx = 0;
  return handler;
}

static void handler_list_cb(fd_set *reads, fd_set *writes, void *ctx) {
  struct HandlerList *handler_list = ctx;
  for (int i = 0; i < handler_list->count; i++) {
    handler_t h = handler_list->handlers[i];
    h.callback(reads, writes, h.context);
  }
}

handler_t *handler_composite_new(handler_t **handlers, int count) {
  struct HandlerList *list =
      calloc(1, sizeof(struct HandlerList) + count * sizeof(handler_t));

  list->count = count;

  for (int i = 0; i < count; i++) {
    list->handlers[i] = *handlers[i];
  }
  handler_t *h = handler_new(&handler_list_cb, list);
  h->should_free_ctx = 1;

  return h;
}

void handler_free(handler_t *handler) {
  if (handler->should_free_ctx)
    free(handler->context);

  free(handler);
}
