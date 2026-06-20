#include "broadcast/byzantine_reliable_broadcast.h"
#include "broadcast/common.h"
#include "constants.h"
#include "link/common.h"
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

void callback(void *ctx, Deliver *delivery) {
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
    Brb *brb = ctx;
    Broadcast s = {};
    strcpy(s.msg, buf);
    brb_broadcast(brb, &s);
  }
}

static int load_key(const char *path, unsigned char *key, size_t len) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    perror(path);
    return -1;
  }
  size_t n = fread(key, 1, len, f);
  fclose(f);
  return n == len ? 0 : -1;
}

int main(int argc, char **argv) {
  int local_rank = atoi(argv[1]);
  int max_rank = atoi(argv[2]);

  unsigned char private_key[crypto_sign_SECRETKEYBYTES];
  unsigned char public_keys[max_rank + 1][crypto_sign_PUBLICKEYBYTES];

  char path[256];
  snprintf(path, sizeof(path), "keys/private_%d.key", local_rank);
  if (load_key(path, private_key, crypto_sign_SECRETKEYBYTES) != 0)
    return 1;

  for (int i = 1; i <= max_rank; i++) {
    snprintf(path, sizeof(path), "keys/public_%d.key", i);
    if (load_key(path, public_keys[i], crypto_sign_PUBLICKEYBYTES) != 0)
      return 1;
  }

  orch_t *orchestrator = orchestrator_new();

  AplConfig aplConfig = {.local_rank = local_rank,
                         .max_rank = max_rank,
                         .base_port = BASE_PORT,
                         .retransmission_period = 3,
                         .private_key = private_key,
                         .public_keys = public_keys};

  BrbConfig config = {
      .max_faulty_nodes = 1, .aplConfig = aplConfig, .sender_rank = 1};

  Brb *brb = brb_init(config);
  wset_t *rb_wset = brb_get_watch_set(brb);
  orchestrator_register_watch_set(orchestrator, rb_wset);
  watch_set_free(rb_wset);
  handler_t *rb_handler = brb_get_handler(brb);
  orchestrator_add_handler(orchestrator, rb_handler);
  brb_set_callback(brb, &callback, NULL);
  int task_count;
  task_t **rb_tasks = brb_get_tasks(brb, &task_count);
  for (int i = 0; i < task_count; i++)
    orchestrator_add_task(orchestrator, rb_tasks[i]);
  free(rb_tasks);

  int fds[] = {STDIN_FILENO};
  wset_t *w_set = watch_set_new(fds, 1);
  orchestrator_register_watch_set(orchestrator, w_set);
  watch_set_free(w_set);
  handler_t *stdin_handler = handler_new(&handle_stdin, brb);
  orchestrator_add_handler(orchestrator, stdin_handler);

  struct timeval timeout = {30, 0};
  printf("Starting peer %d/%d ...\n", local_rank, max_rank);
  // sleep(3);
  printf("Started...\n");
  orchestrator_start(orchestrator, &timeout);

  orchestrator_free(orchestrator);
  brb_free(brb);
  printf("Exiting\n");
  return 0;
}
