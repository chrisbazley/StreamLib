/*
 * StreamLib: Flex memory buffer reader
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
  CJB: 24-Aug-19: Created this source file.
  CJB: 07-Sep-19: First released version.
  CJB: 10-Nov-19: Allow reading from a null anchor (treated as a stream of
                  length 0).
  CJB: 21-Nov-20: Improved debug output on seek failure.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

/* Acorn C/C++ header files */
#include "flex.h"

/* Local headers */
#include "ReaderFlex.h"
#include "Internal/StreamMisc.h"

static int buffer_size(flex_ptr const anchor)
{
  assert(anchor != NULL);
  return *anchor ? flex_size(anchor) : 0;
}

static size_t reader_flex_fread(void *ptr, size_t const size,
                                Reader * const reader)
{
  assert(ptr != NULL);
  assert(reader != NULL);
  flex_ptr const anchor = reader->data;
  assert(anchor != NULL);
  assert(reader->fpos >= 0);

  int const fsize = buffer_size(anchor);
  if (reader->fpos > fsize) {
    DEBUGF("Can't seek %ld (beyond end of flex at %d)\n", reader->fpos, fsize);
    reader->error = 1;
    return 0;
  }

  assert(fsize >= reader->fpos);
  size_t const avail = fsize - (int)reader->fpos;

  /* We can't read past the end of the buffer. This check should
     also make the conversion of bytes-to-copy from long int to
     size_t safe. */
  size_t nread = size;
  if (avail < nread) {
    DEBUGF("set eof\n");
    reader->eof = 1;
    nread = avail;
  }
  DEBUGF("Reading %zu of %zu bytes\n", nread, size);

  if (nread > 0)
  {
    int const bstate = flex_set_budge(0);
    assert(*anchor != NULL);
    const char *rptr = (char *)*anchor + reader->fpos;
    memcpy(ptr, rptr, nread);
    flex_set_budge(bstate);
  }

  return nread;
}

static void reader_flex_destroy(Reader * const reader)
{
  NOT_USED(reader);
}

void reader_flex_init(Reader * const reader, flex_ptr const anchor)
{
  assert(reader != NULL);
  assert(anchor != NULL);

  static ReaderFns const fns = {reader_flex_fread, reader_flex_destroy};
  reader_internal_init(reader, &fns, anchor);
}
