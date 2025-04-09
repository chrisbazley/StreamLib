/*
 * StreamLib: Generic memory buffer writer
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
  CJB: 25-Aug-19: Created this source file.
  CJB: 01-Sep-19: First released version.
  CJB: 28-Jul-22: Removed redundant use of 'extern' and 'const'.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

#ifndef WriterMem_h
#define WriterMem_h

/* ISO library header files */
#include <stdbool.h>
#include <stddef.h>

/* Local header files */
#include "Writer.h"

#if !defined(USE_OPTIONAL) && !defined(_Optional)
#define _Optional
#endif

bool writer_mem_init(Writer */*writer*/,
  _Optional void */*buffer*/, size_t /*buffer_size*/);
   /*
    * creates an abstract writer object to allow data to be stored in the
    * array pointed to by 'buffer' as if it were stored in a file.
    * The maximum number of bytes that can be written is specified by
    * 'buffer_size'. Functions attempting to write beyond the end of the
    * array will return an error value and set the error indicator.
    * Returns: true if successful, otherwise false. Can only fail because
    *          of a lack of free memory.
    */

#endif /* WriterMem_h */
