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

/*
Dependencies: ANSI C library.
Message tokens: None.
History:
  CJB: 11-Aug-19: Created this source file.
  CJB: 01-Sep-19: First released version.
  CJB: 21-Sep-19: Add missing #include.
  CJB: 12-Nov-19: Pass long int instead of int32_t as the minimum file size.
  CJB: 27-Sep-20: Clarified that the input is padded to a minimum size, not
                  the compressed output.
  CJB: 28-Jul-22: Removed redundant use of the 'extern' keyword.
*/

#ifndef WriterGKey_h
#define WriterGKey_h

/* ISO library header files */
#include <stdio.h>
#include <stdbool.h>

/* Local header files */
#include "Writer.h"

bool writer_gkey_init_from(Writer */*writer*/,
                           unsigned int /*history_log_2*/,
                           long int /*min_size*/,
                           Writer */*out*/);
   /*
    * creates an abstract writer object to allow data to be encoded in
    * Gordon Key's compressed format before being written to the writer
    * object pointed to by 'out'. The 'history_log_2' parameter is the
    * number of bytes to look behind, in base 2 logarithmic form, and must
    * be the same as that used to decompress the data.
    * 'min_size' is the minimum size of the input data, in bytes. If the
    * number of bytes written later exceeds 'min_size' then the value stored
    * in the output data is overwritten when the writer is destroyed. This
    * operation may fail if seeking backwards is not supported. If the
    * number of bytes written is less than 'min_size' then trailing zeros
    * are instead appended to pad the input to the requested size.
    * Returns: true if successful, otherwise false. Can only fail because of
    *          lack of free memory.
    */

bool writer_gkey_init(Writer */*writer*/,
                      unsigned int /*history_log_2*/,
                      long int /*min_size*/,
                      FILE */*out*/);
   /*
    * creates an abstract writer object to allow data to be encoded in
    * Gordon Key's compressed format before being written to a file pointed
    * to by 'out'. This function is similar to writer_gkey_init_from except
    * that it implicitly creates a writer object to allow the file to be
    * written. (The second writer is implicitly destroyed with its parent.)
    * Returns: true if successful, otherwise false. Can only fail because of
    *          lack of free memory.
    */

#endif /* WriterGKey_h */
