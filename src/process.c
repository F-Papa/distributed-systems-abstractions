#include "broadcast/reliable_broadcast.h"
#include "broadcast/uniform_reliable_broadcast.h"
#include "constants.h"
#include "orchestration/handler.h"
#include "orchestration/orchestrator.h"
#include "syscall.h"
#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define BASE_PORT 3000

void callback(void *ctx, RbDelivery *delivery) {
  printf("[Peer %d] %s\n", delivery->sender, delivery->msg);
}

void handle_stdin(fd_set *reads, fd_set *writes, void *ctx) {
  if (FD_ISSET(STDIN_FILENO, reads)) {
    char buf[MAX_MSG_LEN];
    if (read(STDIN_FILENO, buf, MAX_MSG_LEN) <= 0) {
      printf("Error reading from STDIN\n");
      return;
    }

    char *new_line = strpbrk(buf, "\n");
    *new_line = '\0';
    Urb *urb = ctx;
    RbSend s = {};
    strcpy(s.msg, buf);
    urb_broadcast(urb, &s);
  }
}

int main(int argc, char **argv) {
  int local_rank = atoi(argv[1]);
  int max_rank = atoi(argv[2]);

  orch_t *orchestrator = orchestrator_new();

  UrbConfig config = {
      .local_rank = local_rank,
      .max_rank = max_rank,
      .base_port = BASE_PORT,
      .retransmission_period = 3,
  };

  Urb *urb = urb_init(config);
  wset_t *rb_wset = urb_get_watch_set(urb);
  orchestrator_register_watch_set(orchestrator, rb_wset);
  watch_set_free(rb_wset);
  handler_t *rb_handler = urb_get_handler(urb);
  orchestrator_add_handler(orchestrator, rb_handler);
  urb_set_callback(urb, &callback, NULL);
  int task_count;
  task_t **rb_tasks = urb_get_tasks(urb, &task_count);
  for (int i = 0; i < task_count; i++)
    orchestrator_add_task(orchestrator, rb_tasks[i]);
  free(rb_tasks);

  int fds[] = {STDIN_FILENO};
  wset_t *w_set = watch_set_new(fds, 1);
  orchestrator_register_watch_set(orchestrator, w_set);
  watch_set_free(w_set);
  handler_t *stdin_handler = handler_new(&handle_stdin, urb);
  orchestrator_add_handler(orchestrator, stdin_handler);

  struct timeval timeout = {10, 0};
  printf("Starting peer %d/%d ...\n", local_rank, max_rank);
  // sleep(3);
  printf("Started...\n");
  orchestrator_start(orchestrator, &timeout);

  orchestrator_free(orchestrator);
  printf("Exiting\n");
  return 0;
}
