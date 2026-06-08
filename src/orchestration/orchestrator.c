#include "orchestration/orchestrator.h"
#include "utils/logging.h"
#include "utils/timeout.h"
#include <sys/time.h>

orch_t *orchestrator_new() {
  orch_t *o = calloc(1, sizeof(orch_t));
  if (o == NULL) {
    return NULL;
  }

  list_t *tasks = list_init();
  if (tasks == NULL) {
    free(o);
    return NULL;
  }

  o->tasks = tasks;
  return o;
}

void orchestrator_free(orch_t *orchestrator) {
  list_free(orchestrator->tasks);
  free(orchestrator);
}

void orchestrator_add_task(orch_t *orchestrator, task_t *task) {
  task->status = NEW;
  list_add(orchestrator->tasks, task);
}

static void orchestrator_set_deadlines(orch_t *orchestrator) {
  for (int i = 0; i < orchestrator->tasks->count; i++) {
    task_t *task = list_get(orchestrator->tasks, i);
    if (task->status == NEW) {
      debug("Deadline set\n");
      task->status = STARTED;
      tv_reset_deadline(&task->deadline, &task->delay);
    }
  }
}

static struct timeval *
orchestrator_get_next_deadline(orch_t *orchestrator,
                               struct timeval *external_deadline) {
  struct timeval *next_deadline = NULL;

  for (int i = 0; i < orchestrator->tasks->count; i++) {
    task_t *task = list_get(orchestrator->tasks, i);

    if (next_deadline == NULL || timercmp(&task->deadline, next_deadline, <)) {
      next_deadline = &task->deadline;
    }

    if (external_deadline) {
      next_deadline = tv_min(next_deadline, external_deadline);
    }
  }

  return next_deadline;
}

void orchestrator_start(orch_t *orchestrator,
                        struct timeval *external_timeout) {

  struct timeval external_deadline;
  if (external_timeout != NULL) {
    tv_reset_deadline(&external_deadline, external_timeout);
  }

  orchestrator_set_deadlines(orchestrator);

  int done = 0;
  while (!done) {
    struct timeval *next_deadline =
        orchestrator_get_next_deadline(orchestrator, &external_deadline);

    // TODO: Dont wait for timeout if no tasks are scheduled
    if (next_deadline == NULL)
      return;

    struct timeval timeout;
    tv_time_to_deadline(next_deadline, &timeout);

    select(0, NULL, NULL, NULL, &timeout);

    struct timeval now;
    gettimeofday(&now, NULL);

    for (int i = 0; i < orchestrator->tasks->count; i++) {
      task_t *task = list_get(orchestrator->tasks, i);

      if (task->status != DONE && timercmp(&now, &task->deadline, >=)) {
        void (*callback)(void *) = task->handler;
        callback(task->context);

        if (task->is_recurrent == 1) {
          tv_reset_deadline(&task->deadline, &task->delay);
        } else {
          task->status = DONE;
        }
      }
    }

    done = external_timeout && timercmp(&now, &external_deadline, >=);
  }
}
