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

/*
Dependencies: ANSI C library.
Message tokens: None.
History:
  CJB: 07-Aug-18: Copied this source file from SF3KtoObj.
  CJB: 01-Sep-19: Added function documentation.
  CJB: 21-Sep-19: Add missing #include.
  CJB: 28-Jul-22: Removed redundant use of the 'extern' keyword.
*/

#ifndef ReaderGKey_h
#define ReaderGKey_h

/* ISO library header files */
#include <stdio.h>
#include <stdbool.h>

/* Local header files */
#include "Reader.h"

bool reader_gkey_init(Reader */*reader*/,
                      unsigned int /*history_log_2*/,
                      FILE */*in*/);
   /*
    * creates an abstract reader object to allow the contents of a file that
    * has been encoded in Gordon Key's compressed format to be read as
    * though it were not thus encoded. The 'history_log_2' parameter is the
    * number of bytes to look behind, in base 2 logarithmic form, and must
    * be the same as that used to compress the data.
    * Returns: true if successful, otherwise false. Can only fail because of
    *          lack of free memory.
    */

bool reader_gkey_init_from(Reader */*reader*/,
                           unsigned int /*history_log_2*/,
                           Reader */*in*/);
   /*
    * creates an abstract reader object to allow data from the reader object
    * pointed to by 'in' to be decompressed on the fly, assuming that the
    * data is encoded in Gordon Key's compressed format. The 'history_log_2'
    * parameter is the number of bytes for the decompressor to look behind,
    * in base 2 logarithmic form, and must be the same as that used to
    * compress the data.
    * Returns: true if successful, otherwise false. Can only fail because of
    *          lack of free memory.
    */

#endif /* ReaderGKey_h */
