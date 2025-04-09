/*
 * StreamLib: Gordon Key compressed file writer
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
  CJB: 02-Nov-19: Deleted potentially misleading debugging output.
  CJB: 12-Nov-19: Pass long int instead of int32_t as the minimum file size.
                  Write the file header lazily instead of in the constructor.
  CJB: 07-Jun-20: Less verbose debugging output by default.
  CJB: 28-Nov-20: Initialize struct using compound literal assignment.
  CJB: 30-Nov-20: Fixed a bug in the empty_in function: instead of reading
                  input until the output buffer does not overflow, it now
                  reads input until all of the input data has been consumed.
                  The previous behaviour could cause an infinite loop because
                  the compressor ignores further input after it has returned
                  GKeyStatus_Finished in response to receiving an empty input
                  buffer.
  CJB: 06-Dec-20: Second attempt at a bugfix: calling empty_in twice before
                  empty_out on writer destruction wasn't sufficient to avoid
                  truncated output in a corner case, therefore a separate flush
                  loop is used with a different termination condition.
  CJB: 03-Apr-21: Assert that the specified minimum size, and the uncompressed
                  size actually written to the file header, are not negative.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

/* GKey library files */
#include "GKeyComp.h"

/* Local headers */
#include "WriterGKey.h"
#include "WriterRaw.h"
#include "Internal/StreamMisc.h"

enum
{
  BUFFER_SIZE = 256, /* No. of bytes to compress at a time */
};

typedef struct {
  bool wrote_hdr, owns_backend;
  char *in_ptr; /* remaining space within in_buffer */
  long int min_size;
  GKeyComp *comp;
  GKeyParameters params;
  Writer *backend;
} WriterGKeyState;

typedef struct {
  WriterGKeyState state;
  struct {
    char in[BUFFER_SIZE];
    char out[BUFFER_SIZE];
  } buffer;
} WriterGKeyData;

static void prepare_for_input(WriterGKeyData *const data)
{
  assert(data != NULL);
  data->state.in_ptr = data->buffer.in;
  data->state.params.in_buffer = data->buffer.in;
}

static void prepare_for_output(WriterGKeyData *const data)
{
  assert(data != NULL);
  data->state.params.out_buffer = data->buffer.out;
  data->state.params.out_size = sizeof(data->buffer.out);
}

static bool write_hdr(WriterGKeyData *const data,
  long int const len)
{
  assert(data != NULL);
  assert(len >= 0);

  if (len > INT32_MAX) {
    DEBUGF("Bad uncompressed size %ld\n", len);
    return false;
  }

  if (!writer_fwrite_int32((int32_t)len, data->state.backend)) {
    DEBUGF("Failed to write uncompressed size\n");
    return false;
  }

  DEBUGF("Wrote uncompressed size %ld\n", len);
  return true;
}

static bool empty_out(WriterGKeyData *const data)
{
  assert(data != NULL);

  /* Write size of compressed data if we didn't already */
  if (!data->state.wrote_hdr) {
    data->state.wrote_hdr = true;
    if (!write_hdr(data, data->state.min_size)) {
      return false;
    }
  }

  assert((const char *)data->state.params.out_buffer >= data->buffer.out);
  assert(data->state.params.out_size <= sizeof(data->buffer.out));
  size_t const used_size = sizeof(data->buffer.out) - data->state.params.out_size;

  /* Empty the output buffer by writing to file */
  size_t const n = writer_fwrite(data->buffer.out, 1, used_size, data->state.backend);
  DEBUGF("Emptied %zu bytes of compressed data from output buffer\n", n);

  if (n != used_size) {
    DEBUGF("Failed to write compressed data to file\n");
    return false;
  }

  prepare_for_output(data);
  return true;
}

static bool empty_in(WriterGKeyData *const data)
{
  assert(data != NULL);
  data->state.params.in_size = data->state.in_ptr -
                         (const char *)data->state.params.in_buffer;

  /* Compress data from the input buffer to the output buffer
     until the input buffer is empty */
  while (data->state.params.in_size > 0) {
    DEBUGF("Compressing %zu bytes of input\n", data->state.params.in_size);
    GKeyStatus const status = gkeycomp_compress(
                                 data->state.comp, &data->state.params);

    assert(status == GKeyStatus_OK || status == GKeyStatus_BufferOverflow);
    DEBUGF("Filled output buffer with %zu bytes of compressed data\n",
           sizeof(data->buffer.out) - data->state.params.out_size);

    if (status == GKeyStatus_BufferOverflow && !empty_out(data)) {
      return false;
    }
  }

  /* Reset the input buffer as it has been consumed. */
  assert(data->state.params.in_buffer == data->state.in_ptr);
  prepare_for_input(data);
  return true;
}

