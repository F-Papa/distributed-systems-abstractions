#include "stubborn_link.h"

struct StubbornLink *sbl_init(int id, int retransmission_period) {
  struct FairLossLink *fll = fll_init(id);
  if (fll == NULL)
    return NULL;

  struct StubbornLink *sbl = calloc(1, sizeof(struct StubbornLink));
  if (sbl == NULL) {
    free(fll);
    return NULL;
  }

  sbl->retransmission_period = retransmission_period;
  sbl->fair_loss_link = fll;
  return sbl;
}

int sbl_send(struct StubbornLink *sbl, SblSend *e) {
  int duplicate = 0;
  sbl->outbox[sbl->n_outbox++] = *e;
  return fll_send(sbl->fair_loss_link, e);
}

void sbl_consume(struct StubbornLink *sbl, struct timeval *timeout) {
  struct timeval retrans_deadline, done_deadline;
  struct timeval retrans_timeout = {.tv_sec = sbl->retransmission_period,
                                    .tv_usec = 0};

  gettimeofday(&retrans_deadline, NULL);
  timeradd(&retrans_deadline, &retrans_timeout, &retrans_deadline);

  if (timeout) {
    gettimeofday(&done_deadline, NULL);
    timeradd(&done_deadline, timeout, &done_deadline);
  }

  for (int done = 0; !done;) {
    struct timeval *min_to;
    if (!timeout || timercmp(&retrans_timeout, timeout, <)) {
      min_to = &retrans_timeout;
    } else {
      min_to = timeout;
    }

    fll_consume(sbl->fair_loss_link, min_to);

    struct timeval now;
    gettimeofday(&now, NULL);
    if (timercmp(&now, &retrans_deadline, >=)) {
      for (size_t i = 0; i < sbl->n_outbox; i++) {
        fll_send(sbl->fair_loss_link, &sbl->outbox[i]);
      }
      retrans_timeout.tv_sec = sbl->retransmission_period;
      retrans_timeout.tv_usec = 0;
      gettimeofday(&retrans_deadline, NULL);
      timeradd(&retrans_deadline, &retrans_timeout, &retrans_deadline);
    }

    done = timeout && timercmp(&now, &done_deadline, >=);
  }
}

void sbl_set_callback(struct StubbornLink *sbl, void (*cb)(SblDeliver *e)) {
  fll_set_callback(sbl->fair_loss_link, cb);
}

void sbl_free(struct StubbornLink *sbl) {
  fll_free(sbl->fair_loss_link);
  free(sbl);
}
