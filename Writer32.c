/*
 * StreamLib: Write 32-bit integers
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
  CJB: 10-Jul-20: Added unsigned 32-bit write function.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

/* Local headers */
#include "Writer.h"
#include "Internal/StreamMisc.h"

bool writer_fwrite_uint32(uint32_t const val, Writer * const writer)
{
  unsigned char const bytes[sizeof(val)] = {
    val, val >> 8, val >> 16, val >> 24
  };
  return writer_fwrite(&bytes, sizeof(bytes), 1, writer) == 1;
}

bool writer_fwrite_int32(int32_t const val, Writer * const writer)
{
  return writer_fwrite_uint32(val, writer);
}
