/*
 * StreamLib: Seek within an input stream
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
#include "Reader.h"

int reader_fseek(Reader * const reader, long int offset,
  int const whence)
{
  assert(reader != NULL);

  if ((whence == SEEK_CUR) && (reader->pushed_back != EOF)) {
    /* We pushed back a character so the file position indicator is
       one character beyond where it should be. */
    --offset;
  }

  switch (whence) {
    case SEEK_CUR:
      DEBUGF("Seeking %ld bytes beyond the current position\n", offset);
      /* -offset may not be representable if -LONG_MIN > LONG_MAX */
      if ((offset < 0) && (offset < -reader->fpos)) {
        reader->error = 1;
        return -1; /* invalid offset */
      }
      if (offset != 0) {
        reader->fpos += offset;
        reader->repos = 1;
      }
      break;

    case SEEK_SET:
      DEBUGF("Seeking %ld bytes beyond the start\n", offset);
      if (offset < 0) {
        reader->error = 1;
        return -1; /* invalid offset */
      }
      if (offset != reader->fpos) {
        reader->fpos = offset;
        reader->repos = 1;
      }
      break;

    default:
      /* A binary stream need not meaningfully support SEEK_END */
      return -1;
  }

  /* Clear any end-of-file condition and undo any previous call to
     push back a character. */
  reader->pushed_back = EOF;
  reader->eof = 0;
  assert(reader->fpos >= 0);
  return 0;
}
