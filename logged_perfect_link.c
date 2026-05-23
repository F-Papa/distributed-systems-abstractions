#include "logged_perfect_link.h"
#include <stdlib.h>

#define DELIM_LEN 1

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
  char id[50];
  int id_len = strcspn(e->msg, ",");
  strncpy(id, e->msg, id_len);

  struct LoggedPerfectLink *lpl = ctx;

  for (size_t i = 0; i < lpl->deliveries->count; i++) {
    LplDeliver *prev_delivery = list_get(lpl->deliveries, i);
    if (prev_delivery->id == atoi(id)) {
      return;
    }
  }

  int offset = id_len + DELIM_LEN;
  for (size_t i = 0; i < strlen(e->msg) - DELIM_LEN; i++) {
    e->msg[i] = e->msg[i + offset];
  }

  LplDeliver *lpl_delivery = calloc(1, sizeof(LplDeliver));
  lpl_delivery->id = atoi(id);
  lpl_delivery->base = *e;

  list_add(lpl->deliveries, lpl_delivery);
  store_deliveries();
  lpl->cb(lpl->ctx, lpl->deliveries);
}

struct LoggedPerfectLink *lpl_init(int id, int retransmission_period) {
  struct StubbornLink *sbl = sbl_init(id, retransmission_period);
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
  char buf[MAX_MSG_LEN];
  int random = (float)rand() / (float)RAND_MAX * 10e7;
  e->id = time(NULL) - random;
  snprintf(buf, MAX_MSG_LEN, "%ld,%s", e->id, e->base.msg);
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
