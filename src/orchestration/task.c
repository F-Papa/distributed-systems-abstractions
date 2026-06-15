#include "task_internal.h"
#include <stdlib.h>

task_t *task_new(void (*callback)(void *), void *context, struct timeval delay,
                 int is_recurrent) {
  task_t *t = calloc(1, sizeof(task_t));
  if (t == NULL)
    return NULL;

  t->context = context;
  t->callback = callback;
  t->delay = delay;
  t->is_recurrent = is_recurrent;
  t->status = NEW;
  return t;
}

void task_run(task_t *task) {
  task->callback(task->context);
}

void task_free(task_t *task) {
  free(task);
}
