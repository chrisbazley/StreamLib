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

/*
Dependencies: ANSI C library.
Message tokens: None.
History:
  CJB: 11-Aug-19: Created this source file.
  CJB: 02-Sep-19: First released version.
  CJB: 26-Oct-19: Clarify documentation of WriterWriteFn.
  CJB: 10-Jul-20: Added signed 16-bit and unsigned 32-bit write functions.
  CJB: 28-Jul-22: Removed redundant use of the 'extern' keyword.
*/

#ifndef Writer_h
#define Writer_h

/* ISO library header files */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

struct Writer;

typedef size_t WriterWriteFn(void const *ptr, size_t size,
  struct Writer *writer);
   /*
    * writes up to 'size' bytes to the data store abstracted by a given
    * writer object (without advancing the file position). If fewer than the
    * requested number of bytes were written then this function sets the
    * error indicator.
    * Returns: the number of bytes successfully written, which may be fewer
    *          than specified if a write error occurred.
    */

typedef bool WriterTermFn(struct Writer *writer);
   /*
    * destroys the type-specific part of an abstract writer object. Must
    * attempt to write any buffered user data and return false on failure.
    * If the error indicator is set then buffered data is discarded and false
    * is returned.
    * Returns: true if successful, otherwise false.
    */

typedef struct {
  WriterWriteFn *fwrite_fn;
  WriterTermFn *term_fn;
} WriterFns;

typedef struct Writer {
  unsigned int error:1, repos:1;
  void *data;
  long int fpos;
  long int flen;
  WriterFns fns;
} Writer;

bool writer_ferror(const Writer */*writer*/);
   /*
    * gets the current value of the error indicator for an abstract
    * writer object. This indicator is set if the error indicator is set for
    * the underlying stream (if any), or if encoding output data fails.
    * Returns: the current value of the error indicator.
    */

long int writer_ftell(const Writer */*writer*/);
   /*
    * gets the current value of the file position indicator for an abstract
    * writer object. The value is the number of bytes from the beginning
    * of the output data.
    * Returns: if successful, the current value of the file position indicator.
    *          On failure, the function returns -1L.
    */

int writer_fseek(Writer */*writer*/, long int /*offset*/, int /*whence*/);
   /*
    * sets the file position indicator for an abstract writer object.
    * The new position is at the number of bytes specified by 'offset'
    * away from the point specified by 'whence'. The offset can be negative
    * but seeking backward may fail (later, on writing data) if not supported
    * by the underlying stream.
    * The specified point is the beginning of the file for SEEK_SET or the
    * current file position for SEEK_CUR. SEEK_END is not supported.
    * Returns: 0 if successful or non-zero if the request is invalid.
    */

int writer_fputc(int /*c*/, Writer */*writer*/);
   /*
    * writes the byte specified by c to the data store abstracted by a
    * writer object. The byte (converted to 'unsigned char') is written at the
    * current position and the file position indicator is incremented by one.
    * Returns: the byte written if successful, otherwise EOF.
    */

size_t writer_fwrite(void const */*ptr*/, size_t /*size*/,
                     size_t /*nmemb*/, Writer */*writer*/);
   /*
    * writes up to 'nmemb' members of the array pointed to by 'ptr' to the
    * data store abstracted by a given writer object. The size of each member,
    * in bytes, is specified by 'size'. The file position indicator is
    * advanced by the number of bytes successfully written. If fewer than the
    * requested number of members were written then this function sets the
    * error indicator.
    * Returns: the number of members successfully written, which may be fewer
    *          than specified if a write error occurred.
    */

bool writer_fwrite_uint16(uint16_t /*val*/, Writer */*writer*/);
   /*
    * writes an unsigned 16-bit integer passed as 'val' to the data store
    * abstracted by a given writer object and advances the file position
    * indicator by four bytes. If writing fails then this function instead
    * sets the error indicator and returns false.
    * Returns: true if successful, otherwise false.
    */

bool writer_fwrite_int16(int16_t /*val*/, Writer */*writer*/);
   /*
    * writes a signed 16-bit integer passed as 'val' to the data store
    * abstracted by a given writer object and advances the file position
    * indicator by four bytes. If writing fails then this function instead
    * sets the error indicator and returns false.
    * Returns: true if successful, otherwise false.
    */

bool writer_fwrite_uint32(uint32_t /*val*/, Writer */*writer*/);
   /*
    * writes an unsigned 32-bit integer passed as 'val' to the data store
    * abstracted by a given writer object and advances the file position
    * indicator by four bytes. If writing fails then this function instead
    * sets the error indicator and returns false.
    * Returns: true if successful, otherwise false.
    */

bool writer_fwrite_int32(int32_t /*val*/, Writer */*writer*/);
   /*
    * writes a signed 32-bit integer passed as 'val' to the data store
    * abstracted by a given writer object and advances the file position
    * indicator by four bytes. If writing fails then this function instead
    * sets the error indicator and returns false.
    * Returns: true if successful, otherwise false.
    */

void writer_internal_init(Writer */*writer*/,
  WriterFns const */*fns*/, void */*data*/);
   /*
    * initializes an abstract writer object. This function is for internal use
    * only by those implementing a new type of writer.
    */

long int writer_destroy(Writer */*writer*/);
   /*
    * flushes any buffered output data and destroys an abstract writer object.
    * Any internal buffers are freed but it is the caller's responsibility to
    * free the storage for the 'writer' object itself, if desired.
    * Any memory buffer or file handle passed upon initialization of the writer
    * will not be freed or closed, even if the error indicator is set.
    * The final length of the output data may differ from the number of bytes
    * written (if writes overlapped) and/or the final value of the file
    * position indicator. That is why it is returned by this function.
    * Returns: if successful, the length of the output data (in bytes).
    *          On failure, the function returns -1L.
    */

#endif /* Writer_h */
