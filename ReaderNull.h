/*
 * StreamLib: Null file reader
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
  CJB: 30-Aug-19: Created this source file.
  CJB: 06-Sep-19: First released version.
  CJB: 28-Jul-22: Removed redundant use of the 'extern' keyword.
*/

#ifndef ReaderNull_h
#define ReaderNull_h

/* Local header files */
#include "Reader.h"

void reader_null_init(Reader */*reader*/);
   /*
    * creates an abstract reader object to allow zero bytes to be read as
    * if from an empty file.
    */

#endif /* ReaderNull_h */
