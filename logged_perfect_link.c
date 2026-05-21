#include "logged_perfect_link.h"
#include <stdlib.h>

static void (*lpl_delivery_callback)(LplDeliver *, size_t) = NULL;
static LplDeliver *_deliveries = NULL;
static int _len_deliveries = 0;

#define DELIM_LEN 1

// TODO: Implement store/retrieve from disk
void retrieve_deliveries() {}
void store_deliveries() {}

void lpl_callback_wrapper(struct StubbornLink *sbl, SblDeliver *deliver) {
  char id[10];
  int id_len = strcspn(deliver->msg, ",");
  strncpy(id, deliver->msg, id_len);

  for (size_t i = 0; i < _len_deliveries; i++) {
    if (_deliveries[i].id == atoi(id)) {
      return;
    }
  }

  int offset = id_len + DELIM_LEN;
  for (size_t i = 0; i < strlen(deliver->msg) - DELIM_LEN; i++) {
    deliver->msg[i] = deliver->msg[i + offset];
  }

  LplDeliver lpl_event = {.base = *deliver, .id = atol(id)};
  _deliveries[_len_deliveries++] = lpl_event;
  store_deliveries();
  lpl_delivery_callback(_deliveries, _len_deliveries);
}

struct LoggedPerfectLink *lpl_init(int id, int retransmission_period) {
  struct StubbornLink *sbl = sbl_init(id, retransmission_period);
  if (sbl == NULL)
    return NULL;

  _len_deliveries = 0;
  _deliveries = calloc(MSG_CAPACITY * MAX_MSG_LEN, sizeof(char));
  if (_deliveries == NULL) {
    sbl_free(sbl);
    return NULL;
  }

  sbl_set_callback(sbl, lpl_callback_wrapper);
  struct LoggedPerfectLink *lpl = calloc(1, sizeof(struct LoggedPerfectLink));
  if (lpl == NULL) {
    sbl_free(sbl);
    free(_deliveries);
    return NULL;
  }

  lpl->stubborn_link = sbl;
  retrieve_deliveries();

  return lpl;
}

void get_deliveries(LplDeliver **deliveries, size_t *len) {
  *deliveries = _deliveries;
  *len = _len_deliveries;
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
                      void (*cb)(LplDeliver *deliveries, size_t len)) {
  lpl_delivery_callback = cb;
}

void lpl_free(struct LoggedPerfectLink *lpl) {
  sbl_free(lpl->stubborn_link);
  free(lpl);
}
