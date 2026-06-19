#ifndef LINK_H
#define LINK_H

#include "constants.h"

typedef struct Send {
  char msg[MAX_MSG_LEN];
  int recipient;
} Send;

typedef struct Deliver {
  char msg[MAX_MSG_LEN];
  int sender;
} Deliver;

#endif // !
