#include "orchestration/orchestrator.h"
#include "syscall.h"
#include <sys/select.h>

#define BASE_PORT 3000

struct CustomCtx {
  char *name;
};

void task1(void *ctx) {
  int n = *(int *)ctx;
  char buf[1024] = "Task number 1: ";
  for (int i = 1; i <= n; i++) {
    sprintf(buf, "%s %d", buf, i);
  }
  printf("%s\n", buf);
}

void task2(void *ctx) {
  int pid = getpid();
  printf("Task 2: Hello from process %d\n", pid);
}

void task3(void *ctx) { printf("Task 3: Oneshot\n"); }

int main(int argc, char **argv) {
  printf("Initializing\n");

  orch_t *orch = orchestrator_new();

  int t1_ctx = 10;
  struct timeval t1_d = {1, 0};
  task_t *t1 = task_new(&task1, &t1_ctx, t1_d, 1);
  orchestrator_add_task(orch, t1);

  struct timeval t2_d = {0, 600 * 1e3};
  task_t *t2 = task_new(&task2, NULL, t2_d, 1);
  orchestrator_add_task(orch, t2);

  struct timeval t3_d = {3, 500 * 1e3};
  task_t *t3 = task_new(&task3, NULL, t3_d, 0);
  orchestrator_add_task(orch, t3);

  struct timeval timeout = {10, 0};
  orchestrator_start(orch, &timeout);

  orchestrator_free(orch);
  return 0;
}
