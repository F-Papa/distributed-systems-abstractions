#include "broadcast/reliable_broadcast.h"
#include "constants.h"
#include "failure_detector/perfect_failure_detector.h"
#include "link/perfect_link.h"
#include "orchestration/handler.h"
#include "orchestration/orchestrator.h"
#include "utils/list.h"
#include "utils/logging.h"
#include "utils/parsing.h"
#include "utils/timeout.h"
#include "watch_set.h"
#include <iso646.h>
#include <string.h>
#include <uuid.h>
#include <sys/select.h>
#include <sys/time.h>

struct ReliableBroadcast {
  RbConfig config;
  struct PerfectLink *perfect_link;
  Pfd *perfect_failure_detector;
  void (*cb)(void *, RbDelivery *);
  void *ctx;
  struct timeval control_deadline;
  struct timeval data_deadline;
  list_t *history[];
};

struct SavedMessage {
  char id[UUID_STR_LEN];
  char msg[MAX_MSG_LEN];
};

static void reset_control_deadline(Rb *rb) {
  struct timeval to = {0, 0};
  to.tv_sec = rb->config.control_retransmission_period;
  tv_reset_deadline(&rb->control_deadline, &to);
}
static void reset_data_deadline(Rb *rb) {
  struct timeval to = {0, 0};
  to.tv_sec = rb->config.data_retransmission_period;
  tv_reset_deadline(&rb->data_deadline, &to);
}

void rb_start(Rb *rb) {
  reset_control_deadline(rb);
  reset_data_deadline(rb);
}

void rb_consume(Rb *rb, struct timeval *external_timeout) {
  orch_t *orch = orchestrator_new();

  wset_t *ws = rb_get_watch_set(rb);
  orchestrator_register_watch_set(orch, ws);
  watch_set_free(ws);

  handler_t *h = rb_get_handler(rb);
  orchestrator_add_handler(orch, h);

  struct timeval control_period = {
      .tv_sec = rb->config.control_retransmission_period, .tv_usec = 0};
  task_t *ctrl = task_new((void *)&pfd_handle_timeout,
                          rb->perfect_failure_detector, control_period, 1);
  orchestrator_add_task(orch, ctrl);

  struct timeval data_period = {.tv_sec = rb->config.data_retransmission_period,
                                .tv_usec = 0};
  task_t *data =
      task_new((void *)&pl_handle_timeout, rb->perfect_link, data_period, 1);
  orchestrator_add_task(orch, data);

  orchestrator_start(orch, external_timeout);
}

static int rb_broadcast_aux(Rb *rb, char *content, char *original_id,
                            int original_sender) {

  char my_id[UUID_STR_LEN];
  if (original_id == NULL) {
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse(uuid, my_id);
    original_id = my_id;
  }

  PlSend msg;
  snprintf(msg.msg, MAX_MSG_LEN, "BC,%d,%s,%s", original_sender, original_id,
           content);

  for (int peer_rank = 1; peer_rank <= rb->config.max_rank; peer_rank++) {
    msg.recipient = peer_rank;
    int status = pl_send(rb->perfect_link, &msg);
    if (status != 0)
      return status;
  }

  return 0;
}

void on_crashed(void *ctx, Crash *e) {
  printf("\t\tPeer %d crashed. Retransmitting messages...\n", e->peer_id);
  Rb *rb = ctx;

  list_t *peer_history = rb->history[e->peer_id - 1];
  for (int i = 0; i < peer_history->count; i++) {
    struct SavedMessage *msg = list_get(peer_history, i);
    printf("Retransmitting: %s\n", msg->msg);
    rb_broadcast_aux(rb, msg->msg, msg->id, e->peer_id);
  }
  printf("Done retransmitting %ld messages from %d\n", peer_history->count,
         e->peer_id);
}

void on_delivery_wrapper(void *ctx, PlDeliver *e) {
  Rb *rb = ctx;
  char buf[MAX_MSG_LEN];

  debug("PlDeliver from %d: %s\n", e->sender, e->msg);

  if (try_parse_message(e->msg, "BC", buf, MAX_MSG_LEN) == 0) {
    RbDelivery delivery;

    // Parse field 1: original sender (rank string)
    int sender_len = strcspn(buf, ",");
    char sender_str[12];
    strncpy(sender_str, buf, sender_len);
    sender_str[sender_len] = '\0';

    int parsed_og_sender = atoi(sender_str);
    if (parsed_og_sender < 1 || parsed_og_sender > rb->config.max_rank) {
      return;
    }

    // Parse field 2: original message ID
    char original_msg_id[UUID_STR_LEN];
    char *id_start = buf + sender_len + DELIM_LEN;
    int id_len = strcspn(id_start, ",");
    strncpy(original_msg_id, id_start, id_len);
    original_msg_id[id_len] = '\0';

    // Parse field 3: message content
    char *content_start = id_start + id_len + DELIM_LEN;
    strncpy(delivery.msg, content_start, MAX_MSG_LEN);
    delivery.sender = parsed_og_sender;

    if (parsed_og_sender == rb->config.local_rank) {
      if (e->sender == rb->config.local_rank) {
        rb->cb(rb->ctx, &delivery);
      }
      return;
    }

    list_t *history_from_peer = rb->history[parsed_og_sender - 1];
    debug("Checking history from og_sender (%d):\n", parsed_og_sender);
    for (int i = 0; i < history_from_peer->count; i++) {
      struct SavedMessage *saved_msg = list_get(history_from_peer, i);
      debug("\tog_id: %s | saved_id: %s\n", original_msg_id,
            saved_msg->id);
      if (strcmp(saved_msg->id, original_msg_id) == 0) {
        printf("Dup message: %s\n", e->msg);
        return;
      }
    }

    struct SavedMessage *msg_to_save = calloc(1, sizeof(struct SavedMessage));
    if (msg_to_save == NULL) {
      return;
    }
    strncpy(msg_to_save->msg, delivery.msg, MAX_MSG_LEN);
    strncpy(msg_to_save->id, original_msg_id, UUID_STR_LEN);
    list_add(history_from_peer, msg_to_save);

    rb->cb(rb->ctx, &delivery);
  } else {
    printf("UNKNOW MESSAGE FROM %d: %s\n", e->sender, e->msg);
  }
}

