#include "perfect_failure_detector.h"
#include "list.h"
#include "string.h"

static list_t *_perfect_links = NULL;

void pfd_callback(struct PerfectLink *pl, PlDeliver *e) {
  if (strpbrk(e->base.msg, "HB") == e->base.msg) {

  } else if (strpbrk(e->base.msg, "IA") == e->base.msg) {
  }
  //
  // int process_crashed = 0;
  // if (!process_crashed)
  //   return;
  //
  // if (_perfect_links == NULL || _perfect_links->count == 0) {
  //   return;
  // }
  //
  // lnode_t *node = _perfect_links->start;
  // Pfd *pfd;
  // for (node = _perfect_links->start; node != NULL; node = node->next) {
  //   pfd = node->element;
  //   if (pfd->perfect_link == pl) {
  //     break;
  //   }
  // }
  //
  // if (pfd->perfect_link != pl) {
  //   return;
  // }
}

Pfd *pfd_init(int max_rank, int rank, int retransmission_period) {
  struct PerfectLink *pl = pl_init(rank, retransmission_period);
  if (pl == NULL) {
    return NULL;
  }

  pl_set_callback(pl, &pfd_callback);

  Pfd *pfd = calloc(1, sizeof(Pfd) + max_rank * sizeof(int));
  if (pfd == NULL) {
    pl_free(pl);
    return NULL;
  }

  return pfd;
}

void pfd_set_oncrash(Pfd *pfd, Crash *e) {}
