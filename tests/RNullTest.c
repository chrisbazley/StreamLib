/*
 * StreamLib test: Null file reader
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
#include "ReaderNull.h"

/* Local headers */
#include "Tests.h"

#define PATH "<Wimp$ScrapDir>.ReaderTest"
#define TEST_STR "qwerty"

enum
{
  NumberOfReaders = 5,
  NumReads = 7,
  Marker = 56,
};

static void test1(void)
{
  /* Init/term */
  Reader r[NumberOfReaders];

  for (size_t i = 0; i < ARRAY_SIZE(r); i++)
  {
    reader_null_init(&r[i]);

    assert(!reader_feof(&r[i]));
    assert(!reader_ferror(&r[i]));
    assert(reader_ftell(&r[i]) == 0);
  }

  for (size_t i = 0; i < ARRAY_SIZE(r); i++)
    reader_destroy(&r[i]);
}

static void test2(void)
{
  /* Get char */
  Reader r;
  reader_null_init(&r);

  assert(reader_fgetc(&r) == EOF);
  assert(reader_ftell(&r) == 0);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);
}

static void test5(void)
{
  /* Unget EOF */
  Reader r;
  reader_null_init(&r);

  assert(reader_ungetc(EOF, &r) == EOF);
  /* Operation fails and the input stream is unchanged */
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);
  assert(reader_ftell(&r) == 0);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);
}

static void test7(void)
{
  /* Unget two chars */
  Reader r;
  reader_null_init(&r);

  assert(reader_ungetc('y', &r) == 'y');
  /* If called too many times without a read or file repositioning
     then the operation may fail */
  assert(reader_ungetc('z', &r) == EOF);

  /* If the file position indicator was zero before the
     ungetc call then it is indeterminate afterwards */
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == 'y');
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);
}

static void test8(void)
{
  /* Read one */
  Reader r;
  reader_null_init(&r);

  int buf[NumReads + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  for (size_t i = 0; i < NumReads; ++i) {
    assert(reader_fread(buf, sizeof(buf[0]), 1, &r) == 0);
    assert(reader_ftell(&r) == 0);
    assert(reader_feof(&r));
    assert(!reader_ferror(&r));

    for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
      assert(buf[n] == Marker);
    }
  }

  reader_destroy(&r);
}

static void test9(void)
{
  /* Read multiple */
  Reader r;
  reader_null_init(&r);

  int buf[NumReads + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  assert(reader_fread(buf, sizeof(buf[0]), NumReads, &r) == 0);
  assert(reader_ftell(&r) == 0);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    assert(buf[n] == Marker);
  }

  reader_destroy(&r);
}

static void test10(void)
{
  /* Read zero */
  Reader r;
  reader_null_init(&r);

  int buf[NumReads + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  assert(reader_fread(buf, sizeof(buf[0]), 0, &r) == 0);
  /* fread returns zero and the contents of the array
     and the state of the stream remain unchanged */
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    assert(buf[n] == Marker);
  }

  reader_destroy(&r);
}

static void test11(void)
{
  /* Read zero size */
  Reader r;
  reader_null_init(&r);

  int buf[NumReads + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  assert(reader_fread(buf, 0, NumReads, &r) == 0);
  /* fread returns zero and the contents of the array
     and the state of the stream remain unchanged */
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    assert(buf[n] == Marker);
  }

  reader_destroy(&r);
}

static void test15(void)
{
  /* Read ui16 */
  Reader r;
  reader_null_init(&r);

  uint16_t buf[2];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  for (size_t x = 0; x < NumReads; ++x) {
    assert(!reader_fread_uint16(buf, &r));

    assert(reader_ftell(&r) == 0);
    assert(reader_feof(&r));
    assert(!reader_ferror(&r));

    for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
      assert(buf[n] == Marker);
    }
  }

  reader_destroy(&r);
}

static void test16(void)
{
  /* Read i32 */
  Reader r;
  reader_null_init(&r);

  int32_t buf[2];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  for (size_t x = 0; x < NumReads; ++x) {
    assert(!reader_fread_int32(buf, &r));

    assert(reader_ftell(&r) == 0);
    assert(reader_feof(&r));
    assert(!reader_ferror(&r));

    for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
      assert(buf[n] == Marker);
    }
  }

  reader_destroy(&r);
}

static void test17(void)
{
  /* Unget at start */
  Reader r;
  reader_null_init(&r);

  assert(reader_ftell(&r) == 0);
  const int push = -12; /* converted to unsigned char */
  assert(reader_ungetc(push, &r) == (unsigned char)push);

  /* If the file position indicator was zero before the
     ungetc call then it is indeterminate afterwards */
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == (unsigned char)push);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));
  assert(reader_ftell(&r) == 0);

  assert(reader_fgetc(&r) == EOF);

  reader_destroy(&r);
}

static void test18(void)
{
  /* Seek forward from current */
  Reader r;
  reader_null_init(&r);

  assert(!reader_fseek(&r, 2, SEEK_CUR));
  assert(reader_ftell(&r) == 2);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);

  reader_destroy(&r);
}

static void test19(void)
{
  /* Seek current */
  Reader r;
  reader_null_init(&r);

  assert(!reader_fseek(&r, 0, SEEK_CUR));
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);

  reader_destroy(&r);
}

static void test21(void)
{
  /* Seek forward from current after unget */
  Reader r;
  reader_null_init(&r);

  assert(reader_ungetc('W', &r) == 'W');

  assert(!reader_fseek(&r, 3, SEEK_CUR));

  /* If the file position indicator was zero before the
     ungetc call then it is indeterminate afterwards */
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);

  reader_destroy(&r);
}

static void test22(void)
{
  /* Seek beyond start from current */
  Reader r;
  reader_null_init(&r);

  assert(reader_fseek(&r, -2, SEEK_CUR));
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);

  reader_destroy(&r);
}

static void test24(void)
{
  /* Seek back from start */
  Reader r;
  reader_null_init(&r);

  assert(reader_fseek(&r, -1, SEEK_SET));
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);

  reader_destroy(&r);
}

static void test25(void)
{
  /* Seek forward from start */
  Reader r;
  reader_null_init(&r);

  assert(!reader_fseek(&r, strlen(TEST_STR)-1l, SEEK_SET));
  assert(reader_ftell(&r) == strlen(TEST_STR)-1l);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);

  reader_destroy(&r);
}

static void test27(void)
{
  /* Seek from end (expected to fail) */
  Reader r;
  reader_null_init(&r);

  assert(reader_fseek(&r, 0, SEEK_END));
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);

  reader_destroy(&r);
}

void ReaderNull_tests(void)
{
  static const struct
  {
    const char *test_name;
    void (*test_func)(void);
  }
  unit_tests[] =
  {
    { "Init/term", test1 },
    { "Get char", test2 },
    { "Unget EOF", test5 },
    { "Unget two chars", test7 },
    { "Read one", test8 },
    { "Read multiple", test9 },
    { "Read zero", test10 },
    { "Read zero size", test11 },
    { "Read ui16", test15 },
    { "Read i32", test16 },
    { "Unget at start", test17 },
    { "Seek forward from current", test18 },
    { "Seek current", test19 },
    { "Seek forward from current after unget", test21 },
    { "Seek beyond start from current", test22 },
    { "Seek back from start", test24 },
    { "Seek forward from start", test25 },
    { "Seek from end", test27 },
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
