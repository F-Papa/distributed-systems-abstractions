#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>

#define KEY_DIR "keys/"

static int write_key(const char *path, const unsigned char *key, size_t len) {
  FILE *f = fopen(path, "wb");
  if (!f) {
    perror(path);
    return -1;
  }
  size_t written = fwrite(key, 1, len, f);
  fclose(f);
  return written == len ? 0 : -1;
}

int main(int argc, char *argv[]) {
  int num_pairs = 4;
  if (argc > 1) {
    num_pairs = atoi(argv[1]);
  }

  if (sodium_init() < 0) {
    fprintf(stderr, "sodium_init() failed\n");
    return 1;
  }

  for (int i = 1; i <= num_pairs; i++) {
    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
    unsigned char sk[crypto_sign_SECRETKEYBYTES];
    crypto_sign_keypair(pk, sk);

    char path[256];
    snprintf(path, sizeof(path), KEY_DIR "private_%d.key", i);
    if (write_key(path, sk, sizeof(sk)) != 0)
      return 1;

    snprintf(path, sizeof(path), KEY_DIR "public_%d.key", i);
    if (write_key(path, pk, sizeof(pk)) != 0)
      return 1;

    printf("Generated key pair %d\n", i);
  }

  return 0;
}
