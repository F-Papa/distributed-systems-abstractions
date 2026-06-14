#ifndef TASK_INTERNAL_H
#define TASK_INTERNAL_H

#include "orchestration/task.h"

enum TaskStatus {
  NEW,
  STARTED,
  DONE,
};

struct Task {
  void (*callback)(void *context);
  void *context;
  struct timeval delay;
  struct timeval deadline;
  int is_recurrent;
  enum TaskStatus status;
};

#endif