int rb_broadcast(Rb *rb, RbSend *e) {
  return rb_broadcast_aux(rb, e->msg, NULL, rb->config.local_rank);
}

Rb *rb_init(RbConfig config) {
  struct PerfectLink *pl = pl_init(config.local_rank, config.data_base_port,
                                   config.data_retransmission_period);
  if (pl == NULL) {
    return NULL;
  }

  Pfd *pfd =
      pfd_init(config.local_rank, config.max_rank, config.control_base_port,
               config.control_retransmission_period);

  if (pfd == NULL) {
    pl_free(pl);
    return NULL;
  }

  list_t *peer_history[config.max_rank];

  for (int peer_rank = 1; peer_rank <= config.max_rank; peer_rank++) {
    if (peer_rank == config.local_rank)
      continue;

    list_t *history = list_init();

    if (history == NULL) {
      pl_free(pl);
      pfd_free(pfd);

      for (int i = 1; i < peer_rank; i++) {
        if (i == config.local_rank)
          continue;
        list_free(peer_history[i - 1], NULL);
      }

      return NULL;
    }
    peer_history[peer_rank - 1] = history;
  }

  Rb *rb = calloc(1, sizeof(Rb) + sizeof(list_t *) * config.max_rank);

  if (rb == NULL) {
    pl_free(pl);
    pfd_free(pfd);
    for (int peer_rank = 1; peer_rank <= config.max_rank; peer_rank++) {
      if (peer_rank == config.local_rank)
        continue;
      list_free(peer_history[peer_rank - 1], NULL);
    }
    return NULL;
  }

  for (int i = 0; i < config.max_rank; i++) {
    rb->history[i] = peer_history[i];
  }

  pl_set_callback(pl, &on_delivery_wrapper, rb);
  pfd_set_oncrash(pfd, &on_crashed, rb);

  rb->config = config;
  rb->perfect_link = pl;
  rb->perfect_failure_detector = pfd;

  return rb;
}

void rb_set_callback(Rb *rb, void (*cb)(void *, RbDelivery *), void *ctx) {
  rb->cb = cb;
  rb->ctx = ctx;
}

void rb_free(Rb *rb) {
  pl_free(rb->perfect_link);
  pfd_free(rb->perfect_failure_detector);
  for (int peer_rank = 1; peer_rank <= rb->config.max_rank; peer_rank++) {
    if (peer_rank == rb->config.local_rank)
      continue;
    list_free(rb->history[peer_rank - 1], NULL);
  }
}

wset_t *rb_get_watch_set(Rb *rb) {
  wset_t *pfd_ws = pfd_get_watch_set(rb->perfect_failure_detector);
  wset_t *pl_ws = pl_get_watch_set(rb->perfect_link);
  wset_t *rb_ws = watch_set_union(pfd_ws, pl_ws);
  watch_set_free(pfd_ws);
  watch_set_free(pl_ws);
  return rb_ws;
}

handler_t *rb_get_handler(Rb *rb) {
  handler_t *handlers[] = {
      pl_get_handler(rb->perfect_link),
      pfd_get_handler(rb->perfect_failure_detector),
  };
  handler_t *rb_handler = handler_composite_new(handlers, 2);

  handler_free(handlers[0]);
  handler_free(handlers[1]);

  return rb_handler;
}

task_t **rb_get_tasks(Rb *rb, int *count) {
  int c1, c2;
  task_t **t1 = pl_get_tasks(rb->perfect_link, &c1);
  task_t **t2 = pfd_get_tasks(rb->perfect_failure_detector, &c2);
  task_t **merged = calloc(c1 + c2, sizeof(task_t *));
  if (merged == NULL) {
    for (int i = 0; i < c1; i++)
      task_free(t1[i]);
    for (int i = 0; i < c2; i++)
      task_free(t2[i]);
    free(t1);
    free(t2);
    *count = 0;
    return NULL;
  }
  memcpy(merged, t1, c1 * sizeof(task_t *));
  memcpy(merged + c1, t2, c2 * sizeof(task_t *));
  free(t1);
  free(t2);
  *count = c1 + c2;
  return merged;
}
