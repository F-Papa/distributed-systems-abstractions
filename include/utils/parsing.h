#ifndef PARSING_H
#define PARSING_H

int try_parse_message(const char *msg, const char *expected_header, char *body,
                      int len);

int parse_message(char *msg, char **fields, int expected_fields);

#endif
