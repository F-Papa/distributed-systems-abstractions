#include "broadcast/best_effort_broadcast.h"
#include "link/perfect_link.h"
#include <stdlib.h>
#include <string.h>

struct BestEffortBroadcast {
  int local_rank;
  int max_rank;
  void (*cb)(void *, BebDelivery *);
  void *ctx;
  struct PerfectLink *perfect_link;
};

static void wrapper(void *ctx, PlDeliver *e) {
  BebDelivery delivery = {.sender = e->base.sender};
  strncpy(delivery.msg, e->base.msg, MAX_MSG_LEN);
  Beb *beb = ctx;
  beb->cb(beb->ctx, &delivery);
}

Beb *beb_init(int local_rank, int max_rank, int base_port,
              int retransmission_period) {
  struct PerfectLink *pl =
      pl_init(local_rank, base_port, retransmission_period);
  if (pl == NULL) {
    return NULL;
  }

  Beb *beb = calloc(1, sizeof(Beb));
  if (beb == NULL) {
    pl_free(pl);
    return NULL;
  }

  beb->local_rank = local_rank;
  beb->max_rank = max_rank;
  beb->perfect_link = pl;
  pl_set_callback(pl, &wrapper, beb);
  return beb;
}

int beb_broadcast(Beb *beb, BebSend *e) {
  PlSend msg;
  strncpy(msg.base.msg, e->msg, MAX_MSG_LEN);

  for (size_t peer_rank = 1; peer_rank <= beb->max_rank; peer_rank++) {
    msg.base.recipient = peer_rank;
    int status = pl_send(beb->perfect_link, &msg);

    if (status != 0)
      return status;
  }

  return 0;
}

void beb_consume(Beb *beb, struct timeval *timeout) {
  pl_consume(beb->perfect_link, timeout);
}

void beb_set_callback(Beb *beb, void (*cb)(void *, BebDelivery *), void *ctx) {
  beb->cb = cb;
  beb->ctx = ctx;
}

int beb_register_fd_sets(Beb *beb, fd_set *reads, fd_set *writes) {
  return pl_register_fd_sets(beb->perfect_link, reads, writes);
}

void beb_handle_fd_sets(Beb *beb, fd_set *reads, fd_set *writes) {
  pl_handle_fd_sets(beb->perfect_link, reads, writes);
}

void beb_handle_timeout(Beb *beb) { pl_handle_timeout(beb->perfect_link); }

void beb_free(Beb *beb) {
  pl_free(beb->perfect_link);
  free(beb);
}
