/*
 * StreamLib: Gordon Key compressed file size estimator
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
  CJB: 08-Sep-19: Created this source file.
  CJB: 21-Nov-19: Changed the output of writer_gkc_init from int32_t to
                  long int.
  CJB: 27-Sep-20: Less verbose debugging output by default.
                  Simplified based on the fact that write_core() can't fail.
                  Added support for padding the end of the input to reach
                  a specified minimum size.
  CJB: 28-Nov-20: Initialize struct using compound literal assignment.
  CJB: 03-Apr-21: Assert that the specified minimum size is not negative.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

/* GKey library files */
#include "GKeyComp.h"

/* Local headers */
#include "WriterGKC.h"
#include "Internal/StreamMisc.h"

enum
{
  BUFFER_SIZE = 256, /* No. of bytes to compress at a time */
};

typedef struct {
  char *in_ptr; /* remaining space within in_buffer */
  long int min_size;
  GKeyComp *comp;
  GKeyParameters params;
  long int *out_size;
} WriterGKeyState;

typedef struct {
  WriterGKeyState state;
  struct {
    char in[BUFFER_SIZE];
  } buffer;
} WriterGKeyData;

static void prepare_for_input(WriterGKeyData *const data)
{
  assert(data != NULL);
  data->state.in_ptr = data->buffer.in;
  data->state.params.in_buffer = data->buffer.in;
}

static void empty_in(WriterGKeyData *const data)
{
  assert(data != NULL);
  data->state.params.in_size = data->state.in_ptr -
                         (const char *)data->state.params.in_buffer;
  DEBUGF("Flushing %zu bytes of input\n", data->state.params.in_size);

  /* Compress the data from the input buffer to the output buffer */
  GKeyStatus const status = gkeycomp_compress(data->state.comp, &data->state.params);
  assert(status == GKeyStatus_OK || status == GKeyStatus_Finished);
  NOT_USED(status);
  DEBUGF("Generated %zu bytes of compressed data\n", data->state.params.out_size);

  /* Reset the input buffer if it has been consumed. */
  if (!data->state.params.in_size) {
    assert(data->state.params.in_buffer == data->state.in_ptr);
    prepare_for_input(data);
  }
}

static void flush(WriterGKeyData *const data)
{
  /* Flush any remaining buffered user data */
  empty_in(data);
  empty_in(data);
}

static void write_core(_Optional void const *ptr,
  unsigned long const bytes_to_write, Writer *const writer)
{
  assert(writer != NULL);
  WriterGKeyData *const data = writer->data;
  assert(data != NULL);

  unsigned long bytes_written = 0;
  while (bytes_written < bytes_to_write) {
    /* If there is still space for uncompressed data in the input buffer
       then copy it from the caller's buffer. */
    assert((const char *)data->state.params.in_buffer <= data->state.in_ptr);
    unsigned long const n = bytes_to_write - bytes_written;
    const size_t space_used = data->state.in_ptr -
                              (const char *)data->state.params.in_buffer;

    assert(space_used <= sizeof(data->buffer.in));
    const size_t space_avail = sizeof(data->buffer.in) - space_used;
    const size_t copy_size = n > space_avail ? space_avail : (size_t)n;

    if (copy_size) {
      if (ptr) {
        DEBUG_VERBOSEF("Copying %zu to input buffer of %zu bytes\n",
               copy_size, space_avail);
        memcpy(data->state.in_ptr, &*ptr, copy_size);
        ptr = (char *)ptr + copy_size;
      } else {
        DEBUG_VERBOSEF("Zeroing %zu in input buffer of %zu bytes\n",
               copy_size, space_avail);
        memset(data->state.in_ptr, 0, copy_size);
      }
      data->state.in_ptr += copy_size;
      bytes_written += copy_size;
    }
    DEBUG_VERBOSEF("Put %lu of %lu bytes\n", bytes_written, bytes_to_write);

    /* If we didn't have room to write all of the data then
       empty the input buffer. */
    if (bytes_written < bytes_to_write) {
      empty_in(data);
    }
  }
}

static bool cleanup(Writer *const writer)
{
  assert(writer != NULL);
  WriterGKeyData *const data = writer->data;
  assert(data != NULL);

  long int const flen = writer->flen;
  long int const min_size = data->state.min_size;

  if (flen < min_size) {
    unsigned long const nzeros = min_size - flen;
    DEBUGF("Writing %lu trailing zeros to reach min size %ld\n",
           nzeros, min_size);

    write_core(NULL, nzeros, writer);
  }

  flush(data);

  /* Allow room for the decompressed size to be stored too */
  if (data->state.params.out_size > LONG_MAX - sizeof(int32_t)) {
    return false;
  }
  assert(data->state.out_size != NULL);
  *data->state.out_size = (long)sizeof(int32_t) + (long)data->state.params.out_size;
  return true;
}

static size_t writer_gkc_fwrite(void const * const ptr,
  size_t const bytes_to_write, Writer * const writer)
{
  assert(ptr != NULL);
  assert(writer != NULL);
  assert(writer->fpos >= 0);

  /* If fseek was used since the last write then find the right position
     at which to start writing. */
  if (writer->fpos != writer->flen) {
    DEBUGF("Seeking offset %ld in file\n", writer->fpos);

    /* Seeking backwards would require compressing data from the start
       of the file to the requested place again but we can't. */
    if (writer->fpos < writer->flen) {
      DEBUGF("Cannot seek backwards\n");
      writer->error = 1;
      return 0;
    }

    unsigned long const bytes_to_skip = writer->fpos - writer->flen;
    DEBUGF("Skipping %lu bytes\n", bytes_to_skip);
    write_core(NULL, bytes_to_skip, writer);
  }

  write_core(ptr, bytes_to_write, writer);
  return bytes_to_write;
}

static bool writer_gkc_destroy(Writer * const writer)
{
  assert(writer != NULL);
  WriterGKeyData *const data = writer->data;
  assert(data != NULL);

  /* Acorn's fclose does not attempt to write any buffered data if
     the error indicator is set for the stream. */
  bool success = true;
  if (!writer->error && !cleanup(writer)) {
    success = false;
  }

  gkeycomp_destroy(data->state.comp);
  free(data);
  return success;
}

bool writer_gkc_init(Writer * const writer,
                      unsigned int const history_log_2,
                      long int *const out_size)
{
  return writer_gkc_init_with_min(writer, history_log_2, 0, out_size);
}

bool writer_gkc_init_with_min(Writer * const writer,
                              unsigned int const history_log_2,
                              long int const min_size,
                              long int *const out_size)
{
  assert(writer != NULL);
  assert(min_size >= 0);
  assert(out_size != NULL);

  _Optional WriterGKeyData *const data = malloc(sizeof(*data));
  if (data == NULL) {
    DEBUGF("Failed to allocate writer data\n");
    return false;
  }

  data->state = (WriterGKeyState){
    .params = {
      .out_buffer = NULL,
      .out_size = 0,
      .prog_cb = (GKeyProgressFn *)NULL,
      .cb_arg = writer,
    },
    .min_size = min_size,
    .out_size = out_size,
  };

  _Optional GKeyComp *const comp = gkeycomp_make(history_log_2);
  if (comp == NULL) {
    DEBUGF("Failed to create compressor\n");
    free(data);
    return false;
  }
  data->state.comp = &*comp;

  static WriterFns const fns = {writer_gkc_fwrite, writer_gkc_destroy};
  writer_internal_init(writer, &fns, &*data);

  prepare_for_input(&*data);

  return true;
}
