
#include <bits/types/struct_timeval.h>
#include <stdlib.h>
#include <unistd.h>

enum TaskStatus {
  NEW,
  STARTED,
  DONE,
};

typedef struct Task {
  void (*callback)(void *context);
  void *context;
  struct timeval delay;
  struct timeval deadline;
  int is_recurrent;
  enum TaskStatus status;
} task_t;

inline static task_t *task_new(void (*callback)(void *), void *context,
                               struct timeval delay, int is_recurrent) {
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
