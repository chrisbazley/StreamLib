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

/*
Dependencies: ANSI C library.
Message tokens: None.
History:
  CJB: 07-Aug-18: Copied this source file from SF3KtoObj.
  CJB: 02-Sep-19: reader_feof and reader_ferror are actual functions instead
                  of macros. Added function documentation.
  CJB: 10-Jul-20: Added signed 16-bit and unsigned 32-bit read functions.
  CJB: 28-Jul-22: Removed redundant use of the 'extern' keyword.
*/

#ifndef Reader_h
#define Reader_h

/* ISO library header files */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

struct Reader;

typedef size_t ReaderReadFn(void *ptr, size_t size,
  struct Reader *reader);
   /*
    * reads up to 'size' bytes from the data store abstracted by a given
    * reader object (without advancing the file position). If fewer than the
    * requested number of bytes were read then this function sets the
    * error or end-of-file indicator as appropriate.
    * Returns: the number of bytes successfully read, which may be fewer
    *          than specified if a read error or end-of-file occurred.
    */

typedef void ReaderTermFn(struct Reader *reader);
   /*
    * destroys the type-specific part of an abstract reader object. Must
    * write any buffered user data and set the error indicator on failure.
    * Should not attempt to write buffered data if the error indicator is
    * already set.
    */

typedef struct {
  ReaderReadFn *fread_fn;
  ReaderTermFn *term_fn;
} ReaderFns;

typedef struct Reader {
  unsigned int error:1, eof:1, repos:1;
  void *data;
  int pushed_back;
  long int fpos;
  ReaderFns fns;
} Reader;

bool reader_feof(const Reader */*reader*/);
   /*
    * gets the current value of the end-of-file indicator for an abstract
    * reader object. This indicator is set if the end-of-file indicator is set
    * for the underlying stream (if any), or if an attempt was made to read
    * beyond the end of any other data source.
    * Returns: the current value of the end-of-file indicator.
    */

bool reader_ferror(const Reader */*reader*/);
   /*
    * gets the current value of the error indicator for an abstract
    * reader object. This indicator is set if the error indicator is set for
    * the underlying stream (if any), or if decoding input data fails.
    * Returns: the current value of the error indicator.
    */

long int reader_ftell(const Reader */*reader*/);
   /*
    * gets the current value of the file position indicator for an abstract
    * reader object. The value is the number of bytes from the beginning
    * of the input data.
    * Returns: if successful, the current value of the file position indicator.
    *          On failure, the function returns -1L.
    */

int reader_fseek(Reader */*reader*/, long int /*offset*/, int /*whence*/);
   /*
    * sets the file position indicator for an abstract reader object.
    * The new position is at the number of bytes specified by 'offset'
    * away from the point specified by 'whence'. The offset can be negative
    * but seeking backward may fail (later, on reading data) if not supported
    * by the underlying stream.
    * The specified point is the beginning of the file for SEEK_SET or the
    * current file position for SEEK_CUR. SEEK_END is not supported.
    * This function also clears the end-of-file indicator and undoes any
    * effects of reader_ungetc on the same object.
    * Returns: 0 if successful or non-zero if the request is invalid.
    */

int reader_fgetc(Reader */*reader*/);
   /*
    * gets the next byte (if any) from a given abstract reader object, and
    * advances the file position indicator. The file position indicator is not
    * advanced if it was already at the end of the source data; instead, this
    * function sets the end-of-file indicator and returns EOF. It also returns
    * EOF if any other error occured.
    * Returns: the next byte from the input stream, or EOF if the
    *          operation failed. The byte is read as type 'unsigned char'.
    */

int reader_ungetc(int /*c*/, Reader */*reader*/);
   /*
    * pushes the byte specified by c back onto the input stream abstracted by
    * a reader object. The pushed-back byte (converted to 'unsigned char')
    * will be returned by the next read operation unless discarded by an
    * intervening call to reader_fseek. No more than one byte may be pushed
    * back.
    * If successful, this function clears the end-of-file indicator and
    * decrements the file position indicator. If the file position indicator
    * was previously zero then it has an undefined value after calling this
    * function.
    * Returns: the pushed-back byte after conversion, or EOF if the
    *          operation failed.
    */

size_t reader_fread(void */*ptr*/, size_t /*size*/,
                    size_t /*nmemb*/, Reader */*reader*/);
   /*
    * reads up to 'nmemb' members into the array pointed to by 'ptr' from a
    * given abstract reader object. The size of each member, in bytes, is
    * specified by 'size'. The file position indicator is advanced by the
    * number of bytes successfully read. If fewer than the requested number of
    * members were read then this function sets the end-of-file or error
    * indicator as appropriate.
    * Returns: the number of members successfully read, which may be fewer
    *          than specified if a read error or end-of-file occurred.
    */

bool reader_fread_uint16(uint16_t */*ptr*/, Reader */*reader*/);
   /*
    * reads an unsigned 16-bit integer into the storage pointed to by 'ptr'
    * from a given abstract reader object. If the file position is too close to
    * the end of the source data to read a complete integer then this function
    * instead sets the end-of-file indicator and returns false. Otherwise the
    * file position indicator is advanced by two bytes.
    * Returns: true if successful, otherwise false.
    */

bool reader_fread_int16(int16_t */*ptr*/, Reader */*reader*/);
   /*
    * reads a signed 16-bit integer into the storage pointed to by 'ptr'
    * from a given abstract reader object. If the file position is too close to
    * the end of the source data to read a complete integer then this function
    * instead sets the end-of-file indicator and returns false. Otherwise the
    * file position indicator is advanced by two bytes.
    * Returns: true if successful, otherwise false.
    */

bool reader_fread_uint32(uint32_t */*ptr*/, Reader */*reader*/);
   /*
    * reads an unsigned 32-bit integer into the storage pointed to by 'ptr'
    * from a given abstract reader object. If the file position is too close to
    * the end of the source data to read a complete integer then this function
    * instead sets the end-of-file indicator and returns false. Otherwise the
    * file position indicator is advanced by four bytes.
    * Returns: true if successful, otherwise false.
    */

bool reader_fread_int32(int32_t */*ptr*/, Reader */*reader*/);
   /*
    * reads a signed 32-bit integer into the storage pointed to by 'ptr'
    * from a given abstract reader object. If the file position is too close to
    * the end of the source data to read a complete integer then this function
    * instead sets the end-of-file indicator and returns false. Otherwise the
    * file position indicator is advanced by four bytes.
    * Returns: true if successful, otherwise false.
    */

void reader_internal_init(Reader */*reader*/,
  ReaderFns const */*fns*/, void */*data*/);
   /*
    * initializes an abstract reader object. This function is for internal use
    * only by those implementing a new type of reader.
    */

void reader_destroy(Reader */*reader*/);
   /*
    * destroys an abstract reader object. Any internal buffers are freed but it
    * is the caller's responsibility to free the storage for the 'reader'
    * object itself, if desired.
    * Any memory buffer or file handle passed upon initialization of the reader
    * will not be freed or closed, even if the error indicator is set.
    */

#endif /* Reader_h */
