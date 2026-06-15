#include "utils/logging.h"
#include "stdarg.h"
#include "stdio.h"

// #define DEBUG DEBUG

void debug(const char *format, ...) {
#ifdef DEBUG
  printf("[DEBUG] ");
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
#else
  return;
#endif
}
