/*
 * StreamLib: Gordon Key compressed file reader
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
  CJB: 15-Aug-19: Fixed broken backward-seek in compressed file, which
                  only appeared to work for small offsets and files.
                  Decompressed size is now read in a separate function.
  CJB: 12-Nov-19: Read the file header lazily instead of in the constructor.
  CJB: 07-Jun-20: Debugging output is less verbose by default.
  CJB: 28-Nov-20: Initialize struct using compound literal assignment.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

/* GKey library files */
#include "GKeyDecomp.h"

/* Local headers */
#include "ReaderGKey.h"
#include "ReaderRaw.h"
#include "Internal/StreamMisc.h"

enum
{
  BUFFER_SIZE = 256, /* No. of bytes to decompress at a time */
};

typedef struct {
  bool read_hdr, owns_backend;
  const char *out_ptr; /* remaining data within out_buffer */
  long int out_total, out_len;
  GKeyDecomp *decomp;
  GKeyParameters params;
  Reader *backend;
} ReaderGKeyState;

typedef struct {
  ReaderGKeyState state;
  struct {
    char in[BUFFER_SIZE];
    char out[BUFFER_SIZE];
  } buffer;
} ReaderGKeyData;

static void prepare_for_output(ReaderGKeyData *const data)
{
  assert(data != NULL);
  data->state.out_ptr = data->buffer.out;
  data->state.params.out_buffer = data->buffer.out;
}

static void rewind_reinit(ReaderGKeyData *const data)
{
  assert(data != NULL);
  data->state.out_total = 0;
  prepare_for_output(data);
  data->state.params.in_size = 0;
}

static unsigned long read_core(_Optional void *ptr, unsigned long const bytes_to_read,
  Reader * const reader)
{
  unsigned long bytes_read = 0;

  assert(reader != NULL);
  ReaderGKeyData *const data = reader->data;
  assert(data != NULL);
  assert(reader->fpos >= 0);

  while (!reader->error && (bytes_read < bytes_to_read)) {
    /* If there is already decompressed data in the output buffer
       then copy that to the caller's buffer. */
    assert((const char *)data->state.params.out_buffer >= data->state.out_ptr);
    unsigned long const n = bytes_to_read - bytes_read;
    const size_t bytes_avail = (const char *)data->state.params.out_buffer -
                                  data->state.out_ptr;
    DEBUG_VERBOSEF("%zu bytes are available (need %lu)\n", bytes_avail, n);
    const size_t copy_size = (size_t)(n > bytes_avail ? bytes_avail : n);

    if (copy_size) {
      if (ptr) {
        DEBUG_VERBOSEF("Copying %zu of %zu bytes from output buffer\n",
               copy_size, bytes_avail);
        memcpy(&*ptr, data->state.out_ptr, copy_size);
        ptr = (char *)ptr + copy_size;
      }
      data->state.out_ptr += copy_size;
      bytes_read += copy_size;
    }

    /* If we didn't get enough data yet then decompress some more. */
    if (bytes_read < bytes_to_read) {
      DEBUG_VERBOSEF("Need to refill output buffer (only got %lu of %lu bytes)\n",
             bytes_read, bytes_to_read);

      bool in_pending = false;
      GKeyStatus status = GKeyStatus_OK;

      assert(data->state.out_ptr == data->state.params.out_buffer);
      prepare_for_output(data);
      data->state.params.out_size = sizeof(data->buffer.out);

      do {
        /* Is the input buffer empty? */
        if (data->state.params.in_size == 0) {
          /* Fill the input buffer by reading from file */
          data->state.params.in_buffer = data->buffer.in;
          data->state.params.in_size = reader_fread(
            data->buffer.in, 1, sizeof(data->buffer.in), data->state.backend);

          DEBUG_VERBOSEF("Filled input buffer with %zu bytes of compressed data\n",
                 data->state.params.in_size);
          if (data->state.params.in_size != sizeof(data->buffer.in) &&
              reader_ferror(data->state.backend)) {
            /* Read error not end of file */
            DEBUGF("Failed to read compressed data from file\n");
            reader->error = 1;
            break;
          }
        }

        /* Decompress the data from the input buffer to the output buffer */
        status = gkeydecomp_decompress(data->state.decomp, &data->state.params);

        /* If the input buffer is empty and it cannot be (re-)filled then
           there is no more input pending. */
        in_pending = data->state.params.in_size > 0 ||
                     (!reader_feof(data->state.backend) &&
                      !reader_ferror(data->state.backend));

        if (in_pending && status == GKeyStatus_TruncatedInput) {
          /* False alarm before end of input data */
          status = GKeyStatus_OK;
        }
      } while (in_pending && status == GKeyStatus_OK);

      DEBUG_VERBOSEF("Filled output buffer with %zu bytes of uncompressed data\n",
             sizeof(data->buffer.out) - data->state.params.out_size);

      if (!reader->error) {
        switch (status) {
          case GKeyStatus_BadInput:
            DEBUGF("Compressed bitstream contains bad data\n");
            reader->error = 1;
            break;

          case GKeyStatus_TruncatedInput:
            DEBUGF("Compressed bitstream appears truncated\n");
            reader->error = 1;
            break;

          case GKeyStatus_BufferOverflow:
            /* The output buffer was filled but not all of the data in
               the input buffer was used up. */
            assert(data->state.params.out_size == 0);
            break;

          case GKeyStatus_OK:
            assert(!in_pending);
            if (data->state.params.out_size == sizeof(data->buffer.out)) {
              DEBUGF("Compressed bitstream appears truncated\n");
              reader->error = 1;
            }
            break;

          default:
            assert("Impossible state" == NULL);
            break;
        }
      }
    }
  }

  data->state.out_total += bytes_read;
  return bytes_read;
}

