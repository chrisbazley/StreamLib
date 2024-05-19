/*
 * StreamLib: Reallocating memory buffer writer
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
  CJB: 26-Aug-19: Created this source file.
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
#include "WriterHeap.h"

typedef struct {
  void **buffer;
  size_t buffer_size;
} WriterHeapData;

static void zero_extend(Writer * const writer, size_t const new_size)
{
  assert(writer != NULL);
  WriterHeapData *const data = writer->data;
  assert(data != NULL);

  assert(new_size >= (unsigned long)writer->flen);
  size_t const bytes_to_skip = new_size - (size_t)writer->flen;
  DEBUGF("Zeroing %zu bytes at offset %ld\n",
    bytes_to_skip, writer->flen);

  if (bytes_to_skip > 0) {
    assert(data->buffer != NULL);
    assert(*data->buffer != NULL);
    memset((char *)(*data->buffer) + writer->flen, 0, bytes_to_skip);
  }
}

static bool resize_buffer(Writer * const writer, size_t const new_size)
{
  assert(writer != NULL);
  WriterHeapData *const data = writer->data;
  assert(data != NULL);

  DEBUGF("realloc from %zu to %zu for writer\n",
    data->buffer_size, new_size);

  void *const new_buffer = realloc(*data->buffer, new_size);
  if (new_buffer == NULL) {
    DEBUGF("realloc failed\n");
    return false;
  }

  *data->buffer = new_buffer;
  data->buffer_size = new_size;
  return true;
}

static bool cleanup(Writer * const writer)
{
  assert(writer != NULL);
  WriterHeapData *const data = writer->data;
  assert(data != NULL);
  assert(data->buffer_size >= (unsigned long)writer->flen);

  /* Truncate the buffer to the minimum required size */
  if ((data->buffer_size > (unsigned long)writer->flen) &&
      !resize_buffer(writer, (size_t)writer->flen)) {
    return false;
  }
  return true;
}

static size_t writer_heap_fwrite(void const *ptr,
  size_t const size, Writer * const writer)
{
  assert(ptr != NULL);
  assert(writer != NULL);
  WriterHeapData *const data = writer->data;
  assert(data != NULL);
  assert(writer->fpos >= 0);
  assert((unsigned long)writer->fpos <= ULONG_MAX - size);

  size_t const buffer_size = data->buffer_size;
  assert(buffer_size >= (unsigned long)writer->flen);

  unsigned long const end = writer->fpos + size;
  if (end > SIZE_MAX) {
    DEBUGF("File position %ld or data size %zu is too big\n",
      writer->fpos, size);

    writer->error = 1;
    return 0;
  }

  if (end > buffer_size) {
    size_t newsize = (size_t)end;
    if ((buffer_size <= (SIZE_MAX / 2)) &&
        ((buffer_size * 2) >= newsize)) {
      newsize = buffer_size * 2;
    }
    if (!resize_buffer(writer, newsize)) {
      writer->error = 1;
      return 0;
    }
  }

  if (writer->fpos > writer->flen) {
    /* To simulate a sparse file, zero-initialize skipped bytes. */
    assert((unsigned long)writer->fpos <= SIZE_MAX);
    zero_extend(writer, (size_t)writer->fpos);
  }

  assert(data->buffer != NULL);
  assert(*data->buffer != NULL);
  memcpy((char *)(*data->buffer) + writer->fpos, ptr, size);

  return size;
}

static bool writer_heap_destroy(Writer * const writer)
{
  assert(writer != NULL);
  /* Acorn's fclose does not attempt to write any buffered data if
     the error indicator is set for the stream. */
  bool success = true;
  if (!writer->error && !cleanup(writer)) {
    success = false;
  }
  free(writer->data);
  return success;
}

bool writer_heap_init(Writer * const writer, void ** const buffer,
  size_t const buffer_size)
{
  assert(writer != NULL);
  assert(buffer != NULL);
  assert(buffer_size == 0 || *buffer != NULL);

  WriterHeapData *const data = malloc(sizeof(*data));
  if (data == NULL) {
    DEBUGF("Failed to allocate memory for a new writer\n");
    return false;
  }

  *data = (WriterHeapData){
    .buffer_size = buffer_size,
    .buffer = buffer,
  };

  static WriterFns const fns = {writer_heap_fwrite, writer_heap_destroy};
  writer_internal_init(writer, &fns, data);

  return true;
}
