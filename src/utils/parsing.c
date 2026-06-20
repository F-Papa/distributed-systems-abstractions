#include "constants.h"
#include <stdlib.h>
#include <string.h>

int try_parse_message(char *msg, char *expected_header, char *body, int len) {
  int i = 0;
  while (expected_header != NULL && expected_header[i] != '\0') {
    if (msg == NULL || msg[i] != expected_header[i]) {
      return -1;
    }
    i++;
  }
  strncpy(body, msg + strlen(expected_header) + DELIM_LEN, len);
  return 0;
}

int parse_message(char *msg, char **fields, int expected_fields) {
  char DELIMITER = ',';
  memset(fields, 0, expected_fields * sizeof(char *));

  char *_ptr = msg;
  for (int i = 0; i < expected_fields - 1; i++) {
    char *delim_ptr = strchr(_ptr, DELIMITER);
    if (delim_ptr == NULL) {
      for (int j = 0; j < i; j++)
        free(fields[j]);

      return -1;
    }

    int field_len = delim_ptr - _ptr + 1;
    fields[i] = calloc(field_len, sizeof(char));
    strncpy(fields[i], _ptr, field_len);
    fields[i][field_len - 1] = '\0';
    _ptr = delim_ptr + 1;
  }

  fields[expected_fields - 1] = calloc(MAX_MSG_LEN, sizeof(char));
  strncpy(fields[expected_fields - 1], _ptr, MAX_MSG_LEN);
  return 0;
}
