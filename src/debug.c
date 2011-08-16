#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

#ifdef DEBUG
void
debug_print(const char *fmt, ...)
{
  FILE *pfile = fopen("/tmp/my-ibus-table.log", "a");
  int do_close = 0;
  va_list va;

  if (pfile)
    do_close = 1;
  else
    pfile = stderr;

  va_start(va, fmt);
  vfprintf(pfile, fmt, va);
  va_end(va);

  if (do_close)
    fclose(pfile);
  else
    fflush(pfile);
}
#endif /* DEBUG */
