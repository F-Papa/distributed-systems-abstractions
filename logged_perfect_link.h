#include "list.h"
#include "stubborn_link.h"

typedef struct {
  SblSend base;
  long id;
} LplSend;

typedef struct {
  SblDeliver base;
  long id;
} LplDeliver;

struct LoggedPerfectLink;

struct LoggedPerfectLink *lpl_init(int id, int retransmission_period);

void get_deliveries(LplDeliver **deliveries, size_t *len);

int lpl_send(struct LoggedPerfectLink *lpl, LplSend *e);

void lpl_consume(struct LoggedPerfectLink *lpl, struct timeval *timeout);

void lpl_set_callback(struct LoggedPerfectLink *lpl,
                      void (*cb)(void *ctx, const list_t *deliveries),
                      void *ctx);

void lpl_free(struct LoggedPerfectLink *lpl);