static bool read_hdr(ReaderGKeyData *const data)
{
  assert(data != NULL);

  int32_t out_len;
  if (!reader_fread_int32(&out_len, data->state.backend)) {
    DEBUGF("Failed to read decompressed size: %s\n",
            reader_feof(data->state.backend) ? "End of file" : "Error");
    return false;
  }

  DEBUGF("Decompressed data size is %" PRId32 " bytes\n", out_len);
  if (out_len < 0) {
    DEBUGF("Bad size %" PRId32 " in compressed file\n", out_len);
    return false;
  }

  data->state.out_len = out_len;
  return true;
}

static size_t reader_gkey_fread(void * const ptr, size_t bytes_to_read,
                                Reader * const reader)
{
  assert(ptr != NULL);
  assert(reader != NULL);
  ReaderGKeyData *const data = reader->data;
  assert(data != NULL);
  assert(reader->fpos >= 0);

  /* Get size of decompressed data if we didn't already */
  if (!data->state.read_hdr) {
    data->state.read_hdr = true;
    if (!read_hdr(data)) {
      reader->error = 1;
      return 0;
    }
  }
  assert(data->state.out_len >= data->state.out_total);

  /* If fseek was used since the last read then find the right
     position at which to start reading. */
  if (reader->fpos > data->state.out_len) {
    DEBUGF("Can't seek %ld beyond end %ld\n", reader->fpos, data->state.out_len);
    reader->error = 1;
    return 0;
  }

  if (reader->fpos != data->state.out_total) {
    DEBUGF("Seeking offset %ld in file (out %ld)\n", reader->fpos,
      data->state.out_total);

    if (reader->fpos < data->state.out_total) {
      assert(data->state.out_ptr >= data->buffer.out);
      size_t const out_buf_used = data->state.out_ptr - data->buffer.out;
      DEBUGF("%zu bytes of buffer were already output\n", out_buf_used);

      long int const buf_start = data->state.out_total - out_buf_used;
      DEBUGF("Buffer starts at offset %ld\n", buf_start);

      if (reader->fpos >= buf_start) {
        long int const buf_offset = reader->fpos - buf_start;
        DEBUGF("Seeking offset %ld in buffer\n", buf_offset);
        data->state.out_total = reader->fpos;
        data->state.out_ptr = data->buffer.out + buf_offset;
      } else {
        /* Seeking backwards requires decompressing data
           from the start of the file to the requested place again. */
        DEBUGF("Seeking start of file for fread\n");
        if (reader_fseek(data->state.backend, sizeof(uint32_t), SEEK_SET)) {
          reader->error = 1;
          return 0;
        }
        rewind_reinit(data);
      }
    }

    unsigned long const bytes_to_skip = reader->fpos - data->state.out_total;
    DEBUGF("Skipping %lu bytes\n", bytes_to_skip);
    unsigned long const nskipped = read_core(NULL, bytes_to_skip, reader);

    assert(nskipped <= bytes_to_skip);
    if (nskipped != bytes_to_skip) {
      return 0;
    }

    DEBUGF("Successfully repositioned to %ld\n", reader->fpos);
  }

  /* Don't try to read more bytes than advertised as available. */
  unsigned long const avail = data->state.out_len - data->state.out_total;
  if (avail < bytes_to_read) {
    DEBUGF("Can't read %zu bytes: end of file at %lu\n",
      bytes_to_read, avail);

    bytes_to_read = (size_t)avail;
    reader->eof = 1;
  }

  unsigned long const nread = read_core(ptr, bytes_to_read, reader);
  assert(nread <= bytes_to_read);
  return (size_t)nread;
}

