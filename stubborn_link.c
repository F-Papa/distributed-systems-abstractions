#include "stubborn_link.h"
#include "list.h"
#include <stdio.h>
#include <unistd.h>

static list_t *_stubborn_links = NULL;

void sbl_wrapper(struct FairLossLink *fll, FllDeliver *e) {
  if (_stubborn_links == NULL || _stubborn_links->count == 0) {
    return;
  }

  lnode_t *node = _stubborn_links->start;
  struct StubbornLink *sbl;
  for (node = _stubborn_links->start; node != NULL; node = node->next) {
    sbl = node->element;
    if (sbl->fair_loss_link == fll)
      break;
  }

  if (node == NULL) {
    return;
  }

  sbl->cb(sbl, e);
}

struct StubbornLink *sbl_init(int id, int retransmission_period) {
  struct FairLossLink *fll = fll_init(id);
  if (fll == NULL)
    return NULL;

  fll_set_callback(fll, &sbl_wrapper);

  if (_stubborn_links == NULL) {
    _stubborn_links = list_init();
    if (_stubborn_links == NULL) {
      fll_free(fll);
      return NULL;
    }
  }

  struct StubbornLink *sbl = calloc(1, sizeof(struct StubbornLink));
  sbl->fair_loss_link = fll;
  sbl->retransmission_period = retransmission_period;
  list_add(_stubborn_links, sbl);
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

void sbl_set_callback(struct StubbornLink *sbl,
                      void (*cb)(struct StubbornLink *, SblDeliver *)) {
  sbl->cb = cb;
}

void sbl_free(struct StubbornLink *sbl) {
  fll_free(sbl->fair_loss_link);
  int idx = list_index(_stubborn_links, sbl);
  if (idx >= 0) {
    list_remove(_stubborn_links, idx);
  }

  if (_stubborn_links->count == 0) {
    list_free(_stubborn_links);
  }
}
