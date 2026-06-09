#include "orchestration/orchestrator.h"
#include "sys/socket.h"
#include "syscall.h"
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define BASE_PORT 3000

struct PingContext {
  int socket;
  int local_rank;
  int max_rank;
  int base_port;
};

static void get_port(int id, int base_port, char *port) {
  int port_number = base_port + id;
  sprintf(port, "%d", port_number);
}

static int get_socket(int local_rank, int base_port) {
  struct addrinfo hints, *resp;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_flags = AI_PASSIVE;

  char port[11];
  get_port(local_rank, base_port, port);

  int status = getaddrinfo("0.0.0.0", port, &hints, &resp);
  if (status < 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return NULL;
  }

  int sock = socket(resp->ai_family, resp->ai_socktype, resp->ai_protocol);

  if (sock < 0) {
    fprintf(stderr, "socket() error: %d\n", errno);
    freeaddrinfo(resp);
    return NULL;
  }
  if (bind(sock, resp->ai_addr, resp->ai_addrlen) < 0) {
    fprintf(stderr, "bind() error: %d\n", errno);
    freeaddrinfo(resp);
    return NULL;
  }

  printf("Id: %d | Listening on port: %s\n", local_rank, port);
  freeaddrinfo(resp);
  return sock;
}

static struct addrinfo *get_peer_addr(int peer_id, int base_port) {
  struct addrinfo hints, *resp;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_flags = AI_PASSIVE;

  char port[11];
  get_port(peer_id, base_port, port);

  int status = getaddrinfo(NULL, port, &hints, &resp);
  if (status < 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return NULL;
  }

  return resp;
}

void send_ping(void *ctx) {
  struct PingContext *context = ctx;
  char buf[100];
  sprintf(buf, "PING from %d", context->local_rank);

  for (int peer = 1; peer <= context->max_rank; peer++) {
    if (peer == context->local_rank) {
      continue;
    }
    struct addrinfo *target_addr = get_peer_addr(peer, context->base_port);
    sendto(context->socket, buf, strlen(buf), 0, target_addr->ai_addr,
           target_addr->ai_addrlen);
    free(target_addr);
  }
}

struct PongContext {
  int socket;
  int local_rank;
};

void send_pong(fd_set *reads, fd_set *writes, void *ctx) {
  struct PongContext *context = ctx;

  if (!FD_ISSET(context->socket, reads)) {
    printf("Handler called when not ready to read\n");
    return;
  }

  char buf[100];
  struct sockaddr_in peer;
  socklen_t peer_len = sizeof(peer);
  int len = recvfrom(context->socket, buf, 100, 0, (struct sockaddr *)&peer,
                     &peer_len);
  buf[len] = '\0';
  printf("Received: %s\n", buf);

  if (strncmp("PONG", buf, strlen("PONG")) == 0) {
    return;
  }

  sprintf(buf, "PONG from %d", context->local_rank);
  sendto(context->socket, buf, strlen(buf), 0, (struct sockaddr *)&peer,
         peer_len);
}

int main(int argc, char **argv) {
  printf("Initializing...\n");
  int local_rank = atoi(argv[1]);
  int max_rank = atoi(argv[2]);
  printf("Local Rank: %d, Max Rank: %d\n", local_rank, max_rank);

  int sock = get_socket(local_rank, BASE_PORT);
  orch_t *orch = orchestrator_new();

  struct PingContext ping_ctx = {
      .base_port = BASE_PORT,
      .local_rank = local_rank,
      .max_rank = max_rank,
      .socket = sock,
  };

  struct timeval ping_d = {1, 500 * 1e3};
  task_t *ping_task = task_new(&send_ping, &ping_ctx, ping_d, 1);
  orchestrator_add_task(orch, ping_task);

  struct PongContext pong_ctx = {.socket = sock, .local_rank = local_rank};
  handler_t *pong_handler = handler_new(&send_pong, &pong_ctx);
  orchestrator_add_handler(orch, pong_handler);
  orchestrator_register_fd(orch, sock, READ);

  struct timeval timeout = {10, 0};
  orchestrator_start(orch, &timeout);

  orchestrator_free(orch);
  return 0;
}
