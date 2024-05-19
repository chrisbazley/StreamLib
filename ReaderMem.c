/*
 * StreamLib: Generic memory buffer reader
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
  CJB: 28-Nov-20: Initialize struct using compound literal assignment.
*/

/* ISO library header files */
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Local headers */
#include "Internal/StreamMisc.h"
#include "ReaderMem.h"

typedef struct {
  const char *buffer;
  size_t buffer_size;
} ReaderMemData;

static size_t reader_mem_fread(void *ptr, size_t const size,
                               Reader * const reader)
{
  assert(ptr != NULL);
  assert(reader != NULL);
  ReaderMemData *const data = reader->data;
  assert(data != NULL);
  assert(reader->fpos >= 0);

  if ((unsigned long)reader->fpos > data->buffer_size) {
    DEBUGF("Can't seek beyond end at %zu\n", data->buffer_size);
    reader->error = 1;
    return 0;
  }

  assert(data->buffer_size >= (unsigned long)reader->fpos);
  size_t const avail = data->buffer_size - (size_t)reader->fpos;

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

  assert(data->buffer != NULL);
  memcpy(ptr, data->buffer + reader->fpos, nread);

  return nread;
}

static void reader_mem_destroy(Reader * const reader)
{
  assert(reader != NULL);
  free(reader->data);
}

bool reader_mem_init(Reader *const reader, const void *const buffer,
  size_t const buffer_size)
{
  assert(reader != NULL);
  assert(buffer_size == 0 || buffer != NULL);

  ReaderMemData *const data = malloc(sizeof(*data));
  if (data == NULL) {
    DEBUGF("Failed to allocate memory for a new reader\n");
    return false;
  }

  *data = (ReaderMemData){
    .buffer = buffer,
    .buffer_size = buffer_size,
  };

  static ReaderFns const fns = {reader_mem_fread, reader_mem_destroy};
  reader_internal_init(reader, &fns, data);

  return true;
}
