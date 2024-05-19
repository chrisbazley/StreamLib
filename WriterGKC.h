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

/*
Dependencies: ANSI C library.
Message tokens: None.
History:
  CJB: 08-Sep-19: Created this source file.
  CJB: 21-Nov-19: Changed the output of writer_gkc_init from int32_t to
                  long int.
  CJB: 27-Sep-20: Added support for padding the end of the input to reach
                  a specified minimum size.
  CJB: 28-Jul-22: Removed redundant use of the 'extern' keyword.
*/

#ifndef WriterGKC_h
#define WriterGKC_h

/* ISO library header files */
#include <stdbool.h>

/* Local header files */
#include "Writer.h"

bool writer_gkc_init_with_min(Writer */*writer*/,
                              unsigned int /*history_log_2*/,
                              long int /*min_size*/,
                              long int */*out_size*/);
   /*
    * creates an abstract writer object to estimate the size of data that
    * has been encoded in Gordon Key's compressed format.
    * The 'history_log_2' parameter is the number of bytes to look behind,
    * in base 2 logarithmic form, and must be the same as that used to
    * decompress the data.
    * 'min_size' is the minimum size of the input data, in bytes. If the
    * number of bytes written is less than 'min_size' then trailing zeros
    * are appended to pad the input to the requested size.
    * 'out_size' points to an object in which to store the size of the
    * compressed data, in bytes. The compressed size isn't available until
    * the writer has been destroyed (and only then if writer_destroy
    * returns the uncompressed file size rather than -1).
    * Returns: true if successful, otherwise false. Can only fail because
    *          of lack of free memory.
    */

bool writer_gkc_init(Writer */*writer*/,
                     unsigned int /*history_log_2*/,
                     long int */*out_size*/);
   /*
    * creates an abstract writer object to estimate the size of data that
    * has been encoded in Gordon Key's compressed format. This function is
    * similar to writer_gkc_init_min except that the input data has no
    * minimum size and therefore cannot be implicitly padded with zeros.
    * Returns: true if successful, otherwise false. Can only fail because
    *          of lack of free memory.
    */

#endif /* WriterGKC_h */
