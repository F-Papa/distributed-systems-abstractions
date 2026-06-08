#include "orchestration/task.h"
#include "utils/list.h"
#include <bits/types/struct_timeval.h>
#include <iso646.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct Orchestrator {
  list_t *tasks;
} orch_t;

orch_t *orchestrator_new();

void orchestrator_free(orch_t *orchestrator);

void orchestrator_add_task(orch_t *orchestrator, task_t *task);

void orchestrator_start(orch_t *orchestrator, struct timeval *external_timeout);
