#include "perfect_link.h"
#include "list.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define DELIM_LEN 1

struct PerfectLink {
  struct StubbornLink *stubborn_link;
  int n_inbox;
  void (*cb)(void *, PlDeliver *);
  void *ctx;
  list_t *inbox;
};

unsigned long djb2(const char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  return hash;
}

static void wrapper(void *ctx, SblDeliver *e) {
  struct PerfectLink *pl = ctx;

  unsigned long *hash = calloc(1, sizeof(unsigned long));
  *hash = djb2(e->msg);

  for (size_t i = 0; i < pl->inbox->count; i++) {
    if (*(unsigned long *)list_get(pl->inbox, i) == *hash) {
      return;
    }
  }

  char id[10];
  int id_len = strcspn(e->msg, ",");
  strncpy(id, e->msg, id_len);

  int offset = id_len + DELIM_LEN;

  for (size_t i = 0; i < strlen(e->msg) - DELIM_LEN; i++) {
    e->msg[i] = e->msg[i + offset];
  }

  list_add(pl->inbox, hash);
  PlDeliver pl_event = {.base = *e, .id = atol(id)};
  pl->cb(pl->ctx, &pl_event);
}

struct PerfectLink *pl_init(int id, int retransmission_period) {
  struct StubbornLink *sbl = sbl_init(id, retransmission_period);
  if (sbl == NULL)
    return NULL;

  list_t *inbox = list_init();
  if (inbox == NULL) {
    sbl_free(sbl);
    return NULL;
  }
  struct PerfectLink *pl = calloc(1, sizeof(struct PerfectLink));
  if (pl == NULL) {
    sbl_free(sbl);
    list_free(inbox);
    return NULL;
  }

  sbl_set_callback(sbl, &wrapper, pl);

  pl->n_inbox = 0;
  pl->stubborn_link = sbl;
  pl->inbox = inbox;
  return pl;
}

int pl_send(struct PerfectLink *pl, PlSend *e) {
  char buf[MAX_MSG_LEN];
  e->id = time(NULL);
  snprintf(buf, MAX_MSG_LEN, "%ld,%s", e->id, e->base.msg);
  strcpy(e->base.msg, buf);
  return sbl_send(pl->stubborn_link, &e->base);
}

void pl_consume(struct PerfectLink *pl, struct timeval *timeout) {
  return sbl_consume(pl->stubborn_link, timeout);
}

void pl_set_callback(struct PerfectLink *pl, void (*cb)(void *, PlDeliver *),
                     void *ctx) {
  pl->cb = cb;
  pl->ctx = ctx;
}

void pl_free(struct PerfectLink *pl) {
  sbl_free(pl->stubborn_link);
  list_free(pl->inbox);
}
