#ifndef HAVE_ASPRINTF

#include <stdio.h> //vsnprintf
#include <stdlib.h> //malloc
#include <stdarg.h> //va start et al

int
asprintf(char** str, char* fmt, ...) __attribute__ ((format (printf,2,3)));

int asprintf(char** str, char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  char one_char[1];
  int len = vsnprintf(one_char, 1, fmt, argp);

  if (len < 1) {
    fprintf(stderr, "An encoding error occurred. Setting the input pointer to NULL.\n");
    *str = NULL;
    return len;
  }

  va_end(argp);

  str = malloc(len+1);
  if (!str) {
    fprintf(stderr, "Couldn’t allocate %i bytes.\n", len + 1);
  }

  return len;
}

#endif
