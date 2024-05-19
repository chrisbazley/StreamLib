/*
 * StreamLib: Flex memory buffer writer
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
Dependencies: ANSI C library, Acorn's Flex library.
Message tokens: None.
History:
  CJB: 25-Aug-19: Created this source file.
  CJB: 01-Sep-19: First released version.
  CJB: 17-Nov-19: Deleted a non-existent return value's description.
  CJB: 28-Jul-22: Removed redundant use of 'extern' and 'const'.
*/

#ifndef WriterFlex_h
#define WriterFlex_h

/* Acorn C/C++ header files */
#include "flex.h"

/* Local header files */
#include "Writer.h"

void writer_flex_init(Writer */*writer*/, flex_ptr /*anchor*/);
   /*
    * creates an abstract writer object to allow data to be stored in a
    * buffer allocated by Acorn's flex library, as if it were stored in a
    * file. Passing in an 'anchor' allows the writer object to grow the
    * buffer as necessary (by calling flex_extend) without taking ownership
    * of it. The caller need not pass the number of bytes allocated because
    * it is queried from the flex library instead.
    */

#endif /* WriterFlex_h */
