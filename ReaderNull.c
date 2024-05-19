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

/* History:
  CJB: 30-Aug-19: Created this source file.
  CJB: 07-Sep-19: First released version.
*/

/* ISO library header files */
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Local headers */
#include "Internal/StreamMisc.h"
#include "ReaderNull.h"

static size_t reader_null_fread(void *ptr, size_t const size,
                                Reader * const reader)
{
  NOT_USED(ptr);
  NOT_USED(size);
  assert(reader != NULL);
  reader->eof = 1;
  return 0;
}

static void reader_null_destroy(Reader * const reader)
{
  NOT_USED(reader);
}

void reader_null_init(Reader *const reader)
{
  assert(reader != NULL);
  static ReaderFns const fns = {reader_null_fread, reader_null_destroy};
  reader_internal_init(reader, &fns, NULL);
}
