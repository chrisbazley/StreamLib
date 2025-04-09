/*
 * StreamLib: Read 16-bit integers
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
  CJB: 10-Jul-20: Added signed 16-bit read function.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

/* Local headers */
#include "Reader.h"
#include "Internal/StreamMisc.h"

bool reader_fread_uint16(uint16_t * const ptr, Reader * const reader)
{
  assert(ptr != NULL);

  unsigned char bytes[sizeof(*ptr)];
  if (reader_fread(&bytes, sizeof(bytes), 1, reader) != 1) {
    return false;
  }

  *ptr = bytes[0] | ((uint16_t)bytes[1] << 8);

  return true;
}

bool reader_fread_int16(int16_t * const ptr, Reader * const reader)
{
  uint16_t val;
  if (!reader_fread_uint16(&val, reader)) {
    return false;
  }
  unsigned int const v2 = val;

  if (v2 <= INT16_MAX) {
    *ptr = v2;
  } else {
    /* Beware that -INT16_MIN may be unrepresentable as int16_t. */
    unsigned int const neg = -v2;
    if (neg <= INT16_MAX) {
      *ptr = -(int)neg;
    } else {
      *ptr = INT16_MIN;
    }
  }
  return true;
}
