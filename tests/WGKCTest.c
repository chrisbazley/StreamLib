/*
 * StreamLib test: Gordon Key compressed file size estimator
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

/* ISO library headers */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* StreamLib headers */
#include "WriterGKC.h"
#include "WriterGKey.h"
#include "WriterNull.h"

/* Local headers */
#include "Tests.h"

enum
{
  MaxHistoryLog2 = 12,
  LongDataSize = 1024,
  MinSize = LongDataSize + 999,
};

static void test1(void)
{
  /* Estimated size */
  Writer null, gkey, gkc, gkc_min;
  long int out_size = LONG_MIN, out_size_with_min = LONG_MIN;
  char const *const string = "PLEASE DO NOT BEND / BITTE NICHT BIEGEN / NE PAS PLIER";
  size_t const len = strlen(string);

  for (unsigned int hist_log2 = 0; hist_log2 <= MaxHistoryLog2; hist_log2++)
  {
    writer_null_init(&null);
    assert(writer_gkey_init_from(&gkey, hist_log2, 0, &null));
    assert(writer_gkc_init(&gkc, hist_log2, &out_size));
    assert(writer_gkc_init_with_min(&gkc_min, hist_log2, 0, &out_size_with_min));

    for (size_t total = 0; total < LongDataSize; total += len)
    {
      size_t const rem = LongDataSize - total;
      size_t const nmemb = len > rem ? rem : len;

      assert(writer_fwrite(string, 1, nmemb, &gkc) == nmemb);
      assert(writer_fwrite(string, 1, nmemb, &gkc_min) == nmemb);
      assert(writer_fwrite(string, 1, nmemb, &gkey) == nmemb);
    }

    assert(writer_destroy(&gkc) == LongDataSize);
    assert(writer_destroy(&gkc_min) == LongDataSize);
    assert(writer_destroy(&gkey) == LongDataSize);

    printf("History log2 %u, output size %ld\n",
      hist_log2, out_size);

    assert(writer_destroy(&null) == out_size);
    assert(out_size == out_size_with_min);
  }
}

static void test2(void)
{
  /* Estimated size with minimum */
  Writer null, gkey, gkc;
  long int out_size = LONG_MIN;
  char const *const string = "PLEASE DO NOT BEND / BITTE NICHT BIEGEN / NE PAS PLIER";
  size_t const len = strlen(string);

  for (unsigned int hist_log2 = 0; hist_log2 <= MaxHistoryLog2; hist_log2++)
  {
    writer_null_init(&null);
    assert(writer_gkey_init_from(&gkey, hist_log2, MinSize, &null));
    assert(writer_gkc_init_with_min(&gkc, hist_log2, MinSize, &out_size));

    for (size_t total = 0; total < LongDataSize; total += len)
    {
      size_t const rem = LongDataSize - total;
      size_t const nmemb = len > rem ? rem : len;

      assert(writer_fwrite(string, 1, nmemb, &gkey) == nmemb);
      assert(writer_fwrite(string, 1, nmemb, &gkc) == nmemb);
    }

    assert(writer_destroy(&gkc) == LongDataSize);
    assert(writer_destroy(&gkey) == LongDataSize);

    printf("History log2 %u, output size %ld\n",
      hist_log2, out_size);

    assert(writer_destroy(&null) == out_size);
  }
}

void WriterGKC_tests(void)
{
  static const struct
  {
    const char *test_name;
    void (*test_func)(void);
  }
  unit_tests[] =
  {
    { "Estimated size", test1 },
    { "Estimated size with minimum", test2 },
  };

  for (size_t count = 0; count < ARRAY_SIZE(unit_tests); count ++)
  {
    printf("Test %zu/%zu : %s\n",
           1 + count,
           ARRAY_SIZE(unit_tests),
           unit_tests[count].test_name);

    Fortify_EnterScope();

    unit_tests[count].test_func();

    Fortify_LeaveScope();
  }
}
