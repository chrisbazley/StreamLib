/*
 * StreamLib: Generic memory buffer writer
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
  CJB: 28-Nov-20: Initialize struct using compound literal assignment.
*/

/* ISO library header files */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

/* Local headers */
#include "Internal/StreamMisc.h"
#include "WriterMem.h"

typedef struct {
  char *buffer;
  size_t buffer_size;
} WriterMemData;

static void zero_extend(Writer * const writer, size_t const new_len)
{
  assert(writer != NULL);
  WriterMemData *const data = writer->data;
  assert(data != NULL);

  assert(new_len >= (unsigned long)writer->flen);
  size_t const bytes_to_skip = new_len - (size_t)writer->flen;
  DEBUGF("Zeroing %zu bytes at offset %ld\n",
    bytes_to_skip, writer->flen);

  if (bytes_to_skip > 0) {
    assert(data->buffer != NULL);
    memset(data->buffer + writer->flen, 0, bytes_to_skip);
  }
}

static size_t writer_mem_fwrite(void const *ptr,
  size_t const size, Writer * const writer)
{
  assert(ptr != NULL);
  assert(writer != NULL);
  WriterMemData *const data = writer->data;
  assert(data != NULL);
  assert((unsigned long)writer->flen <= data->buffer_size);
  assert(writer->fpos >= 0);

  if ((unsigned long)writer->fpos > data->buffer_size) {
    DEBUGF("Can't seek beyond end at %zu\n", data->buffer_size);
    writer->error = 1;
    return 0;
  }

  assert(data->buffer_size >= (unsigned long)writer->fpos);
  size_t const avail = data->buffer_size - (size_t)writer->fpos;

  size_t nwrite = size;
  if (avail < nwrite) {
    DEBUGF("Partial write outside file\n");
    writer->error = 1;
    nwrite = avail;
  }

  if (writer->fpos > writer->flen) {
    /* To simulate a sparse file, zero-initialize skipped bytes. */
    assert((unsigned long)writer->fpos <= SIZE_MAX);
    zero_extend(writer, (size_t)writer->fpos);
  }

  if (nwrite > 0) {
    assert(data->buffer != NULL);
    memcpy(data->buffer + writer->fpos, ptr, nwrite);
  }

  return nwrite;
}

static bool writer_mem_destroy(Writer * const writer)
{
  assert(writer != NULL);
  free(writer->data);
  return true;
}

bool writer_mem_init(Writer * const writer, void * const buffer,
  size_t const buffer_size)
{
  assert(writer != NULL);
  assert(buffer_size == 0 || buffer != NULL);

  WriterMemData *const data = malloc(sizeof(*data));
  if (data == NULL) {
    DEBUGF("Failed to allocate memory for a new writer\n");
    return false;
  }

  *data = (WriterMemData){
    .buffer = buffer,
    .buffer_size = buffer_size,
  };

  static WriterFns const fns = {writer_mem_fwrite, writer_mem_destroy};
  writer_internal_init(writer, &fns, data);

  return true;
}
