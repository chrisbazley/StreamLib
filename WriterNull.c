/*
 * StreamLib: Null file writer
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
  CJB: 29-Aug-19: Created this source file.
  CJB: 07-Sep-19: First released version.
  CJB: 09-Apr-25: Dogfooding the _Optional qualifier.
*/

/* ISO library header files */
#include <limits.h>

/* Local headers */
#include "WriterNull.h"
#include "Internal/StreamMisc.h"

static size_t writer_null_fwrite(void const *ptr,
  size_t const size, Writer * const writer)
{
  NOT_USED(writer);
  NOT_USED(ptr);
  return size;
}

static bool writer_null_destroy(Writer * const writer)
{
  NOT_USED(writer);
  return true;
}

void writer_null_init(Writer * const writer)
{
  assert(writer != NULL);
  static WriterFns const fns = {writer_null_fwrite, writer_null_destroy};
  writer_internal_init(writer, &fns, writer);
}
