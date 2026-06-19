#include "broadcast/best_effort_broadcast.h"
#include "broadcast/reliable_broadcast.h"
#include "broadcast/uniform_reliable_broadcast.h"
#include "constants.h"
#include "utils/list.h"
#include "utils/logging.h"
#include "utils/parsing.h"
#include "uuid.h"
#include <iso646.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct UniformReliableBroadcast {
  UrbConfig config;
  Beb *best_effort_broadcast;
  list_t *received;
  void (*callback)(void *, RbDelivery *);
  void *context;
};

struct SavedMessage {
  char id[UUID_STR_LEN];
  int was_delivered;
  int acks[];
};

enum Fields {
  ORIGINAL_SENDER = 0,
  ORIGINAL_ID,
  BODY,
};

static int broadcast_aux(Urb *urb, char *content, char *original_id,
                         int original_sender) {
  BebSend to_send = {};
  char id_buf[UUID_STR_LEN];
  if (original_id == NULL) {
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse(uuid, id_buf);
    original_id = id_buf;
  }
  snprintf(to_send.msg, MAX_MSG_LEN, "BC,%d,%s,%s", original_sender,
           original_id, content);

  return beb_broadcast(urb->best_effort_broadcast, &to_send);
}

static RbDelivery create_rb_delivery(const char **fields) {
  RbDelivery delivery;
  int original_sender = atoi(fields[ORIGINAL_SENDER]);
  strncpy(delivery.msg, fields[BODY], MAX_MSG_LEN);
  delivery.sender = original_sender;
  return delivery;
}

static struct SavedMessage *get_saved_message(Urb *urb, const char *id) {
  for (int i = 0; i < urb->received->count; i++) {
    struct SavedMessage *msg = list_get(urb->received, i);
    debug("Previously saved message (%d/%d): %s\n", i + 1, urb->received->count,
          msg->id);
    if (strcmp(msg->id, id) == 0) {
      return msg;
    }
  }
  return NULL;
}

static int should_deliver(const Urb *urb, struct SavedMessage *prev_saved_msg) {
  int acks_received = 0;
  for (int i = 0; i < urb->config.max_rank; i++) {
    if (prev_saved_msg->acks[i])
      acks_received++;
  }
  int quorum_size = urb->config.max_rank / 2 + 1;
  if (acks_received >= quorum_size) {
    return 1;
  }
  return 0;
}

static struct SavedMessage *save_message(Urb *urb, const char **fields) {
  int alloc_size =
      sizeof(struct SavedMessage) + sizeof(int) * urb->config.max_rank;
  struct SavedMessage *msg = calloc(1, alloc_size);
  strncpy(msg->id, fields[ORIGINAL_ID], UUID_STR_LEN);
  list_add(urb->received, msg);
  return msg;
}

static void urb_callback(void *ctx, BebDelivery *e) {
  debug("Calling URB callback\n");
  Urb *urb = ctx;
  char buf[MAX_MSG_LEN];
  debug("Msg from (%d): %s\n", e->sender, e->msg);

  if (try_parse_message(e->msg, "BC", buf, MAX_MSG_LEN) != 0) {
    printf("UNKNOW MESSAGE FROM %d: %s\n", e->sender, e->msg);
    return;
  }

  char *fields[3];
  if (parse_message(buf, fields, 3) != 0)
    return;

  debug("Original Sender: %s\n", fields[ORIGINAL_SENDER]);

  int original_sender = atoi(fields[ORIGINAL_SENDER]);
  if (original_sender < 1 || original_sender > urb->config.max_rank) {
    for (int i = 0; i < 3; i++)
      free(fields[i]);
    return;
  }

  debug("Original Id: %s\n", fields[ORIGINAL_ID]);

  struct SavedMessage *prev_saved_msg =
      get_saved_message(urb, fields[ORIGINAL_ID]);

  RbDelivery delivery = create_rb_delivery((const char **)fields);
  if (prev_saved_msg == NULL) {
    prev_saved_msg = save_message(urb, (const char **)fields);
    if (original_sender != urb->config.local_rank) {
      broadcast_aux(urb, delivery.msg, fields[ORIGINAL_ID], original_sender);
    }
  }

  prev_saved_msg->acks[e->sender - 1] = 1;

  if (!prev_saved_msg->was_delivered && should_deliver(urb, prev_saved_msg)) {
    urb->callback(urb, &delivery);
    prev_saved_msg->was_delivered = 1;
  }

  for (int i = 0; i < 3; i++)
    free(fields[i]);

  debug("URB Callback Returned\n");
}

Urb *urb_init(UrbConfig config) {
  Beb *best_effort_broadcast =
      beb_init(config.local_rank, config.max_rank, config.base_port,
               config.retransmission_period);

  if (best_effort_broadcast == NULL)
    return NULL;

  list_t *received = list_init();
  if (received == NULL) {
    beb_free(best_effort_broadcast);
    return NULL;
  }

  Urb *urb = calloc(1, sizeof(Urb));
  if (urb == NULL) {
    beb_free(urb->best_effort_broadcast);
    list_free(received, NULL);
    return NULL;
  }

  urb->config = config;
  urb->received = received;
  urb->best_effort_broadcast = best_effort_broadcast;
  beb_set_callback(best_effort_broadcast, &urb_callback, urb);
  return urb;
}

int urb_broadcast(Urb *urb, RbSend *e) {
  return broadcast_aux(urb, e->msg, NULL, urb->config.local_rank);
}

void urb_set_callback(Urb *urb, void (*cb)(void *, RbDelivery *), void *ctx) {
  urb->callback = cb;
  urb->context = ctx;
}

void urb_free(Urb *urb) {
  beb_free(urb->best_effort_broadcast);
  list_free(urb->received, NULL);
  free(urb);
}

wset_t *urb_get_watch_set(Urb *urb) {
  return beb_get_watch_set(urb->best_effort_broadcast);
}

handler_t *urb_get_handler(Urb *urb) {
  return beb_get_handler(urb->best_effort_broadcast);
}

task_t **urb_get_tasks(Urb *urb, int *count) {
  return beb_get_tasks(urb->best_effort_broadcast, count);
}
