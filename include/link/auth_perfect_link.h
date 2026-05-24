#include "link/perfect_link.h"
#include <sodium.h>

#define SIG_HEX_LEN (crypto_sign_BYTES * 2 + 1)

typedef struct {
  PlSend base;
} AuthPlSend;

typedef struct {
  PlDeliver base;
} AuthPlDeliver;

struct AuthPerfectLink;

struct AuthPerfectLink *apl_init(int id, int retransmission_period,
                                 const unsigned char private_key[crypto_sign_SECRETKEYBYTES],
                                 int max_rank,
                                 const unsigned char public_keys[][crypto_sign_PUBLICKEYBYTES]);

int apl_send(struct AuthPerfectLink *apl, AuthPlSend *e);

void apl_consume(struct AuthPerfectLink *apl, struct timeval *timeout);

void apl_set_callback(struct AuthPerfectLink *apl,
                      void (*cb)(void *, AuthPlDeliver *e), void *ctx);

void apl_free(struct AuthPerfectLink *apl);