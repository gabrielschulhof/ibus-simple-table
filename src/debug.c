/*
 * Simple IBus Table
 * Copyright (C) 2016  Gabriel Schulhof <gabrielschulhof@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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
