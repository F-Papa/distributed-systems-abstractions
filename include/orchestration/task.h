#ifndef TASK_H
#define TASK_H

#include <bits/types/struct_timeval.h>

typedef struct Task task_t;

task_t *task_new(void (*callback)(void *), void *context, struct timeval delay,
                 int is_recurrent);

#endif
