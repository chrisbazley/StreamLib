/*
 * StreamLib: Raw file reader
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
  CJB: 28-Jul-22: Removed redundant use of the 'extern' keyword.
*/

#ifndef ReaderRaw_h
#define ReaderRaw_h

/* ISO library header files */
#include <stdio.h>

/* Local header files */
#include "Reader.h"

void reader_raw_init(Reader */*reader*/, FILE */*in*/);
   /*
    * creates an abstract reader object to allow the contents of a file
    * to be read through an interface that can also abstract other data
    * sources.
    */

#endif /* ReaderRaw_h */
