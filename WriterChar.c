/*
 * StreamLib: Write characters
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
#include "Writer.h"
#include "Internal/StreamMisc.h"

int writer_fputc(int const c, Writer * const writer)
{
  unsigned char cc = c;
  assert(writer != NULL);
  return (writer_fwrite(&cc, sizeof(cc), 1, writer) == 1) ? c : EOF;
}
