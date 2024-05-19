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

/*
Dependencies: ANSI C library
Message tokens: None.
History:
  CJB: 24-Aug-19: Created this source file.
  CJB: 01-Sep-19: First released version.
  CJB: 28-Jul-22: Removed redundant use of the 'extern' keyword.
*/

#ifndef ReaderMem_h
#define ReaderMem_h

/* ISO library header files */
#include <stdbool.h>
#include <stddef.h>

/* Local header files */
#include "Reader.h"

bool reader_mem_init(Reader */*reader*/,
                     const void */*buffer*/,
                     size_t /*buffer_size*/);
   /*
    * creates an abstract reader object to allow data to be read from the
    * array pointed to by 'buffer' as if it were stored in a file.
    * The maximum number of bytes that can be read is specified by
    * 'buffer_size'. Functions attempting to read beyond the end of the
    * array will return an error value and set the end-of-file indicator.
    * Returns: true if successful, otherwise false. Can only fail because
    *          of a lack of free memory.
    */

#endif /* ReaderMem_h */
