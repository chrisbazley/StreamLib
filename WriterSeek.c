/*
 * StreamLib: Seek within an output stream
 * Copyright (C) 2019 Christopher Bazley
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* History:
  CJB: 05-Nov-19: Split into a separate compilation unit.
*/

/* ISO library header files */
#include <stdio.h>

/* Local headers */
#include "Internal/StreamMisc.h"
#include "Writer.h"

int writer_fseek(Writer * const writer, long int const offset,
  int const whence)
{
  /* It's tempting to disallow all backward seeks for simplicity but
     we cannot if we want to allow the Gordon Key compressed file writer
     (which prepends a final size) to piggyback on other writers. */
  assert(writer != NULL);
  assert(writer->fpos >= 0);

  switch (whence) {
    case SEEK_CUR:
      DEBUGF("Seeking %ld bytes beyond the current position\n", offset);
      /* -offset may not be representable if -LONG_MIN > LONG_MAX */
      if ((offset < 0) && (offset < -writer->fpos)) {
        writer->error = 1;
        return -1; /* not supported */
      }
      if (offset != 0) {
        writer->fpos += offset;
        writer->repos = 1;
      }
      break;

    case SEEK_SET:
      DEBUGF("Seeking %ld bytes beyond the start\n", offset);
      if (offset < 0) {
        writer->error = 1;
        return -1; /* invalid offset */
      }
      if (offset != writer->fpos) {
        writer->fpos = offset;
        writer->repos = 1;
      }
      break;

    default:
      /* A binary stream need not meaningfully support SEEK_END */
      return -1;
  }
  assert(writer->fpos >= 0);
  return 0;
}
