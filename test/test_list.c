#include "utils/list.h"

int main() {
  list_t *l = list_init();
  int uno = 1, dos = 2, tres = 3;
  list_add(l, &uno);
  list_add(l, &dos);
  list_add(l, &tres);

  printf("%d\n", list_index(l, &uno));
  printf("%d\n", list_index(l, &dos));
  printf("%d\n", list_index(l, &tres));
  printf("%d\n", list_index(l, NULL));
}
