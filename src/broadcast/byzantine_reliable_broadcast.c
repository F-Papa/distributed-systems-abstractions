#include "broadcast/byzantine_reliable_broadcast.h"
#include "constants.h"
#include "link/auth_perfect_link.h"
#include "orchestration/handler.h"
#include "utils/list.h"
#include "utils/parsing.h"
#include "watch_set.h"
#include <string.h>

const char *SEND_CODE = "SE";
const char *ECHO_CODE = "EC";
const char *READY_CODE = "RY";

struct ByzantineReliableBroadcast {
  int local_rank;
  int max_rank;
  int sender_rank;
  void (*cb)(void *, Deliver *);
  void *ctx;
  int sent_echo;
  int sent_ready;
  int delivered;
  int max_faulty_nodes;
  Apl *auth_perfect_link;
  list_t *readys;
  list_t *echos;
};

typedef struct ReadyEntry {
  int peer_rank;
  char msg[MAX_MSG_LEN];
} ReadyEntry;

typedef ReadyEntry EchoEntry;

int broadcast_aux(Brb *brb, const char *msg, const char *code) {
  for (int peer_rank = 1; peer_rank <= brb->max_rank; peer_rank++) {
    Send send = {.recipient = peer_rank};
    snprintf(send.msg, MAX_MSG_LEN, "%s,%s", code, msg);
    if (apl_send(brb->auth_perfect_link, &send) != 0) {
      return -1;
    }
  }
  return 0;
}

static void handle_send(Brb *brb, int sender, const char *body) {
  if (brb->sent_echo || sender != brb->sender_rank) {
    return;
  }

  brb->sent_echo = 1;
  broadcast_aux(brb, body, ECHO_CODE);
}

static int get_count(list_t *list, const char *body) {
  int count = 0;
  for (int i = 0; i < list->count; i++) {
    ReadyEntry *tmp = list_get(list, i);
    if (strcmp(tmp->msg, body) == 0)
      count++;
  }

  return count;
}

static void handle_echo(Brb *brb, int sender, const char *body) {
  for (int i = 1; i < brb->echos->count; i++) {
    EchoEntry *tmp = list_get(brb->echos, i);
    if (tmp->peer_rank == sender) {
      return;
    }
  }

  EchoEntry *echo = calloc(1, sizeof(EchoEntry));
  if (echo == NULL)
    return;

  echo->peer_rank = sender;
  strcpy(echo->msg, body);
  list_add(brb->echos, echo);

  int echo_count = get_count(brb->echos, body);
  int quorum_size = (float)(brb->max_rank + brb->max_faulty_nodes) / 2;
  if (echo_count > quorum_size && !brb->sent_ready) {
    brb->sent_ready = 1;
    broadcast_aux(brb, body, READY_CODE);
  }

  return;
}

static void handle_ready(Brb *brb, int sender, const char *body) {
  for (int i = 1; i < brb->readys->count; i++) {
    ReadyEntry *tmp = list_get(brb->readys, i);
    if (tmp->peer_rank == sender) {
      return;
    }
  }

  ReadyEntry *ready = calloc(1, sizeof(ReadyEntry));
  if (ready == NULL)
    return;

  ready->peer_rank = sender;
  strcpy(ready->msg, body);
  list_add(brb->readys, ready);

  int ready_count = get_count(brb->readys, body);
  if (ready_count > 2 * brb->max_faulty_nodes && !brb->delivered) {
    brb->delivered = 1;
    Deliver d = {.sender = sender};
    strncpy(d.msg, body, MAX_MSG_LEN);
    brb->cb(brb->ctx, &d);
  } else if (ready_count > brb->max_faulty_nodes && !brb->sent_ready) {
    brb->sent_ready = 1;
    broadcast_aux(brb, body, READY_CODE);
  }
}

static void brb_callback(void *ctx, Deliver *deliver) {
  Brb *brb = ctx;
  char body[MAX_MSG_LEN];

  if (try_parse_message(deliver->msg, SEND_CODE, body, MAX_MSG_LEN) == 0) {
    return handle_send(brb, deliver->sender, body);
  }
  if (try_parse_message(deliver->msg, ECHO_CODE, body, MAX_MSG_LEN) == 0) {
    return handle_echo(brb, deliver->sender, body);
  }
  if (try_parse_message(deliver->msg, READY_CODE, body, MAX_MSG_LEN) == 0) {
    return handle_ready(brb, deliver->sender, body);
  }

  printf("Invalid message received: %s\n", deliver->msg);
}

Brb *brb_init(BrbConfig config) {
  Apl *auth_perfect_link = apl_init(config.aplConfig);
  if (auth_perfect_link == NULL)
    return NULL;

  list_t *readys = list_init();
  if (readys == NULL) {
    apl_free(auth_perfect_link);
    return NULL;
  }

  list_t *echos = list_init();
  if (echos == NULL) {
    apl_free(auth_perfect_link);
    list_free(readys, NULL);
    return NULL;
  }

  Brb *brb = calloc(1, sizeof(Brb));
  if (brb == NULL) {
    apl_free(auth_perfect_link);
    list_free(readys, NULL);
    list_free(echos, NULL);
    return NULL;
  }

  apl_set_callback(auth_perfect_link, &brb_callback, brb);
  brb->readys = readys;
  brb->echos = echos;
  brb->max_faulty_nodes = config.max_faulty_nodes;
  brb->local_rank = config.aplConfig.local_rank;
  brb->max_rank = config.aplConfig.max_rank;
  brb->auth_perfect_link = auth_perfect_link;
  brb->sender_rank = config.sender_rank;
  return brb;
}

void brb_free(Brb *brb) {
  list_free(brb->readys, NULL);
  list_free(brb->echos, NULL);
  apl_free(brb->auth_perfect_link);
  free(brb);
}

int brb_broadcast(Brb *brb, Broadcast *e) {
  return broadcast_aux(brb, e->msg, SEND_CODE);
}

void brb_set_callback(Brb *brb, void (*cb)(void *, Deliver *), void *ctx) {
  brb->cb = cb;
  brb->ctx = ctx;
}

wset_t *brb_get_watch_set(Brb *brb) {
  return apl_get_watch_set(brb->auth_perfect_link);
}

handler_t *brb_get_handler(Brb *brb) {
  return apl_get_handler(brb->auth_perfect_link);
}

task_t **brb_get_tasks(Brb *brb, int *count) {
  return apl_get_tasks(brb->auth_perfect_link, count);
}
