#include "perfect_link.h"
#include "list.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DELIM_LEN 1

static list_t *_perfect_links = NULL;

unsigned long djb2(const char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  return hash;
}

void pl_callback_wrapper(struct StubbornLink *sbl, SblDeliver *deliver) {
  if (_perfect_links == NULL || _perfect_links->count == 0) {
    return;
  }

  lnode_t *node = _perfect_links->start;
  struct PerfectLink *pl;
  for (node = _perfect_links->start; node != NULL; node = node->next) {
    pl = node->element;
    if (pl->stubborn_link == sbl) {
      break;
    }
  }

  if (pl->stubborn_link != sbl) {
    return;
  }

  unsigned long hash = djb2(deliver->msg);
  for (size_t i = 0; i < pl->n_inbox; i++) {
    if (pl->inbox[i] == hash) {
      return;
    }
  }

  char id[10];
  int id_len = strcspn(deliver->msg, ",");
  strncpy(id, deliver->msg, id_len);

  int offset = id_len + DELIM_LEN;

  for (size_t i = 0; i < strlen(deliver->msg) - DELIM_LEN; i++) {
    deliver->msg[i] = deliver->msg[i + offset];
  }

  pl->inbox[pl->n_inbox++] = hash;
  PlDeliver pl_event = {.base = *deliver, .id = atol(id)};
  pl->cb(pl, &pl_event);
}

struct PerfectLink *pl_init(int id, int retransmission_period) {
  struct StubbornLink *sbl = sbl_init(id, retransmission_period);
  if (sbl == NULL)
    return NULL;

  sbl_set_callback(sbl, pl_callback_wrapper);

  if (_perfect_links == NULL) {
    _perfect_links = list_init();
    if (_perfect_links == NULL) {
      return NULL;
    }
  }

  struct PerfectLink *pl = calloc(1, sizeof(struct PerfectLink));
  if (pl == NULL) {
    sbl_free(sbl);
    return NULL;
  }

  pl->n_inbox = 0;
  pl->stubborn_link = sbl;
  list_add(_perfect_links, pl);
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

void pl_set_callback(struct PerfectLink *pl,
                     void (*cb)(struct PerfectLink *, PlDeliver *)) {
  pl->cb = cb;
}

void pl_free(struct PerfectLink *pl) {
  sbl_free(pl->stubborn_link);
  int idx = list_index(_perfect_links, pl);
  if (idx >= 0) {
    list_remove(_perfect_links, idx);
  }

  if (_perfect_links->count == 0) {
    list_free(_perfect_links);
  }
}
