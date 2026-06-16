#include "utils/list.h"

list_t *list_init() {
  list_t *l = calloc(1, sizeof(list_t));
  return l;
}

void list_free(list_t *l, void (*destructor)(void *)) {

  lnode_t *node = l->start;
  while (node) {
    lnode_t *tmp = node;
    node = node->next;
    if (destructor) {
      destructor(tmp->element);
    } else {
      free(tmp->element);
    }
    free(tmp);
  }
  free(l);
}

void list_add(list_t *l, void *elem) {
  lnode_t *new = calloc(1, sizeof(lnode_t));
  if (l->count == 0) {
    l->start = new;
  } else {
    lnode_t *n = l->start;
    for (size_t i = 0; i < l->count - 1; i++) {
      n = n->next;
    }
    n->next = new;
  }

  new->element = elem;
  l->count++;
}

int list_remove(list_t *l, size_t idx) {
  if (idx >= l->count) {
    printf("Remove out of bounds (%ld vs %ld)\n", l->count, idx);
    return -1;
  }

  lnode_t *prev = l->start;
  if (idx == 0) {
    lnode_t *to_remove = l->start;
    l->start = to_remove->next;
    free(to_remove);
  } else {
    for (size_t i = 0; i < idx - 1; i++) {
      prev = prev->next;
    }
    lnode_t *to_remove = prev->next;
    prev->next = to_remove->next;
    free(to_remove);
  }

  l->count--;
  return 0;
}

int list_index(list_t *l, void *elem) {
  if (l->count == 0) {
    return -1;
  }

  lnode_t *n = l->start;
  if (n->element == elem) {
    return 0;
  }

  for (size_t i = 1; i < l->count; i++) {
    n = n->next;
    if (n->element == elem) {
      return i;
    }
  }

  return -1;
}

void *list_get(list_t *l, size_t idx) {
  if (idx >= l->count) {
    printf("Get out of bounds (%ld vs %ld)\n", l->count, idx);
    return NULL;
  }

  lnode_t *node = l->start;
  for (size_t i = 0; i < idx; i++) {
    node = node->next;
  }
  return node->element;
}
