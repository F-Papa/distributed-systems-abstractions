#include "link/auth_perfect_link.h"
#include "constants.h"
#include "link/perfect_link.h"
#include "orchestration/handler.h"
#include "utils/logging.h"
#include <stdlib.h>
#include <string.h>

struct AuthPerfectLink {
  struct PerfectLink *perfect_link;
  unsigned char private_key[crypto_sign_SECRETKEYBYTES];
  unsigned char (*public_keys)[crypto_sign_PUBLICKEYBYTES];
  int max_rank;
  void (*cb)(void *, AuthPlDeliver *);
  void *ctx;
};

static void wrapper(void *ctx, PlDeliver *e) {
  struct AuthPerfectLink *apl = ctx;

  int sig_hex_len = strcspn(e->base.msg, ",");
  if (sig_hex_len == 0 || sig_hex_len >= (int)sizeof(e->base.msg))
    return;

  int sender = e->base.sender;
  if (sender < 1 || sender > apl->max_rank) {
    debug("AuthPL: unknown sender %d\n", sender);
    return;
  }

  unsigned char sig[crypto_sign_BYTES];
  size_t sig_len = 0;
  if (sodium_hex2bin(sig, sizeof(sig), e->base.msg, sig_hex_len, NULL, &sig_len,
                     NULL) != 0)
    return;

  int offset = sig_hex_len + DELIM_LEN;
  if (offset >= (int)strlen(e->base.msg))
    return;

  if (crypto_sign_verify_detached(
          sig, (unsigned char *)e->base.msg + offset,
          (unsigned long long)(strlen(e->base.msg) - offset),
          apl->public_keys[sender]) != 0) {
    debug("AuthPL: invalid signature from %d\n", sender);
    return;
  }

  size_t msg_len = strlen(e->base.msg) - offset + 1;
  memmove(e->base.msg, e->base.msg + offset, msg_len);

  AuthPlDeliver apl_event = {.base = *e};
  apl->cb(apl->ctx, &apl_event);
}

struct AuthPerfectLink *
apl_init(int id, int base_port, int retransmission_period,
         const unsigned char private_key[crypto_sign_SECRETKEYBYTES],
         int max_rank,
         const unsigned char public_keys[][crypto_sign_PUBLICKEYBYTES]) {
  struct PerfectLink *pl = pl_init(id, base_port, retransmission_period);
  if (pl == NULL)
    return NULL;

  struct AuthPerfectLink *apl = calloc(1, sizeof(struct AuthPerfectLink));
  if (apl == NULL) {
    pl_free(pl);
    return NULL;
  }

  unsigned char (*pk_copy)[crypto_sign_PUBLICKEYBYTES] =
      calloc(max_rank + 1, sizeof(*pk_copy));
  if (pk_copy == NULL) {
    pl_free(pl);
    free(apl);
    return NULL;
  }

  memcpy(apl->private_key, private_key, crypto_sign_SECRETKEYBYTES);
  for (int i = 1; i <= max_rank; i++)
    memcpy(pk_copy[i], public_keys[i], crypto_sign_PUBLICKEYBYTES);

  apl->public_keys = pk_copy;
  apl->max_rank = max_rank;
  apl->perfect_link = pl;
  apl->cb = NULL;
  apl->ctx = NULL;

  pl_set_callback(pl, &wrapper, apl);

  return apl;
}

int apl_send(struct AuthPerfectLink *apl, AuthPlSend *e) {
  unsigned char sig[crypto_sign_BYTES];
  unsigned long long sig_len = 0;
  crypto_sign_detached(sig, &sig_len, (unsigned char *)e->base.base.msg,
                       (unsigned long long)strlen(e->base.base.msg),
                       apl->private_key);

  char sig_hex[SIG_HEX_LEN];
  sodium_bin2hex(sig_hex, sizeof(sig_hex), sig, sizeof(sig));

  char buf[MAX_MSG_LEN];
  snprintf(buf, sizeof(buf), "%s,%s", sig_hex, e->base.base.msg);
  strcpy(e->base.base.msg, buf);

  return pl_send(apl->perfect_link, &e->base);
}

void apl_consume(struct AuthPerfectLink *apl, struct timeval *timeout) {
  return pl_consume(apl->perfect_link, timeout);
}

void apl_set_callback(struct AuthPerfectLink *apl,
                      void (*cb)(void *, AuthPlDeliver *), void *ctx) {
  apl->cb = cb;
  apl->ctx = ctx;
}

void apl_free(struct AuthPerfectLink *apl) {
  pl_free(apl->perfect_link);
  free(apl->public_keys);
  free(apl);
}

void apl_handle_timeout(struct AuthPerfectLink *apl) {
  pl_handle_timeout(apl->perfect_link);
}

wset_t *apl_get_watch_set(struct AuthPerfectLink *apl) {
  return pl_get_watch_set(apl->perfect_link);
}

handler_t *apl_get_handler(struct AuthPerfectLink *apl) {
  return pl_get_handler(apl->perfect_link);
}
