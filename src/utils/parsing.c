#include "string.h"

#define DELIM_LEN 1

int try_parse_message(char *msg, char *expected_header, char *body, int len) {
  if (strpbrk(msg, expected_header) != msg) {
    return -1;
  }
  strncpy(body, msg + strlen(expected_header) + DELIM_LEN, len);
  return 0;
}
