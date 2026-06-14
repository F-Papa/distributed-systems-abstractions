#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include "orchestration/handler.h"
#include "orchestration/task.h"
#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <iso646.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct Orchestrator orch_t;

orch_t *orchestrator_new();

void orchestrator_free(orch_t *orchestrator);

void orchestrator_add_task(orch_t *orchestrator, task_t *task);

void orchestrator_add_handler(orch_t *orchestrator, handler_t *handler);

void orchestrator_start(orch_t *orchestrator, struct timeval *external_timeout);

void orchestrator_register_fd(orch_t *orchestrator, int fd, event_t event);

void orchestrator_unregister_fd(orch_t *orchestrator, int fd, event_t event);

void orchestrator_register_watch_set(orch_t *orchestrator, wset_t *watch_set);

void orchestrator_clear_watch_set(orch_t *orchestrator, wset_t *watch_set);

#endif
