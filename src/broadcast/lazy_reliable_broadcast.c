#include "broadcast/reliable_broadcast.h"
#include "constants.h"
#include "failure_detector/perfect_failure_detector.h"
#include "link/perfect_link.h"
#include "utils/list.h"
#include "utils/parsing.h"
#include "utils/timeout.h"
#include <iso646.h>
#include <string.h>
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

int rb_register_fd_sets(Rb *rb, fd_set *reads, fd_set *writes) {
  int max_pl_fd = pl_register_fd_sets(rb->perfect_link, reads, writes);
  int max_pfd_fd =
      pfd_register_fd_sets(rb->perfect_failure_detector, reads, writes);

  return max_pl_fd > max_pfd_fd ? max_pl_fd : max_pfd_fd;
}

void rb_handle_fd_sets(Rb *rb, fd_set *reads, fd_set *writes) {
  pl_handle_fd_sets(rb->perfect_link, reads, writes);
  pfd_handle_fd_sets(rb->perfect_failure_detector, reads, writes);
}

void rb_handle_timeout(Rb *rb) {
  struct timeval now;
  gettimeofday(&now, NULL);

  if (timercmp(&now, &rb->control_deadline, >=)) {
    pfd_handle_timeout(rb->perfect_failure_detector);
    reset_control_deadline(rb);
  }
  if (timercmp(&now, &rb->data_deadline, >=)) {
    pl_handle_timeout(rb->perfect_link);
    reset_data_deadline(rb);
  }
}

void rb_consume(Rb *rb, struct timeval *external_timeout) {
  struct timeval external_deadline = {0, 0};

  if (rb->control_deadline.tv_sec == 0 && rb->control_deadline.tv_usec == 0) {
    reset_control_deadline(rb);
  }

  if (rb->data_deadline.tv_sec == 0 && rb->data_deadline.tv_usec == 0) {
    reset_data_deadline(rb);
  }

  if (external_timeout) {
    gettimeofday(&external_deadline, NULL);
    timeradd(&external_deadline, external_timeout, &external_deadline);
  }

  fd_set reads, writes;
  FD_ZERO(&reads);
  FD_ZERO(&writes);
  int max_fd = rb_register_fd_sets(rb, &reads, &writes);

  while (1) {
    struct timeval now;
    gettimeofday(&now, NULL);

    if (external_timeout && timercmp(&now, &external_deadline, >=)) {
      break;
    }

    struct timeval pfd_timeout, pl_timeout;
    timersub(&rb->control_deadline, &now, &pfd_timeout);
    timersub(&rb->data_deadline, &now, &pl_timeout);

    struct timeval *next_timeout = tv_min(&pfd_timeout, &pl_timeout);

    if (external_timeout) {
      next_timeout = tv_min(next_timeout, external_timeout);
    }

    fd_set reads_copy = reads, writes_copy = writes;

    int fds_set =
        select(max_fd + 1, &reads_copy, &writes_copy, NULL, next_timeout);

    if (fds_set > 0) {
      pfd_handle_fd_sets(rb->perfect_failure_detector, &reads_copy,
                         &writes_copy);
      pl_handle_fd_sets(rb->perfect_link, &reads_copy, &writes_copy);
    } else {
      rb_handle_timeout(rb);
    }
  }
}

static int rb_broadcast_aux(Rb *rb, char *content, char *original_id,
                            int original_sender) {
  PlSend msg;
  snprintf(msg.base.msg, MAX_MSG_LEN, "BC,%d,%s,%s", original_sender,
           original_id, content);

  for (int peer_rank = 1; peer_rank <= rb->config.max_rank; peer_rank++) {
    msg.base.recipient = peer_rank;
    int status = pl_send(rb->perfect_link, &msg);
    if (status != 0)
      return status;
  }

  return 0;
}

void on_crashed(void *ctx, Crash *e) {
  Rb *rb = ctx;
  list_t *peer_history = rb->history[e->peer_id - 1];
  for (int i = 0; i < peer_history->count; i++) {
    struct SavedMessage *msg = list_get(peer_history, i);
    rb_broadcast_aux(rb, msg->msg, msg->id, e->peer_id);
  }
}

void on_delivery_wrapper(void *ctx, PlDeliver *e) {
  Rb *rb = ctx;
  char buf[MAX_MSG_LEN];

  if (try_parse_message(e->base.msg, "BC", buf, MAX_MSG_LEN) == 0) {
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

    // Parse field 2: original message ID (or "NULL")
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
      rb->cb(rb->ctx, &delivery);
      return;
    }

    list_t *history_from_peer = rb->history[parsed_og_sender - 1];
    for (int i = 0; i < history_from_peer->count; i++) {
      struct SavedMessage *saved_msg = list_get(history_from_peer, i);
      if (strcmp(saved_msg->id, e->id) == 0) {
        return;
      }
    }

    struct SavedMessage *msg_to_save = calloc(1, sizeof(struct SavedMessage));
    if (msg_to_save == NULL) {
      return;
    }
    strncpy(msg_to_save->msg, delivery.msg, MAX_MSG_LEN);
    if (strcmp("NULL", original_msg_id) == 0) {
      strncpy(msg_to_save->id, e->id, UUID_STR_LEN);
    } else {
      strncpy(msg_to_save->id, original_msg_id, UUID_STR_LEN);
    }
    list_add(history_from_peer, msg_to_save);

    rb->cb(rb->ctx, &delivery);
  } else {
    printf("UNKNOW MESSAGE FROM %d: %s\n", e->base.sender, e->base.msg);
  }
}

int rb_broadcast(Rb *rb, RbSend *e) {
  return rb_broadcast_aux(rb, e->msg, "NULL", rb->config.local_rank);
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
        list_free(peer_history[i - 1]);
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
      list_free(peer_history[peer_rank - 1]);
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
    list_free(rb->history[peer_rank - 1]);
  }
}

// WatchSet.Union?
wset_t *rb_get_watch_set(Rb *rb) {}

// Handler.Union?
handler_t *rb_get_handler(Rb *rb);
