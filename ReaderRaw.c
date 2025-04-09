/*
 * StreamLib: Raw file reader
 * Copyright (C) 2018 Christopher Bazley
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
  CJB: 07-Aug-18: Copied this source file from SF3KtoObj.
  CJB: 11-Aug-19: Extra DEBUGF and const qualifiers.
  CJB: 27-Oct-19: Only call strerror (for debug output) if ferror.
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
#include "ReaderRaw.h"
#include "Internal/StreamMisc.h"

static size_t reader_raw_fread(void *ptr, size_t const size,
                               Reader * const reader)
{
  assert(ptr != NULL);
  assert(reader != NULL);
  FILE *const f = reader->data;
  assert(f != NULL);

  /* If fseek was used since the last read then find the right
     position at which to start reading. */
  if (reader->repos) {
    DEBUGF("Seeking offset %ld for fread\n", reader->fpos);

    if (fseek(f, reader->fpos, SEEK_SET)) {
      DEBUGF("fseek failed: %s\n", strerror(errno));
      reader->error = 1;
      return 0;
    }
    reader->repos = 0;
  }

  const size_t nread = fread(ptr, 1, size, f);
  if (nread != size) {
    DEBUGF("%zu of %zu bytes read\n", nread, size);

    if (ferror(f)) {
      DEBUGF("set error: %s\n", strerror(errno));
      reader->error = 1;
    } else {
      assert(feof(f));
      DEBUGF("set eof\n");
      reader->eof = 1;
    }
  }

  return nread;
}

static void reader_raw_destroy(Reader * const reader)
{
  NOT_USED(reader);
}

void reader_raw_init(Reader * const reader, FILE * const in)
{
  assert(reader != NULL);
  assert(in != NULL);
  assert(!ferror(in));
  assert(!feof(in));

  static ReaderFns const fns = {reader_raw_fread, reader_raw_destroy};
  reader_internal_init(reader, &fns, in);
}
