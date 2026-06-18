#include "link/perfect_link.h"
#include "constants.h"
#include "link/stubborn_link.h"
#include "utils/list.h"
#include "utils/logging.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <uuid.h>

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
    hash = ((hash << 5) + hash) + c;
  return hash;
}

static void wrapper(void *ctx, SblDeliver *e) {
  struct PerfectLink *pl = ctx;

  char id[UUID_STR_LEN];
  int id_len = strcspn(e->msg, ",");
  strncpy(id, e->msg, id_len);
  id[id_len] = '\0';

  unsigned long *hash = calloc(1, sizeof(unsigned long));
  char to_hash[MAX_MSG_LEN];
  snprintf(to_hash, MAX_MSG_LEN, "%d%s", e->sender, id);
  *hash = djb2(to_hash);

  for (size_t i = 0; i < pl->inbox->count; i++) {
    if (*(unsigned long *)list_get(pl->inbox, i) == *hash) {
      free(hash);
      return;
    }
  }

  int offset = id_len + DELIM_LEN;
  for (size_t i = 0; i <= strlen(e->msg) - offset; i++) {
    e->msg[i] = e->msg[i + offset];
  }

  list_add(pl->inbox, hash);
  PlDeliver pl_event = {.sender = e->sender};
  strcpy(pl_event.msg, e->msg);
  debug("Calling PL Callback\n");
  pl->cb(pl->ctx, &pl_event);
  debug("PL Callback Returned\n");
}

struct PerfectLink *pl_init(int id, int base_port, int retransmission_period) {
  struct StubbornLink *sbl = sbl_init(id, base_port, retransmission_period);
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
    list_free(inbox, NULL);
    return NULL;
  }

  sbl_set_callback(sbl, &wrapper, pl);

  pl->n_inbox = 0;
  pl->stubborn_link = sbl;
  pl->inbox = inbox;
  return pl;
}

int pl_send(struct PerfectLink *pl, PlSend *e) {
  uuid_t uuid;
  uuid_generate_random(uuid);
  char id[UUID_STR_LEN];
  uuid_unparse(uuid, id);
  SblSend msg_with_id = {.recipient = e->recipient};
  snprintf(msg_with_id.msg, MAX_MSG_LEN, "%s,%s", id, e->msg);
  return sbl_send(pl->stubborn_link, &msg_with_id);
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
  list_free(pl->inbox, NULL);
}

void pl_handle_timeout(struct PerfectLink *pl) {
  sbl_handle_timeout(pl->stubborn_link);
}

wset_t *pl_get_watch_set(struct PerfectLink *pl) {
  return sbl_get_watch_set(pl->stubborn_link);
}

handler_t *pl_get_handler(struct PerfectLink *pl) {
  return sbl_get_handler(pl->stubborn_link);
}

task_t **pl_get_tasks(struct PerfectLink *pl, int *count) {
  return sbl_get_tasks(pl->stubborn_link, count);
}
