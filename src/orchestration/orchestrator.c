#include "orchestration/orchestrator.h"
#include "utils/list.h"
#include "utils/timeout.h"
#include <iso646.h>
#include <sys/select.h>
#include <sys/time.h>

struct Orchestrator {
  list_t *tasks;
  list_t *handlers;
  fd_set reads;
  fd_set writes;
  int nfds;
};

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

  list_t *handlers = list_init();
  if (handlers == NULL) {
    list_free(tasks);
    free(o);
    return NULL;
  }

  FD_ZERO(&o->reads);
  FD_ZERO(&o->writes);
  o->tasks = tasks;
  o->handlers = handlers;
  return o;
}

void orchestrator_register_fd(orch_t *orchestrator, int fd, event_t event) {
  if (event & READ) {
    FD_SET(fd, &orchestrator->reads);
    if (orchestrator->nfds < fd)
      orchestrator->nfds = fd;
  }
  if (event & WRITE) {
    FD_SET(fd, &orchestrator->writes);
    if (orchestrator->nfds < fd)
      orchestrator->nfds = fd;
  }
}

void orchestrator_unregister_fd(orch_t *orchestrator, int fd, event_t event) {
  if (event & READ) {
    FD_CLR(fd, &orchestrator->reads);
  }
  if (event & WRITE) {
    FD_CLR(fd, &orchestrator->writes);
  }
}

void orchestrator_free(orch_t *orchestrator) {
  list_free(orchestrator->tasks);
  free(orchestrator);
}

void orchestrator_add_task(orch_t *orchestrator, task_t *task) {
  task->status = NEW;
  list_add(orchestrator->tasks, task);
}

void orchestrator_add_handler(orch_t *orchestrator, handler_t *handler) {
  list_add(orchestrator->handlers, handler);
}

static void orchestrator_set_deadlines(orch_t *orchestrator) {
  for (int i = 0; i < orchestrator->tasks->count; i++) {
    task_t *task = list_get(orchestrator->tasks, i);
    if (task->status == NEW) {
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

    fd_set reads_cpy = orchestrator->reads;
    fd_set writes_cpy = orchestrator->writes;

    if (select(orchestrator->nfds + 1, &reads_cpy, &writes_cpy, NULL,
               &timeout) == 0) {
      struct timeval now;
      gettimeofday(&now, NULL);

      for (int i = 0; i < orchestrator->tasks->count; i++) {
        task_t *task = list_get(orchestrator->tasks, i);

        if (task->status != DONE && timercmp(&now, &task->deadline, >=)) {
          task->callback(task->context);

          if (task->is_recurrent == 1) {
            tv_reset_deadline(&task->deadline, &task->delay);
          } else {
            task->status = DONE;
          }
        }
      }
      done = external_timeout && timercmp(&now, &external_deadline, >=);
    } else {
      for (int i = 0; i < orchestrator->handlers->count; i++) {
        handler_t *handler = list_get(orchestrator->handlers, i);
        handler->callback(&reads_cpy, &writes_cpy, handler->context);
      }
    }
  }
}
