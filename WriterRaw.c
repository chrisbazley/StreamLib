/*
 * StreamLib: Raw file writer
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
  CJB: 11-Aug-19: Created this source file.
  CJB: 07-Sep-19: First released version.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

/* Local headers */
#include "WriterRaw.h"
#include "Internal/StreamMisc.h"

static size_t writer_raw_fwrite(void const *ptr,
  size_t const size, Writer * const writer)
{
  assert(ptr != NULL);
  assert(writer != NULL);
  FILE *const f = writer->data;
  assert(f != NULL);

  /* If fseek was used since the last write then find the right
     position at which to start writing. */
  if (writer->repos) {
    DEBUGF("Seeking offset %ld for fwrite\n", writer->fpos);

    if (fseek(f, writer->fpos, SEEK_SET)) {
      DEBUGF("fseek failed: %s\n", strerror(errno));
      writer->error = 1;
      return 0;
    }
    writer->repos = 0;
  }

  const size_t nwritten = fwrite(ptr, 1, size, f);
  if (nwritten != size) {
    DEBUGF("%zu of %zu bytes written: %s\n", nwritten, size, strerror(errno));
    writer->error = 1;
  }
  return nwritten;
}

static bool writer_raw_destroy(Writer * const writer)
{
  assert(writer != NULL);
  FILE *const f = writer->data;
  assert(f != NULL);

  /* Acorn's fclose returns an error and does not attempt to
     write any buffered data if the error indicator is set
     for the stream. */
  if (!writer->error && fflush(f)) {
    return false;
  }
  return true;
}

void writer_raw_init(Writer * const writer, FILE * const out)
{
  assert(writer != NULL);
  assert(out != NULL);
  assert(!ferror(out));

  static WriterFns const fns = {writer_raw_fwrite, writer_raw_destroy};
  writer_internal_init(writer, &fns, out);
}
