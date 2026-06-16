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

static int send_with_headers(Urb *urb, char *content, char *original_id,
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

// TODO: implement
static void urb_callback(void *ctx, BebDelivery *e) {
  debug("Calling URB callback\n");
  Urb *urb = ctx;
  RbDelivery delivery;
  char buf[MAX_MSG_LEN];
  debug("Msg from (%d): %s\n", e->sender, e->msg);

  if (try_parse_message(e->msg, "BC", buf, MAX_MSG_LEN) == 0) {
    // Parse field 1: original sender (rank string)
    int sender_len = strcspn(buf, ",");
    char sender_str[12];
    strncpy(sender_str, buf, sender_len);
    sender_str[sender_len] = '\0';

    debug("Original Sender: %s\n", sender_str);

    int parsed_og_sender = atoi(sender_str);
    if (parsed_og_sender < 1 || parsed_og_sender > urb->config.max_rank) {
      debug("Invalid sender\n", sender_str);
      return;
    }

    // Parse field 2: original message ID (or "NULL")
    char original_msg_id[UUID_STR_LEN];
    char *id_start = buf + sender_len + DELIM_LEN;
    int id_len = strcspn(id_start, ",");
    strncpy(original_msg_id, id_start, id_len);
    original_msg_id[id_len] = '\0';

    debug("Original Id: %s\n", original_msg_id);

    // Parse field 3: message content
    char *content_start = id_start + id_len + DELIM_LEN;
    strncpy(delivery.msg, content_start, MAX_MSG_LEN);
    delivery.sender = parsed_og_sender;

    debug("Content: %s\n", delivery.msg);

    struct SavedMessage *prev_saved_msg = NULL;
    for (int i = 0; i < urb->received->count; i++) {
      struct SavedMessage *msg = list_get(urb->received, i);
      debug("Previously saved message (%d/%d): %s\n", i + 1,
            urb->received->count, msg->id);
      if (strcmp(msg->id, original_msg_id) == 0) {
        prev_saved_msg = msg;
        break;
      }
    }

    if (prev_saved_msg == NULL) {
      debug("%s) wasn't already received.\n", original_msg_id);
      int alloc_size =
          sizeof(struct SavedMessage) + sizeof(int) * urb->config.max_rank;
      prev_saved_msg = calloc(1, alloc_size);
      strncpy(prev_saved_msg->id, original_msg_id, UUID_STR_LEN);
      list_add(urb->received, prev_saved_msg);
      debug("%s) Added to list.\n", original_msg_id);

      if (parsed_og_sender != urb->config.local_rank) {
        send_with_headers(urb, delivery.msg, original_msg_id, parsed_og_sender);
        debug("%s) Acknowledged.\n", original_msg_id);
      } else {
        debug("%s) Ignored (sent by self).\n", original_msg_id);
      }
    } else {
      debug("%s) already received.\n", original_msg_id);
    }

    prev_saved_msg->acks[e->sender - 1] = 1;

    if (!prev_saved_msg->was_delivered) {
      int acks_received = 0;
      for (int i = 0; i < urb->config.max_rank; i++) {
        if (prev_saved_msg->acks[i])
          acks_received++;
      }

      int quorum_size = urb->config.max_rank / 2 + 1;
      debug("%s) Already delivered. Acks: %d/%d\n", original_msg_id,
            acks_received, quorum_size);
      if (acks_received >= quorum_size) {
        urb->callback(urb, &delivery);
        prev_saved_msg->was_delivered = 1;
      }
    } else {
      debug("%s) Already delivered.\n", original_msg_id);
    }
  } else {
    printf("UNKNOW MESSAGE FROM %d: %s\n", e->sender, e->msg);
  }
  debug("URB Callback Returned\n");
}

// TODO: implement
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

// TODO: implement
int urb_broadcast(Urb *urb, RbSend *e) {
  return send_with_headers(urb, e->msg, NULL, urb->config.local_rank);
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
