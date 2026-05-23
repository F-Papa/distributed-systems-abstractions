#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct ListNode {
  void *element;
  struct ListNode *next;
} lnode_t;

typedef struct List {
  lnode_t *start;
  size_t count;
} list_t;

list_t *list_init();

void list_free(list_t *l);

void list_add(list_t *l, void *elem);

int list_remove(list_t *l, size_t idx);

void *list_get(list_t *l, size_t idx);

int list_index(list_t *l, void *elem);
