#include "constants.h"
#include "link/logged_perfect_link.h"
#include <stdlib.h>
#include <string.h>
#include <uuid.h>

// TODO: Implement store/retrieve from disk
void retrieve_deliveries() {}
void store_deliveries() {}

struct LoggedPerfectLink {
  struct StubbornLink *stubborn_link;
  void (*cb)(void *, const list_t *);
  void *ctx;
  list_t *deliveries;
};

static void wrapper(void *ctx, SblDeliver *e) {
  char id[UUID_STR_LEN];
  int id_len = strcspn(e->msg, ",");
  strncpy(id, e->msg, id_len);
  id[id_len] = '\0';

  struct LoggedPerfectLink *lpl = ctx;

  for (size_t i = 0; i < lpl->deliveries->count; i++) {
    LplDeliver *prev_delivery = list_get(lpl->deliveries, i);
    if (strcmp(prev_delivery->id, id) == 0) {
      return;
    }
  }

  int offset = id_len + DELIM_LEN;
  for (size_t i = 0; i < strlen(e->msg) - DELIM_LEN; i++) {
    e->msg[i] = e->msg[i + offset];
  }

  LplDeliver *lpl_delivery = calloc(1, sizeof(LplDeliver));
  strcpy(lpl_delivery->id, id);
  lpl_delivery->base = *e;

  list_add(lpl->deliveries, lpl_delivery);
  store_deliveries();
  lpl->cb(lpl->ctx, lpl->deliveries);
}

struct LoggedPerfectLink *lpl_init(int id, int base_port,
                                   int retransmission_period) {
  struct StubbornLink *sbl = sbl_init(id, base_port, retransmission_period);
  if (sbl == NULL)
    return NULL;

  list_t *deliveries = list_init();
  if (deliveries == NULL) {
    sbl_free(sbl);
    return NULL;
  }

  struct LoggedPerfectLink *lpl = calloc(1, sizeof(struct LoggedPerfectLink));
  if (lpl == NULL) {
    sbl_free(sbl);
    list_free(deliveries);
    return NULL;
  }

  sbl_set_callback(sbl, &wrapper, lpl);
  lpl->deliveries = deliveries;
  lpl->stubborn_link = sbl;
  retrieve_deliveries();

  return lpl;
}

int lpl_send(struct LoggedPerfectLink *lpl, LplSend *e) {
  uuid_t uuid;
  uuid_generate_random(uuid);
  uuid_unparse(uuid, e->id);
  char buf[MAX_MSG_LEN];
  snprintf(buf, MAX_MSG_LEN, "%s,%s", e->id, e->base.msg);
  strcpy(e->base.msg, buf);
  return sbl_send(lpl->stubborn_link, &e->base);
}

void lpl_consume(struct LoggedPerfectLink *lpl, struct timeval *timeout) {
  return sbl_consume(lpl->stubborn_link, timeout);
};

void lpl_set_callback(struct LoggedPerfectLink *lpl,
                      void (*cb)(void *ctx, const list_t *deliveries),
                      void *ctx) {
  lpl->cb = cb;
  lpl->ctx = ctx;
}

void lpl_free(struct LoggedPerfectLink *lpl) {
  sbl_free(lpl->stubborn_link);
  list_free(lpl->deliveries);
  free(lpl);
}

int lpl_register_fd_sets(struct LoggedPerfectLink *lpl, fd_set *reads,
                         fd_set *writes) {
  return sbl_register_fd_sets(lpl->stubborn_link, reads, writes);
}

void lpl_handle_fd_sets(struct LoggedPerfectLink *lpl, fd_set *reads,
                        fd_set *writes) {
  sbl_handle_fd_sets(lpl->stubborn_link, reads, writes);
}

void lpl_handle_timeout(struct LoggedPerfectLink *lpl) {
  sbl_handle_timeout(lpl->stubborn_link);
}
