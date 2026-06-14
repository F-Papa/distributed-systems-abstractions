#ifndef AUTH_PERFECT_LINK_H
#define AUTH_PERFECT_LINK_H

#include "link/perfect_link.h"
#include "watch_set.h"
#include <sodium.h>

#define SIG_HEX_LEN (crypto_sign_BYTES * 2 + 1)

typedef struct {
  PlSend base;
} AuthPlSend;

typedef struct {
  PlDeliver base;
} AuthPlDeliver;

typedef struct AuthPerfectLink Apl;

struct AuthPerfectLink *
apl_init(int id, int base_port, int retransmission_period,
         const unsigned char private_key[crypto_sign_SECRETKEYBYTES],
         int max_rank,
         const unsigned char public_keys[][crypto_sign_PUBLICKEYBYTES]);

int apl_send(struct AuthPerfectLink *apl, AuthPlSend *e);

void apl_consume(struct AuthPerfectLink *apl, struct timeval *timeout);

void apl_set_callback(struct AuthPerfectLink *apl,
                      void (*cb)(void *, AuthPlDeliver *e), void *ctx);

void apl_free(struct AuthPerfectLink *apl);

int apl_register_fd_sets(struct AuthPerfectLink *apl, fd_set *reads,
                         fd_set *writes);

void apl_handle_fd_sets(struct AuthPerfectLink *apl, fd_set *reads,
                        fd_set *writes);

void apl_handle_timeout(struct AuthPerfectLink *apl);

wset_t *apl_get_watch_set(struct AuthPerfectLink *apl);

handler_t *apl_get_handler(struct AuthPerfectLink *apl);

#endif
