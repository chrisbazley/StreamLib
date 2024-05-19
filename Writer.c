/*
 * StreamLib: Abstract writer interface
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
  CJB: 05-Nov-19: Moved writer_fseek, writer_fputc, writer_fwrite_uint16
                  and writer_fwrite_int32 to separate source files.
                  Added fast paths for common cases (one member was
                  written or member size is one byte).
  CJB: 07-Jun-20: Debugging output is less verbose by default.
  CJB: 28-Nov-20: Initialize struct using compound literal assignment.
*/

/* ISO library header files */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

/* Local headers */
#include "Internal/StreamMisc.h"
#include "Writer.h"

bool writer_ferror(const Writer * const writer)
{
  assert(writer != NULL);
  return writer->error;
}

long int writer_ftell(const Writer * const writer)
{
  assert(writer != NULL);
  return writer->fpos;
}

size_t writer_fwrite(void const * const ptr, size_t const size,
  size_t const nmemb, Writer * const writer)
{
  size_t nwritten = 0;
  DEBUG_VERBOSEF("Write %zu members of size %zu\n", nmemb, size);

  assert(ptr != NULL);
  assert(writer != NULL);
  assert(writer->fpos >= 0);

  if (!writer->error) {
    size_t const bytes_to_write = nmemb * size;

    if (bytes_to_write > 0) {
      if (bytes_to_write > (unsigned long)LONG_MAX ||
          (unsigned long)writer->fpos >
            (unsigned long)LONG_MAX - bytes_to_write) {
        DEBUGF("File position or data size is too big\n");
        writer->error = 1;
      } else {
        size_t const n = writer->fns.fwrite_fn(
          ptr, bytes_to_write, writer);

        writer->fpos += n;
        if (writer->fpos > writer->flen) {
          writer->flen = writer->fpos;
        }
        if (size == n) {
          nwritten = 1;
        } else {
          nwritten = size == 1 ? n : n / size;
        }
        DEBUG_VERBOSEF("Wrote %zu members of size %zu\n", nwritten, size);
        assert(nwritten == nmemb || writer->error);
      }
    }
  }

  return nwritten;
}

void writer_internal_init(Writer *const writer,
  WriterFns const *const fns, void *data)
{
  assert(writer != NULL);
  DEBUGF("Initializing writer %p with data %p\n", (void *)writer, data);
  assert(fns != NULL);

  *writer = (Writer){
    .fns = *fns,
    .data = data,
    .error = 0,
    .repos = 0,
    .fpos = 0,
    .flen = 0,
  };
}

long int writer_destroy(Writer *const writer)
{
  assert(writer != NULL);
  DEBUGF("Destroying writer %p\n", (void *)writer);

  /* Acorn's fclose returns an error if the error indicator is set
     for the stream so do likewise. */
  return !writer->fns.term_fn(writer) || writer->error ? -1l : writer->flen;
}
