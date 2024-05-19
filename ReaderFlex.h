/*
 * StreamLib: Flex memory buffer reader
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
  CJB: 24-Aug-19: Created this source file.
  CJB: 01-Sep-19: First released version.
  CJB: 29-Sep-19: Corrected documentation of reader_flex_init.
  CJB: 28-Jul-22: Removed redundant use of the 'extern' keyword.
*/

#ifndef ReaderFlex_h
#define ReaderFlex_h

/* Acorn C/C++ header files */
#include "flex.h"

/* Local header files */
#include "Reader.h"

void reader_flex_init(Reader */*reader*/, flex_ptr /*anchor*/);
   /*
    * creates an abstract reader object to allow a data store allocated by
    * Acorn's flex library to be read like the contents of a file. The
    * caller need not pass the number of bytes allocated because it is
    * queried from the flex library instead. Functions attempting to read
    * beyond the end of the allocated store will return an error value and
    * set the end-of-file indicator. Movement of the flex store is
    * automatically disabled while it is being accessed.
    */

#endif /* ReaderFlex_h */
