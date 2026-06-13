#include "link/stubborn_link.h"
#include "link/fair_loss_link.h"
#include "utils/list.h"
#include "utils/logging.h"
#include "utils/timeout.h"
#include <stdio.h>
#include <unistd.h>

struct StubbornLink {
  int retransmission_period;
  struct FairLossLink *fair_loss_link;
  void (*cb)(void *, SblDeliver *);
  void *ctx;
  list_t *outbox;
  struct timeval next_to;
};

static void wrapper(void *ctx, FllDeliver *e) {
  struct StubbornLink *sbl = ctx;
  debug("Calling SBL Callback\n");
  sbl->cb(sbl->ctx, e);
  debug("SBL Callback Returned\n");
}

struct StubbornLink *sbl_init(int id, int base_port,
                              int retransmission_period) {
  struct FairLossLink *fll = fll_init(id, base_port);
  if (fll == NULL)
    return NULL;

  list_t *outbox = list_init();
  if (outbox == NULL) {
    fll_free(fll);
    return NULL;
  }

  struct StubbornLink *sbl = calloc(1, sizeof(struct StubbornLink));
  if (sbl == NULL) {
    fll_free(fll);
    list_free(outbox);
    return NULL;
  }

  fll_set_callback(fll, &wrapper, sbl);

  sbl->fair_loss_link = fll;
  sbl->outbox = outbox;
  sbl->retransmission_period = retransmission_period;
  return sbl;
}

int sbl_send(struct StubbornLink *sbl, SblSend *e) {
  SblSend *copy = calloc(1, sizeof(SblSend));
  *copy = *e;
  list_add(sbl->outbox, copy);
  return fll_send(sbl->fair_loss_link, e);
}

void sbl_handle_timeout(struct StubbornLink *sbl) {
  for (size_t i = 0; i < sbl->outbox->count; i++) {
    fll_send(sbl->fair_loss_link, list_get(sbl->outbox, i));
  }
}

void sbl_handle_fd_sets(struct StubbornLink *sbl, fd_set *reads,
                        fd_set *writes) {
  fll_handle_fd_sets(sbl->fair_loss_link, reads, writes);
}

int sbl_register_fd_sets(struct StubbornLink *sbl, fd_set *reads,
                         fd_set *writes) {
  return fll_register_fd_sets(sbl->fair_loss_link, reads, writes);
}

void sbl_consume(struct StubbornLink *sbl, struct timeval *timeout) {
  struct timeval retrans_deadline, done_deadline;
  struct timeval retrans_timeout = {.tv_sec = sbl->retransmission_period,
                                    .tv_usec = 0};

  tv_reset_deadline(&retrans_deadline, &retrans_timeout);

  if (timeout) {
    gettimeofday(&done_deadline, NULL);
    timeradd(&done_deadline, timeout, &done_deadline);
  }

  int done = 0;
  while (!done) {
    struct timeval *min_to = tv_min(&retrans_timeout, timeout);

    fll_consume(sbl->fair_loss_link, min_to);

    struct timeval now;
    gettimeofday(&now, NULL);
    if (timercmp(&now, &retrans_deadline, >=)) {
      sbl_handle_timeout(sbl);
      retrans_timeout.tv_sec = sbl->retransmission_period;
      retrans_timeout.tv_usec = 0;
      tv_reset_deadline(&retrans_deadline, &retrans_timeout);
    }

    done = timeout && timercmp(&now, &done_deadline, >=);
  }
}

void sbl_set_callback(struct StubbornLink *sbl,
                      void (*cb)(void *, SblDeliver *), void *ctx) {
  sbl->cb = cb;
  sbl->ctx = ctx;
}

void sbl_free(struct StubbornLink *sbl) {
  fll_free(sbl->fair_loss_link);
  list_free(sbl->outbox);
  free(sbl);
}

wset_t *sbl_get_watch_set(struct StubbornLink *sbl) {
  return fll_get_watch_set(sbl->fair_loss_link);
}