static bool flush(WriterGKeyData *const data)
{
  /* Flush any remaining buffered user data to the backend */
  assert(data != NULL);
  data->state.params.in_size = data->state.in_ptr -
                         (const char *)data->state.params.in_buffer;

  /* Compress data from the input buffer to the output buffer
     until no further input will be accepted */
  GKeyStatus status;
  do {
    DEBUGF("Flushing %zu bytes of input\n", data->state.params.in_size);
    status = gkeycomp_compress(data->state.comp, &data->state.params);
    assert(status == GKeyStatus_OK || status == GKeyStatus_BufferOverflow ||
           status == GKeyStatus_Finished);

    DEBUGF("Filled output buffer with %zu bytes of compressed data\n",
           sizeof(data->buffer.out) - data->state.params.out_size);

    if (!empty_out(data)) {
      return false;
    }
  } while (status != GKeyStatus_Finished);

  assert(data->state.params.in_size == 0);
  return true;
}

static unsigned long write_core(_Optional void const *ptr,
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
    if ((bytes_written < bytes_to_write) && !empty_in(data)) {
      writer->error = 1;
      break;
    }
  }

  /* If we failed to compress input data then we still report
     that some data was written if it was copied to the input
     buffer (like preceding calls to this function did). */
  return bytes_written;
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

    if (write_core(NULL, nzeros, writer) != nzeros) {
      DEBUGF("Failed to write trailing zeros\n");
      return false;
    }
  }

  if (!flush(data)) {
    return false;
  }

  if (flen > min_size) {
    /* Try to rewind the output file to correct the input data size. */
    if (writer_fseek(data->state.backend, 0, SEEK_SET)) {
      DEBUGF("Failed to seek start of file to increase size\n");
      return false;
    }

    /* Store the true uncompressed size at the start of the output. */
    if (!write_hdr(data, flen)) {
      return false;
    }
  }

  DEBUGF("Cleaned up successfully\n");
  return true;
}

static size_t writer_gkey_fwrite(void const * const ptr,
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
      DEBUGF("Cannot seek backwards (current position: %ld)\n", writer->flen);
      writer->error = 1;
      return 0;
    }

    unsigned long const bytes_to_skip = writer->fpos - writer->flen;
    DEBUGF("Skipping %lu bytes\n", bytes_to_skip);
    unsigned long const nskipped = write_core(NULL, bytes_to_skip, writer);

    assert(nskipped <= bytes_to_skip);
    if (nskipped != bytes_to_skip) {
      return 0;
    }
  }

  unsigned long const nwritten = write_core(ptr, bytes_to_write, writer);
  assert(nwritten <= bytes_to_write);
  return (size_t)nwritten;
}

static bool writer_gkey_destroy(Writer * const writer)
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

  if (data->state.owns_backend) {
    if (writer_destroy(data->state.backend) < 0) {
      success = false;
    }
    free(data->state.backend);
  }

  free(data);
  return success;
}

bool writer_gkey_init_from(Writer * const writer,
                           unsigned int const history_log_2,
                           long int const min_size,
                           Writer * const out)
{
  assert(writer != NULL);
  assert(out != NULL);
  assert(!writer_ferror(out));

  _Optional WriterGKeyData *const data = malloc(sizeof(*data));
  if (data == NULL) {
    DEBUGF("Failed to allocate writer data\n");
    return false;
  }

  data->state = (WriterGKeyState){
    .backend = out,
    .owns_backend = false,
    .wrote_hdr = false,
    .min_size = min_size,
    .params = {
      .prog_cb = (GKeyProgressFn *)NULL,
      .cb_arg = writer,
    },
  };

  _Optional GKeyComp *const comp = gkeycomp_make(history_log_2);
  if (comp == NULL) {
    DEBUGF("Failed to create compressor\n");
    free(data);
    return false;
  }
  data->state.comp = &*comp;

  static WriterFns const fns = {writer_gkey_fwrite, writer_gkey_destroy};
  writer_internal_init(writer, &fns, &*data);

  prepare_for_input(&*data);
  prepare_for_output(&*data);

  return true;
}

bool writer_gkey_init(Writer * const writer,
                      unsigned int const history_log_2,
                      long int const min_size,
                      FILE * const out)
{
  assert(writer != NULL);
  assert(min_size >= 0);
  assert(out != NULL);
  assert(!ferror(out));

  _Optional Writer *const raw = malloc(sizeof(*raw));
  if (raw == NULL) {
    DEBUGF("Failed to allocate raw backend\n");
    return false;
  }

  writer_raw_init(&*raw, out);

  bool const success = writer_gkey_init_from(
    writer, history_log_2, min_size, &*raw);

  if (!success) {
    DEBUGF("Failed to initialize a new writer\n");
    (void)writer_destroy(&*raw);
    free(raw);
  } else {
    WriterGKeyData *const data = writer->data;
    data->state.owns_backend = true; /* override default */
  }

  return success;
}
