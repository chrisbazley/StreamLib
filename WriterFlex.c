/*
 * StreamLib: Flex memory buffer writer
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
  CJB: 25-Aug-19: Created this source file.
  CJB: 07-Sep-19: First released version.
  CJB: 07-Jun-20: Fixed misplaced "Seeking offset..." debugging output.
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
#include "WriterFlex.h"
#include "Internal/StreamMisc.h"

static void zero_extend(Writer * const writer, size_t const new_size)
{
  assert(writer != NULL);
  flex_ptr const anchor = writer->data;
  assert(anchor != NULL);

  assert(new_size >= (unsigned long)writer->flen);
  size_t const bytes_to_skip = new_size - (size_t)writer->flen;
  DEBUGF("Zeroing %zu bytes at offset %ld\n",
    bytes_to_skip, writer->flen);

  if (bytes_to_skip > 0) {
    assert(anchor != NULL);
    int const bstate = flex_set_budge(0);
    assert(*anchor != NULL);
    memset((char *)*anchor + writer->flen, 0, bytes_to_skip);
    flex_set_budge(bstate);
  }
}

static int buffer_size(flex_ptr const anchor)
{
  assert(anchor != NULL);
  return *anchor ? flex_size(anchor) : 0;
}

static bool resize_buffer(flex_ptr const anchor, int const new_size)
{
  assert(anchor != NULL);

  DEBUGF("flex_extend from %d to %d for writer\n",
    buffer_size(anchor), new_size);

  int const success = *anchor ?
      flex_extend(anchor, new_size) :
      flex_alloc(anchor, new_size);

  if (!success) {
    DEBUGF("flex_extend failed\n");
    return false;
  }

  return true;
}

static size_t writer_flex_fwrite(void const *ptr,
  size_t const size, Writer * const writer)
{
  assert(ptr != NULL);
  assert(writer != NULL);
  flex_ptr const anchor = writer->data;
  assert(anchor != NULL);
  assert(writer->fpos >= 0);
  assert((unsigned long)writer->fpos <= ULONG_MAX - size);

  unsigned long const end = (unsigned long)writer->fpos + size;
  unsigned long const max =
    (SIZE_MAX < (unsigned)INT_MAX ? SIZE_MAX : (unsigned)INT_MAX);

  if (end > max) {
    DEBUGF("File position %ld or data size %zu is too big\n",
      writer->fpos, size);

    writer->error = 1;
    return 0;
  }

  int const fsize = buffer_size(anchor);

  if (end > (unsigned)fsize) {
    int newsize = (int)end;
    if ((fsize <= (INT_MAX / 2)) &&
        ((fsize * 2) >= newsize)) {
      newsize = fsize * 2;
    }
    if (!resize_buffer(anchor, newsize)) {
      writer->error = 1;
      return 0;
    }
  }

  if (writer->fpos > writer->flen) {
    DEBUGF("Seeking offset %ld in file\n", writer->fpos);
    /* To simulate a sparse file, zero-initialize skipped bytes. */
    assert((unsigned long)writer->fpos <= SIZE_MAX);
    zero_extend(writer, (size_t)writer->fpos);
  }

  assert(anchor != NULL);
  int const bstate = flex_set_budge(0);
  assert(*anchor != NULL);
  memcpy((char *)*anchor + writer->fpos, ptr, size);
  flex_set_budge(bstate);

  return size;
}

static bool writer_flex_destroy(Writer * const writer)
{
  assert(writer != NULL);
  flex_ptr const anchor = writer->data;
  assert(anchor != NULL);

  /* Acorn's fclose does not attempt to write any buffered data
     if the error indicator is set for the stream. */
  if (!writer->error) {
    /* Truncate the buffer to the minimum required size */
    int const fsize = buffer_size(anchor);
    assert(fsize >= writer->flen);
    if ((fsize > writer->flen) &&
        !resize_buffer(anchor, (int)writer->flen)) {
      return false;
    }
  }
  return true;
}

void writer_flex_init(Writer * const writer, flex_ptr const anchor)
{
  assert(writer != NULL);
  assert(anchor != NULL);

  static WriterFns const fns = {writer_flex_fwrite, writer_flex_destroy};
  writer_internal_init(writer, &fns, anchor);
}
