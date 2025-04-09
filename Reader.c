/*
 * StreamLib: Abstract reader interface
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
  CJB: 08-Aug-18: Made reader_fread fail if a previous error occurred
                  or already at the end of the file (like UnixLib).
  CJB: 11-Aug-18: Made fseek/ftell independent of the back-end to allow
                  the file position to change freely without triggering
                  decompression.
  CJB: 12-Aug-19: Modified reader_fread_int32 not to rely on undefined
                  behaviour caused by unrepresentable results of
                  left-shifting a signed integer type.
                  Modified reader_fseek to guard against '-offset'
                  being unrepresentable as a long int (i.e. > LONG_MAX).
  CJB: 07-Sep-19: reader_feof and reader_ferror are actual functions now.
                  Reads that would cause the file position indicator to
                  overflow now set the error indicator instead.
  CJB: 05-Nov-19: Moved reader_fseek, reader_fgetc, reader_ungetc,
                  reader_fread_uint16 and reader_fread_int32 to separate
                  source files.
                  Added fast paths for common cases (one member was
                  read or member size is one byte).
  CJB: 07-Jun-20: Debugging output is less verbose by default.
  CJB: 28-Nov-20: Initialize struct using compound literal assignment.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

/* Local headers */
#include "Reader.h"
#include "Internal/StreamMisc.h"

bool reader_feof(const Reader * const reader)
{
  assert(reader != NULL);
  return reader->eof;
}

bool reader_ferror(const Reader * const reader)
{
  assert(reader != NULL);
  return reader->error;
}

long int reader_ftell(const Reader * const reader)
{
  assert(reader != NULL);
  long int pos = reader->fpos;
  if (reader->pushed_back != EOF) {
    /* We pushed back a character so the file position indicator is
       one character beyond where it should be. */
    --pos;
  }
  return pos;
}

size_t reader_fread(void *ptr, size_t const size, size_t const nmemb,
  Reader * const reader)
{
  size_t nread = 0;
  DEBUG_VERBOSEF("Read %zu members of size %zu\n", nmemb, size);

  assert(ptr != NULL);
  assert(reader != NULL);
  assert(reader->fpos >= 0);

  if (!reader->eof && !reader->error) {
    size_t bytes_to_read = nmemb * size;

    if (bytes_to_read > 0) {
      size_t nbytes = 0;

      if (reader->pushed_back != EOF) {
        /* A character was pushed back so output that first. */
        unsigned char const pb = (unsigned char)reader->pushed_back;
        DEBUGF("Read pushed back char %d\n", pb);
        unsigned char *const cptr = ptr;
        *cptr = pb;
        reader->pushed_back = EOF;
        ptr = cptr + 1;
        --bytes_to_read;
        ++nbytes;
      }

      if (bytes_to_read > 0) {
        if (bytes_to_read > (unsigned long)LONG_MAX ||
            (unsigned long)reader->fpos >
              (unsigned long)LONG_MAX - bytes_to_read) {
          DEBUGF("File position or data size is too big\n");
          reader->error = 1;
        } else {
          size_t const n = reader->fns.fread_fn(
            ptr, bytes_to_read, reader);

          nbytes += n;
          reader->fpos += n;
        }
      }
      if (size == nbytes) {
        nread = 1;
      } else {
        nread = size == 1 ? nbytes : nbytes / size;
      }
      DEBUG_VERBOSEF("Got %zu members of size %zu\n", nread, size);
      assert(nread == nmemb || reader->error || reader->eof);
    }
  }

  return nread;
}

void reader_internal_init(Reader *const reader,
  ReaderFns const *const fns, void *const data)
{
  assert(reader != NULL);
  assert(fns != NULL);

  *reader = (Reader){
    .fns = *fns,
    .data = data,
    .eof = 0,
    .error = 0,
    .repos = 0,
    .pushed_back = EOF,
    .fpos = 0,
  };
}

void reader_destroy(Reader * const reader)
{
  assert(reader != NULL);
  reader->fns.term_fn(reader);
}
