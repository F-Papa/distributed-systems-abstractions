#include "fair_loss_link.h"
#include "list.h"
#include <sys/select.h>

#define DELIM_LEN 1

list_t *_links = NULL;

struct FairLossLink {
  int id;
  int socket;
  fd_set reads;
  void (*callback)(void *, FllDeliver *);
  void *ctx;
};

void get_port(int id, char *port) {
  int port_number = 3000 + id;
  sprintf(port, "%d", port_number);
}

struct FairLossLink *fll_init(int id) {
  struct addrinfo hints, *resp;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_flags = AI_PASSIVE;

  char port[11];
  get_port(id, port);

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

  printf("Id: %d | Listening on port: %s\n", id, port);

  freeaddrinfo(resp);

  if (_links == NULL) {
    _links = list_init();
    if (_links == NULL) {
      return NULL;
    }
  }

  struct FairLossLink *fll = calloc(1, sizeof(struct FairLossLink));
  if (fll == NULL)
    return NULL;

  fll->socket = sock;
  fll->id = id;
  FD_ZERO(&fll->reads);
  FD_SET(sock, &fll->reads);

  list_add(_links, fll);
  return fll;
}

struct addrinfo *get_peer_addr(int recipient_id) {
  struct addrinfo hints, *resp;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_flags = AI_PASSIVE;

  char port[11];
  get_port(recipient_id, port);

  int status = getaddrinfo(NULL, port, &hints, &resp);
  if (status < 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return NULL;
  }

  return resp;
}

int fll_send(struct FairLossLink *fll, FllSend *e) {
  struct addrinfo *target_addr = get_peer_addr(e->recipient);

  char buf[MAX_MSG_LEN];
  int len = snprintf(buf, MAX_MSG_LEN, "%d,%s", fll->id, e->msg);

  if (sendto(fll->socket, buf, len, 0, target_addr->ai_addr,
             target_addr->ai_addrlen) <= 0) {
    printf("sendto() error (%d)\n", errno);
    return -1;
  };

  free(target_addr);
  return 0;
}

void fll_set_callback(struct FairLossLink *fll,
                      void (*cb)(void *ctx, FllDeliver *e), void *ctx) {
  fll->callback = cb;
  fll->ctx = ctx;
}

void fll_consume(struct FairLossLink *fll, struct timeval *timeout) {
  int timed_out = 0;
  while (!timed_out) {
    fd_set copy = fll->reads;

    if (select(fll->socket + 1, &copy, 0, 0, timeout) == 0)
      timed_out = 1;

    if (FD_ISSET(fll->socket, &copy)) {
      char buf[MAX_MSG_LEN];
      int len = recvfrom(fll->socket, buf, MAX_MSG_LEN - 1, 0, NULL, NULL);
      if (len <= 0) {
        printf("recfrom() error (%d)\n", errno);
      };

      buf[len] = '\0';
      char id[10];
      int id_len = strcspn(buf, ",");
      strncpy(id, buf, id_len);

      FllDeliver *e = calloc(1, sizeof(FllDeliver));

      strcpy(e->msg, buf + id_len + DELIM_LEN);
      e->sender = atoi(id);
      fll->callback(fll->ctx, e);
    }
  }
}

void fll_free(struct FairLossLink *fll) {
  if (fll->socket >= 0) {
    close(fll->socket);
  }
  int idx = list_index(_links, fll);
  if (idx >= 0) {
    list_remove(_links, idx);
  }
  if (_links->count == 0) {
    list_free(_links);
  }
}
