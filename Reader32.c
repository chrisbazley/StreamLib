/*
 * StreamLib: Read 32-bit integers
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
  CJB: 10-Jul-20: Added unsigned 32-bit read function.
*/

/* ISO library header files */
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

/* Local headers */
#include "Internal/StreamMisc.h"
#include "Reader.h"

bool reader_fread_uint32(uint32_t * const ptr, Reader * const reader)
{
  assert(ptr != NULL);

  unsigned char bytes[sizeof(*ptr)];
  if (reader_fread(&bytes, sizeof(bytes), 1, reader) != 1) {
    return false;
  }

  *ptr = bytes[0] | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);

  return true;
}

bool reader_fread_int32(int32_t * const ptr, Reader * const reader)
{
  uint32_t val;
  if (!reader_fread_uint32(&val, reader)) {
    return false;
  }

  if (val <= INT32_MAX) {
    *ptr = val;
  } else {
    /* Beware that -INT32_MIN may be unrepresentable as int32_t. */
    uint32_t const neg = -val;
    if (neg <= INT32_MAX) {
      *ptr = -(int32_t)neg;
    } else {
      *ptr = INT32_MIN;
    }
  }
  return true;
}
