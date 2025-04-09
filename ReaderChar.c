/*
 * StreamLib: Read characters
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

/* History:
  CJB: 05-Nov-19: Split into a separate compilation unit.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <stdio.h>

/* Local headers */
#include "Reader.h"
#include "Internal/StreamMisc.h"

int reader_fgetc(Reader * const reader)
{
  unsigned char c;
  assert(reader != NULL);
  return (reader_fread(&c, sizeof(c), 1, reader) == 1) ? c : EOF;
}

int reader_ungetc(int const c, Reader * const reader)
{
  int pushed;
  assert(reader != NULL);
  if ((reader->pushed_back != EOF) || (c == EOF)) {
    /* We already pushed back a character and can't push back another.
     */
    pushed = EOF;
  } else {
    /* Push back the specified character and return it converted to
       unsigned char. */
    reader->pushed_back = c;
    reader->eof = 0;
    pushed = (unsigned char)c;
  }
  return pushed;
}
