#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MAX_MSG_LEN 1024
#define MAX_CALLBACKS 10
#define FLL_SEND 1
#define FLL_DELIVER 1

typedef struct {
  int recipient;
  char msg[MAX_MSG_LEN];
} FllSend;

typedef struct {
  int sender;
  char msg[MAX_MSG_LEN];
} FllDeliver;

struct FairLossLink {
  int id;
  int socket;
  fd_set reads;
  void (*callback)(struct FairLossLink *, FllDeliver *);
  // void (*callback[MAX_CALLBACKS])(FllDeliver *);
};

struct FairLossLink *fll_init(int id);
int fll_send(struct FairLossLink *fll, FllSend *e);
void fll_set_callback(struct FairLossLink *fll,
                      void (*cb)(struct FairLossLink *fll, FllDeliver *e));
void fll_consume(struct FairLossLink *fll, struct timeval *timeout);
void fll_free(struct FairLossLink *fll);
