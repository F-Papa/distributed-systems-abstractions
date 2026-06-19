#ifndef BYZANTINE_LEADER_ELECTION_H
#define BYZANTINE_LEADER_ELECTION_H

#include "link/auth_perfect_link.h"
#include "orchestration/handler.h"
#include "orchestration/task.h"
#include "watch_set.h"
#include <bits/types/struct_timeval.h>
#include <sodium.h>
#include <sys/select.h>

typedef struct ByzantineLeaderElector Ble;

typedef struct byzantine_trust {
  int peer_rank;
} ByzTrust;

typedef struct byzantine_complain {
  int round;
  int peer_rank;
} ByzComplain;

typedef struct BleConfig {
  int max_faulty_peers;
  AplConfig aplConfig;
} BleConfig;

Ble *ble_init(BleConfig config);

void ble_set_on_trust_callback(Ble *ble, void (*cb)(void *, ByzTrust *),
                               void *ctx);

void ble_start(Ble *ble, struct timeval *timeout);

void ble_free(Ble *ble);

void ble_handle_timeout(Ble *ble);

wset_t *ble_get_watch_set(Ble *ble);

handler_t *ble_get_handler(Ble *ble);

task_t **ble_get_tasks(Ble *ble, int *count);

#endif