static void reader_gkey_destroy(Reader * const reader)
{
  assert(reader != NULL);
  ReaderGKeyData *const data = reader->data;
  assert(data != NULL);
  gkeydecomp_destroy(data->state.decomp);
  if (data->state.owns_backend) {
    reader_destroy(data->state.backend);
    free(data->state.backend);
  }
  free(data);
}

bool reader_gkey_init_from(Reader * const reader,
                           unsigned int const history_log_2,
                           Reader * const in)
{
  assert(reader != NULL);
  assert(in != NULL);
  assert(!reader_ferror(in));
  assert(!reader_feof(in));

  _Optional ReaderGKeyData *const data = malloc(sizeof(*data));
  if (data == NULL) {
    DEBUGF("Failed to allocate memory for a new reader\n");
    return false;
  }

  data->state = (ReaderGKeyState){
    .backend = in,
    .owns_backend = false,
    .read_hdr = false,
    .params = {
      .prog_cb = (GKeyProgressFn *)NULL,
      .cb_arg = reader,
    },
  };

  _Optional GKeyDecomp *const decomp = gkeydecomp_make(history_log_2);
  if (decomp == NULL) {
    DEBUGF("Failed to create decompressor\n");
    free(data);
    return false;
  }
  data->state.decomp = &*decomp;

  static ReaderFns const fns = {reader_gkey_fread, reader_gkey_destroy};
  reader_internal_init(reader, &fns, &*data);
  rewind_reinit(&*data);

  return true;
}

bool reader_gkey_init(Reader * const reader,
                      unsigned int const history_log_2,
                      FILE * const in)
{
  assert(reader != NULL);
  assert(in != NULL);
  assert(!ferror(in));
  assert(!feof(in));

  _Optional Reader *const raw = malloc(sizeof(*raw));
  if (raw == NULL) {
    DEBUGF("Failed to allocate raw backend\n");
    return false;
  }

  reader_raw_init(&*raw, in);

  bool const success = reader_gkey_init_from(
    reader, history_log_2, &*raw);

  if (!success) {
    DEBUGF("Failed to initialize a new reader\n");
    reader_destroy(&*raw);
    free(raw);
  } else {
    ReaderGKeyData *const data = reader->data;
    data->state.owns_backend = true; /* override default */
  }

  return success;
}
