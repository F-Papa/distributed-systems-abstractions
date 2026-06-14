#ifndef HANDLER_H
#define HANDLER_H

#include <bits/types/struct_timeval.h>
#include <sys/select.h>
#include <unistd.h>

typedef struct Handler handler_t;

handler_t *handler_new(void (*callback)(fd_set *, fd_set *, void *),
                       void *context);

handler_t *handler_composite_new(handler_t **handlers, int count);

void handler_free(handler_t *handler);

#endif // !HANDLER_H
