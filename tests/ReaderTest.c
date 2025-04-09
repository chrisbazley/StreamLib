/*
 * StreamLib test: File reader
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

/* ISO library headers */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GKeyLib headers */
#include "GKeyComp.h"

/* StreamLib headers */
#include "ReaderRaw.h"
#include "ReaderGKey.h"
#ifdef ACORN_FLEX
#include "ReaderFlex.h"
#endif
#include "ReaderMem.h"

/* Local headers */
#include "Tests.h"

#define TEST_STR "qwerty"

enum
{
  NumberOfReaders = 5,
  HistoryLog2 = 9,
  FortifyAllocationLimit = 2048,
  BufferSize = 512,
  LongDataSize = 320, /* greater than internal buffer size */
  Marker = 56,
  Offset = 3
};

typedef enum  {
  READERTYPE_RAW,
  READERTYPE_GKEY,
#ifdef ACORN_FLEX
  READERTYPE_FLEX,
#endif
  READERTYPE_MEM,
  READERTYPE_COUNT
} ReaderType;

#ifdef ACORN_FLEX
static void *anchor;
#endif
static _Optional void *buffer;
static size_t buffer_size;
static _Optional FILE *f;
static char file_name[L_tmpnam];

static void make_file(ReaderType const rtype, const void *const data,
  size_t size, size_t const nmemb)
{
  switch (rtype) {
  case READERTYPE_RAW:
    tmpnam(file_name);
    f = fopen(file_name, "wb");
    if (f == NULL) perror("Failed to open file");
    assert(f != NULL);

    /* Standard C library cannot necessarily read size=0 */
    if (size > 0) {
      size_t const n = fwrite(data, size, nmemb, &*f);
      if (n != nmemb) { perror("Failed to write to file"); }
      assert(n == nmemb);
    }
    f = freopen(file_name, "rb", &*f);
    assert(f != NULL);
    break;

  case READERTYPE_GKEY:
    tmpnam(file_name);
    f = fopen(file_name, "wb");
    if (f == NULL) perror("Failed to open file");
    assert(f != NULL);

    size *= nmemb;

    assert(fputc(size & UCHAR_MAX, &*f) >= 0);
    assert(fputc((size >> CHAR_BIT) & UCHAR_MAX, &*f) >= 0);
    assert(fputc((size >> (CHAR_BIT * 2)) & UCHAR_MAX, &*f) >= 0);
    assert(fputc((size >> (CHAR_BIT * 3)) & UCHAR_MAX, &*f) >= 0);
    {
      _Optional GKeyComp *const comp = gkeycomp_make(HistoryLog2);
      GKeyStatus stat;
      char buf[BufferSize];
      GKeyParameters params = {
        .in_buffer = data,
        .in_size = size
      };
      do
      {
        params.out_buffer = buf;
        params.out_size = sizeof(buf);
        assert(comp);
        stat = gkeycomp_compress(&*comp, &params);
        /* Standard C library cannot necessarily read size=0 */
        if (sizeof(buf) - params.out_size > 0) {
          assert(f);
          size_t const n = fwrite(buf, sizeof(buf) - params.out_size, 1, &*f);
          if (n != 1) { perror("Failed to write to file"); }
          assert(n == 1);
        }
      }
      while ((stat == GKeyStatus_OK) ||
             (stat == GKeyStatus_BufferOverflow));

      assert(stat == GKeyStatus_Finished);

      gkeycomp_destroy(comp);
    }
    assert(f != NULL);
    f = freopen(file_name, "rb", &*f);
    assert(f != NULL);
    break;

#ifdef ACORN_FLEX
  case READERTYPE_FLEX:
    size *= nmemb;
    assert(size <= INT_MAX);
    assert(!anchor);
    assert(flex_alloc(&anchor, (int)size));
    {
      int const bstate = flex_set_budge(0);
      memcpy(anchor, data, size);
      flex_set_budge(bstate);
    }
    break;
#endif

  case READERTYPE_MEM:
    size *= nmemb;
    assert(!buffer);
    buffer = malloc(size);
    buffer_size = size;
    assert(buffer != NULL);
    memcpy(&*buffer, data, size);
    break;

  default:
    abort();
    break;
  }
}

static void make_file_from_string(ReaderType const rtype, const char *const data)
{
  make_file(rtype, data, strlen(data), 1);
}

static void rewind_file(ReaderType const rtype)
{
  switch (rtype) {
  case READERTYPE_RAW:
  case READERTYPE_GKEY:
    assert(f);
    rewind(&*f);
    break;

#ifdef ACORN_FLEX
  case READERTYPE_FLEX:
#endif
  case READERTYPE_MEM:
    break;

  default:
    abort();
    break;
  }
}

static void delete_file(ReaderType const rtype)
{
  switch (rtype) {
  case READERTYPE_RAW:
  case READERTYPE_GKEY:
    assert(f);
    assert(!fclose(&*f));
    remove(file_name);
    f = NULL;
    break;

#ifdef ACORN_FLEX
  case READERTYPE_FLEX:
    flex_free(&anchor);
    break;
#endif

  case READERTYPE_MEM:
    free(buffer);
    buffer = NULL;
    buffer_size = 0;
    break;

  default:
    abort();
    break;
  }
}

static void init_reader(ReaderType const rtype, Reader *const r)
{
  switch (rtype) {
  case READERTYPE_RAW:
    assert(f);
    reader_raw_init(r, &*f);
    break;

  case READERTYPE_GKEY:
    assert(f);
    assert(reader_gkey_init(r, HistoryLog2, &*f));
    break;

#ifdef ACORN_FLEX
  case READERTYPE_FLEX:
    reader_flex_init(r, &anchor);
    break;
#endif

  case READERTYPE_MEM:
    assert(buffer);
    assert(reader_mem_init(r, &*buffer, buffer_size));
    break;

  default:
    abort();
    break;
  }
}

static void test1(ReaderType const rtype)
{
  /* Init/term */
  Reader r[NumberOfReaders];

  make_file_from_string(rtype, "x");

  for (size_t i = 0; i < ARRAY_SIZE(r); i++)
  {
    init_reader(rtype, &r[i]);

    assert(!reader_feof(&r[i]));
    assert(!reader_ferror(&r[i]));
    assert(reader_ftell(&r[i]) == 0);

    rewind_file(rtype);
  }

  for (size_t i = 0; i < ARRAY_SIZE(r); i++)
    reader_destroy(&r[i]);

  delete_file(rtype);
}

static void test2(ReaderType const rtype)
{
  /* Get char */
  Reader r;

  make_file_from_string(rtype, "x");

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == 'x');
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);
  assert(reader_ftell(&r) == 1);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);

  delete_file(rtype);
}

static void test3(ReaderType const rtype)
{
  /* Get char fail recovery */
  unsigned long limit;

  make_file_from_string(rtype, "xy");

  for (limit = 0; limit < FortifyAllocationLimit; ++limit)
  {
    Reader r;
    init_reader(rtype, &r);

    Fortify_SetNumAllocationsLimit(limit);
    int const c = reader_fgetc(&r);
    Fortify_SetNumAllocationsLimit(ULONG_MAX);

    assert(!reader_feof(&r));
    if (c == EOF) {
      assert(reader_ferror(&r));
      assert(reader_ftell(&r) == 0);
    } else {
      assert(c == 'x');
      assert(!reader_ferror(&r));
      assert(reader_ftell(&r) == 1);
    }

    reader_destroy(&r);

    if (c != EOF)
      break;

    rewind_file(rtype);
  }
  assert(limit != FortifyAllocationLimit);

  delete_file(rtype);
}

static void test4(ReaderType const rtype)
{
  /* Unget char */
  Reader r;

  make_file_from_string(rtype, "x");

  init_reader(rtype, &r);

  const int push = -12; /* converted to unsigned char */

  assert(reader_fgetc(&r) == 'x');
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_ungetc(push, &r) == (unsigned char)push);
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == (unsigned char)push);
  /* File position after reading or discarding all pushed-back
     characters shall be the same as it was before */
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);
  assert(reader_ftell(&r) == 1);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);

  delete_file(rtype);
}

static void test5(ReaderType const rtype)
{
  /* Unget EOF */
  Reader r;

  make_file_from_string(rtype, "x");

  init_reader(rtype, &r);

  assert(reader_ungetc(EOF, &r) == EOF);
  /* Operation fails and the input stream is unchanged */
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == 'x');
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);
  assert(reader_ftell(&r) == 1);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);

  delete_file(rtype);
}

static void test6(ReaderType const rtype)
{
  /* Unget char clears EOF */
  Reader r;

  make_file_from_string(rtype, "x");

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == 'x');
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);
  assert(reader_ftell(&r) == 1);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_ungetc('y', &r) == 'y');
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == 'y');
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == EOF);
  assert(reader_ftell(&r) == 1);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);

  delete_file(rtype);
}

static void test7(ReaderType const rtype)
{
  /* Unget two chars */
  Reader r;

  make_file_from_string(rtype, "x");

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == 'x');
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_ungetc('y', &r) == 'y');
  /* If called too many times without a read or file repositioning
     then the operation may fail */
  assert(reader_ungetc('z', &r) == EOF);
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == 'y');
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);

  delete_file(rtype);
}

static void test8(ReaderType const rtype)
{
  /* Read one */
  Reader r;
  static const int expected[] = {1232, -24243443, 0, -13};
  make_file(rtype, expected, sizeof(expected[0]), ARRAY_SIZE(expected));

  init_reader(rtype, &r);

  int buf[ARRAY_SIZE(expected) + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  for (size_t i = 0; i < ARRAY_SIZE(expected); ++i) {
    assert(reader_fread(buf, sizeof(buf[0]), 1, &r) == 1);
    assert(reader_ftell(&r) == (long)sizeof(buf[0]) * ((long)i+1l));
    assert(!reader_feof(&r));
    assert(!reader_ferror(&r));

    assert(buf[0] == expected[i]);
    buf[0] = Marker;
    for (size_t n = 1; n < ARRAY_SIZE(buf); ++n) {
      assert(buf[n] == Marker);
    }
  }

  assert(reader_fread(buf, sizeof(buf), 1, &r) == 0);
  assert(reader_ftell(&r) == sizeof(buf[0]) * ARRAY_SIZE(expected));
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    assert(buf[n] == Marker);
  }

  reader_destroy(&r);

  delete_file(rtype);
}

static void test9(ReaderType const rtype)
{
  /* Read multiple */
  Reader r;
  static const int expected[] = {1232, -24243443, 0, -13};
  make_file(rtype, expected, sizeof(expected[0]), ARRAY_SIZE(expected));

  init_reader(rtype, &r);

  int buf[ARRAY_SIZE(expected) + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  assert(reader_fread(buf, sizeof(buf[0]), ARRAY_SIZE(expected), &r) == ARRAY_SIZE(expected));
  assert(reader_ftell(&r) == ARRAY_SIZE(expected) * sizeof(buf[0]));
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = 0; n < ARRAY_SIZE(expected); ++n) {
    assert(buf[n] == expected[n]);
    buf[n] = Marker;
  }
  assert(buf[ARRAY_SIZE(expected)] == Marker);

  assert(reader_fread(buf, sizeof(buf[0]), 1, &r) == 0);
  assert(reader_ftell(&r) == ARRAY_SIZE(expected) * sizeof(buf[0]));
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    assert(buf[n] == Marker);
  }

  reader_destroy(&r);

  delete_file(rtype);
}

static void test10(ReaderType const rtype)
{
  /* Read zero */
  Reader r;
  static const int expected[] = {1232, -24243443, 0, -13};
  make_file(rtype, expected, sizeof(expected[0]), ARRAY_SIZE(expected));

  init_reader(rtype, &r);

  int buf[ARRAY_SIZE(expected) + 1];
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

  delete_file(rtype);
}

static void test11(ReaderType const rtype)
{
  /* Read zero size */
  Reader r;
  static const int expected[] = {1232, -24243443, 0, -13};
  make_file(rtype, expected, sizeof(expected[0]), ARRAY_SIZE(expected));

  init_reader(rtype, &r);

  int buf[ARRAY_SIZE(expected) + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  assert(reader_fread(buf, 0, ARRAY_SIZE(expected), &r) == 0);
  /* fread returns zero and the contents of the array
     and the state of the stream remain unchanged */
  assert(reader_ftell(&r) == 0);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    assert(buf[n] == Marker);
  }

  reader_destroy(&r);

  delete_file(rtype);
}

static void test12(ReaderType const rtype)
{
  /* Read past EOF */
  Reader r;
  static const int expected[] = {1232, -24243443, 0, -13};
  make_file(rtype, expected, sizeof(expected[0]), ARRAY_SIZE(expected));

  init_reader(rtype, &r);

  int buf[ARRAY_SIZE(expected) + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  assert(reader_fread(buf, sizeof(buf[0]), ARRAY_SIZE(buf), &r) == ARRAY_SIZE(expected));
  assert(reader_ftell(&r) == ARRAY_SIZE(expected) * sizeof(buf[0]));
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = 0; n < ARRAY_SIZE(expected); ++n) {
    assert(buf[n] == expected[n]);
  }
  assert(buf[ARRAY_SIZE(expected)] == Marker);

  reader_destroy(&r);

  delete_file(rtype);
}

static void test13(ReaderType const rtype)
{
  /* Read partial */
  Reader r;
  static const int expected[] = {1232, -24243443, 0, -13};
  make_file(rtype, expected, sizeof(expected) - 1, 1);

  init_reader(rtype, &r);

  int buf[ARRAY_SIZE(expected) + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  assert(reader_fread(buf, sizeof(buf[0]), ARRAY_SIZE(buf), &r) == ARRAY_SIZE(expected) - 1);
  assert(reader_ftell(&r) == sizeof(expected) - 1);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = 0; n < ARRAY_SIZE(expected) - 1; ++n) {
    assert(buf[n] == expected[n]);
  }
  /* If a partial member is read, its value is indeterminate. */
  assert(buf[ARRAY_SIZE(expected)] == Marker);

  reader_destroy(&r);

  delete_file(rtype);
}

static void test14(ReaderType const rtype)
{
  /* Read fail recovery */
  unsigned long limit;
  static const int expected[] = {1232, -24243443, 0, -13};
  make_file(rtype, expected, sizeof(expected[0]), ARRAY_SIZE(expected));

  int buf[ARRAY_SIZE(expected) + 1];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  for (limit = 0; limit < FortifyAllocationLimit; ++limit) {
    Reader r;
    init_reader(rtype, &r);

    Fortify_SetNumAllocationsLimit(limit);
    size_t const n = reader_fread(buf, sizeof(buf[0]),
      ARRAY_SIZE(expected), &r);

    Fortify_SetNumAllocationsLimit(ULONG_MAX);

    assert(!reader_feof(&r));
    if (n == 0) {
      assert(reader_ferror(&r));
    } else {
      assert(n == ARRAY_SIZE(expected));
      for (size_t i = 0; i < n; ++i) {
        assert(buf[i] == expected[i]);
      }
      assert(!reader_ferror(&r));
    }

    /* If an error occurs, the file position indicator is
       indeterminate but we shouldn't have read more members
       than requested. */
    for (size_t i = n; i < ARRAY_SIZE(buf); ++i) {
      assert(buf[i] == Marker);
    }
    reader_destroy(&r);

    if (n > 0)
      break;

    rewind_file(rtype);
  }
  assert(limit != FortifyAllocationLimit);

  delete_file(rtype);
}

static void test15(ReaderType const rtype)
{
  /* Read ui16 */
  Reader r;
  static const uint16_t e[] = {
    UINT16_MAX,
    UINT16_MAX - 1,
    0,
    1,
    0x1536 };

  unsigned char expected[sizeof(e)];
  size_t j = 0;
  for (size_t i = 0; i < ARRAY_SIZE(e); ++i) {
    for (size_t k = 0; k < sizeof(e[0]); ++k) {
      expected[j++] = e[i] >> (CHAR_BIT * k);
    }
  };

  make_file(rtype, expected, sizeof(expected[0]), ARRAY_SIZE(expected));

  init_reader(rtype, &r);

  uint16_t buf[2];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  for (size_t x = 0; x < ARRAY_SIZE(e); ++x) {
    assert(reader_fread_uint16(buf, &r));

    assert(reader_ftell(&r) == ((long)x + 1l) * (long)sizeof(e[0]));
    assert(!reader_feof(&r));
    assert(!reader_ferror(&r));

    assert(buf[0] == e[x]);
    buf[0] = Marker;
    assert(buf[1] == Marker);
  }

  assert(!reader_fread_uint16(buf, &r));
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    assert(buf[n] == Marker);
  }

  assert(reader_ftell(&r) == sizeof(expected));
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);
  delete_file(rtype);
}

static void test16(ReaderType const rtype)
{
  /* Read i32 */
  Reader r;
  static const int32_t e[] = {
    INT32_MAX,
    INT32_MIN,
    INT32_MAX - 1,
    INT32_MIN + 1,
    0,
    1,
    -1,
    0x7cf41536 };

  unsigned char expected[sizeof(e)];
  size_t j = 0;
  for (size_t i = 0; i < ARRAY_SIZE(e); ++i) {
    for (size_t k = 0; k < sizeof(e[0]); ++k) {
      expected[j++] = (uint32_t)e[i] >> (CHAR_BIT * k);
    }
  };

  make_file(rtype, expected, sizeof(expected[0]), ARRAY_SIZE(expected));

  init_reader(rtype, &r);

  int32_t buf[2];
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    buf[n] = Marker;
  }

  for (size_t x = 0; x < ARRAY_SIZE(e); ++x) {
    assert(reader_fread_int32(buf, &r));

    assert(reader_ftell(&r) == ((long)x+1l) * (long)sizeof(e[0]));
    assert(!reader_feof(&r));
    assert(!reader_ferror(&r));

    assert(buf[0] == e[x]);
    buf[0] = Marker;
    assert(buf[1] == Marker);
  }

  assert(!reader_fread_int32(buf, &r));
  for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
    assert(buf[n] == Marker);
  }

  assert(reader_ftell(&r) == sizeof(expected));
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);
  delete_file(rtype);
}

static void test17(ReaderType const rtype)
{
  /* Unget at start */
  Reader r;
  make_file_from_string(rtype, "");

  init_reader(rtype, &r);

  assert(reader_ftell(&r) == 0);
  const int push = -12; /* converted to unsigned char */
  assert(reader_ungetc(push, &r) == (unsigned char)push);

  /* If the file position indicator was zero before the
     call then it is indeterminate afterwards */
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == (unsigned char)push);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));
  assert(reader_ftell(&r) == 0);

  assert(reader_fgetc(&r) == EOF);
  assert(reader_feof(&r));
  assert(!reader_ferror(&r));
  assert(reader_ftell(&r) == 0);

  reader_destroy(&r);

  delete_file(rtype);
}

static void test18(ReaderType const rtype)
{
  /* Seek forward from current */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == TEST_STR[0]);

  assert(!reader_fseek(&r, 2, SEEK_CUR));
  assert(reader_ftell(&r) == 3);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == TEST_STR[3]);

  reader_destroy(&r);

  delete_file(rtype);
}

static void test19(ReaderType const rtype)
{
  /* Seek current */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == TEST_STR[0]);

  assert(!reader_fseek(&r, 0, SEEK_CUR));
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == TEST_STR[1]);

  reader_destroy(&r);

  delete_file(rtype);
}

static void test20(ReaderType const rtype)
{
  /* Seek back from current */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  for (size_t n = 0; n < strlen(TEST_STR); ++n) {
    assert(reader_fgetc(&r) == TEST_STR[n]);
  }

  assert(!reader_fseek(&r, -(long int)strlen(TEST_STR) + Offset, SEEK_CUR));
  assert(reader_ftell(&r) == Offset);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == TEST_STR[Offset]);

  reader_destroy(&r);

  delete_file(rtype);
}

static void test21(ReaderType const rtype)
{
  /* Seek forward from current after unget */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == TEST_STR[0]);
  assert(reader_fgetc(&r) == TEST_STR[1]);
  assert(reader_ungetc('W', &r) == 'W');

  assert(!reader_fseek(&r, 3, SEEK_CUR));
  assert(reader_ftell(&r) == 4);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == TEST_STR[4]);

  reader_destroy(&r);

  delete_file(rtype);
}

static void test22(ReaderType const rtype)
{
  /* Seek beyond start from current */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == TEST_STR[0]);
  assert(reader_fseek(&r, -2, SEEK_CUR));
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(reader_ferror(&r));

  reader_destroy(&r);

  if (rtype == READERTYPE_RAW) {
    rewind_file(rtype);
    assert(f);
    assert(!ferror(&*f));
    assert(fgetc(&*f) == TEST_STR[0]);
    assert(fseek(&*f, -2, SEEK_CUR));
    /* Result depends on standard C library */
    assert(ftell(&*f) == 1);
#ifdef ACORN_C
    assert(ferror(&*f));
#endif
    /* End of C library-dependent code */
    assert(!feof(&*f));
  }

  delete_file(rtype);
}

static void test23(ReaderType const rtype)
{
  /* Seek beyond end from current.
   * It may be impossible to determine the length of a file therefore the
   * behaviour when reading beyond EOF depends on the standard C library.
   */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == TEST_STR[0]);

  assert(!reader_fseek(&r, strlen(TEST_STR)*2l, SEEK_CUR));
  assert(reader_ftell(&r) == (strlen(TEST_STR)*2l) + 1l);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));
  assert(reader_fgetc(&r) == EOF);
  /* Result depends on standard C library */
#ifdef ACORN_C
  assert(!reader_feof(&r));
  assert(reader_ferror(&r));
#endif
  /* End of C library-dependent code */
  reader_destroy(&r);

  if (rtype == READERTYPE_RAW) {
    rewind_file(rtype);
    assert(f);
    assert(fgetc(&*f) == TEST_STR[0]);
    assert(!fseek(&*f, strlen(TEST_STR)*2l, SEEK_CUR));
    assert(ftell(&*f) == (strlen(TEST_STR)*2l) + 1l);
    assert(!feof(&*f));
    assert(!ferror(&*f));
    assert(fgetc(&*f) == EOF);
    /* Result depends on standard C library */
#ifdef ACORN_C
    assert(!feof(f));
    assert(ferror(f));
#endif
    /* End of C library-dependent code */
  }

  delete_file(rtype);
}

static void test24(ReaderType const rtype)
{
  /* Seek back from start */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == TEST_STR[0]);
  assert(reader_fseek(&r, -1, SEEK_SET));
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(reader_ferror(&r));

  reader_destroy(&r);

  if (rtype == READERTYPE_RAW) {
    rewind_file(rtype);
    assert(f);
    assert(fgetc(&*f) == TEST_STR[0]);
    assert(fseek(&*f, -1, SEEK_SET));
    /* Result depends on standard C library */
    assert(ftell(&*f) == 1);
#ifdef ACORN_C
    assert(ferror(&*f));
#endif
    /* End of C library-dependent code */
    assert(!feof(&*f));
  }

  delete_file(rtype);
}

static void test25(ReaderType const rtype)
{
  /* Seek forward from start */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == TEST_STR[0]);

  assert(!reader_fseek(&r, strlen(TEST_STR)-1l, SEEK_SET));
  assert(reader_ftell(&r) == strlen(TEST_STR)-1l);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == TEST_STR[strlen(TEST_STR)-1]);

  reader_destroy(&r);

  delete_file(rtype);
}

static void test26(ReaderType const rtype)
{
  /* Seek beyond end from start.
   * It may be impossible to determine the length of a file therefore the
   * behaviour when reading beyond EOF depends on the standard C library.
   */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == TEST_STR[0]);

  assert(!reader_fseek(&r, strlen(TEST_STR)*2l, SEEK_SET));
  assert(reader_ftell(&r) == strlen(TEST_STR)*2l);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));
  assert(reader_fgetc(&r) == EOF);
  /* Result depends on standard C library */
#ifdef ACORN_C
  assert(!reader_feof(&r));
  assert(reader_ferror(&r));
#endif
  /* End of C library-dependent code */
  reader_destroy(&r);

  if (rtype == READERTYPE_RAW) {
    rewind_file(rtype);
    assert(f);
    assert(fgetc(&*f) == TEST_STR[0]);
    assert(!fseek(&*f, strlen(TEST_STR)*2l, SEEK_SET));
    assert(ftell(&*f) == strlen(TEST_STR)*2l);
    assert(!feof(&*f));
    assert(!ferror(&*f));
    assert(fgetc(&*f) == EOF);
    /* Result depends on standard C library */
#ifdef ACORN_C
    assert(!feof(f));
    assert(ferror(f));
#endif
    /* End of C library-dependent code */
  }

  delete_file(rtype);
}

static void test27(ReaderType const rtype)
{
  /* Seek from end (expected to fail) */
  Reader r;
  make_file_from_string(rtype, TEST_STR);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == TEST_STR[0]);

  assert(reader_fseek(&r, 0, SEEK_END));
  assert(reader_ftell(&r) == 1);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  reader_destroy(&r);

  delete_file(rtype);
}

static void test28(ReaderType const rtype)
{
  /* Read after seek forward fail recovery */
  unsigned long limit;
  make_file_from_string(rtype, TEST_STR);

  for (limit = 0; limit < FortifyAllocationLimit; ++limit)
  {
    Reader r;
    init_reader(rtype, &r);

    assert(!reader_fseek(&r, Offset, SEEK_SET));

    Fortify_SetNumAllocationsLimit(limit);
    int const c = reader_fgetc(&r);
    Fortify_SetNumAllocationsLimit(ULONG_MAX);

    assert(!reader_feof(&r));
    if (c == EOF) {
      assert(reader_ferror(&r));
      assert(reader_ftell(&r) == Offset);
    } else {
      assert(c == TEST_STR[Offset]);
      assert(!reader_ferror(&r));
      assert(reader_ftell(&r) == Offset+1);
    }

    reader_destroy(&r);

    if (c != EOF)
      break;

    rewind_file(rtype);
  }
  assert(limit != FortifyAllocationLimit);

  delete_file(rtype);
}

static void test29(ReaderType const rtype)
{
  /* Read after seek back fail recovery */
  unsigned long limit;
  make_file_from_string(rtype, TEST_STR);

  for (limit = 0; limit < FortifyAllocationLimit; ++limit)
  {
    Reader r;
    init_reader(rtype, &r);

    long int pos;
    int c;
    for (pos = 0; pos < (long int)sizeof(TEST_STR)/2; ++pos) {
      c = reader_fgetc(&r);
      assert(c == TEST_STR[pos]);
    }

    const long int offset = -(long int)sizeof(TEST_STR) / 3;
    assert(!reader_fseek(&r, offset, SEEK_CUR));

    Fortify_SetNumAllocationsLimit(limit);
    c = reader_fgetc(&r);
    Fortify_SetNumAllocationsLimit(ULONG_MAX);

    assert(!reader_feof(&r));
    if (c == EOF) {
      assert(reader_ferror(&r));
    } else {
      assert(c == TEST_STR[pos + offset]);
      assert(!reader_ferror(&r));
      assert(reader_ftell(&r) == pos + offset + 1);
    }

    reader_destroy(&r);

    if (c != EOF)
      break;

    rewind_file(rtype);
  }
  assert(limit != FortifyAllocationLimit);

  delete_file(rtype);
}

static void test30(ReaderType const rtype)
{
  /* Seek forward far from current */
  Reader r;
  unsigned char data[LongDataSize];
  for (size_t n = 0; n < sizeof(data); ++n) {
    data[n] = rand();
  }
  make_file(rtype, data, sizeof(data), 1);

  init_reader(rtype, &r);

  assert(reader_fgetc(&r) == data[0]);

  assert(!reader_fseek(&r, (long int)sizeof(data) - 1 - Offset, SEEK_CUR));
  assert(reader_ftell(&r) == (long int)sizeof(data) - Offset);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  assert(reader_fgetc(&r) == data[sizeof(data) - Offset]);

  reader_destroy(&r);

  delete_file(rtype);
}

static void test31(ReaderType const rtype)
{
  /* Seek back far from current */
  Reader r;
  unsigned char data[LongDataSize];
  for (size_t n = 0; n < sizeof(data); ++n) {
    data[n] = rand();
  }
  make_file(rtype, data, sizeof(data), 1);

  init_reader(rtype, &r);

  for (size_t n = 0; n < sizeof(data); ++n) {
    assert(reader_fgetc(&r) == data[n]);
  }

  assert(!reader_fseek(&r, -(long int)sizeof(data) + Offset, SEEK_CUR));
  assert(reader_ftell(&r) == Offset);
  assert(!reader_feof(&r));
  assert(!reader_ferror(&r));

  for (size_t n = Offset; n < sizeof(data); ++n) {
    assert(reader_fgetc(&r) == data[n]);
  }

  reader_destroy(&r);

  delete_file(rtype);
}

static const char *rtype_to_string(ReaderType const rtype)
{
  const char *s;
  switch (rtype) {
  case READERTYPE_RAW:
    s = "Raw";
    break;
  case READERTYPE_GKEY:
    s = "GKey";
    break;
#ifdef ACORN_FLEX
  case READERTYPE_FLEX:
    s = "Flex";
    break;
#endif
  case READERTYPE_MEM:
    s = "Mem";
    break;
  default:
    s = "Unknown";
    break;
  }
  return s;
}

void Reader_tests(void)
{
  static const struct
  {
    const char *test_name;
    void (*test_func)(ReaderType const rtype);
  }
  unit_tests[] =
  {
    { "Init/term", test1 },
    { "Get char", test2 },
    { "Get char fail recovery", test3 },
    { "Unget char", test4 },
    { "Unget EOF", test5 },
    { "Unget char clears EOF", test6 },
    { "Unget two chars", test7 },
    { "Read one", test8 },
    { "Read multiple", test9 },
    { "Read zero", test10 },
    { "Read zero size", test11 },
    { "Read past EOF", test12 },
    { "Read partial", test13 },
    { "Read fail recovery", test14 },
    { "Read ui16", test15 },
    { "Read i32", test16 },
    { "Unget at start", test17 },
    { "Seek forward from current", test18 },
    { "Seek current", test19 },
    { "Seek back from current", test20 },
    { "Seek forward from current after unget", test21 },
    { "Seek beyond start from current", test22 },
    { "Seek beyond end from current", test23 },
    { "Seek back from start", test24 },
    { "Seek forward from start", test25 },
    { "Seek beyond end from start", test26 },
    { "Seek from end", test27 },
    { "Read after seek forward fail recovery", test28 },
    { "Read after seek back fail recovery", test29 },
    { "Seek forward far from current", test30 },
    { "Seek back far from current", test31 },
  };

  for (size_t count = 0; count < ARRAY_SIZE(unit_tests); count ++)
  {
    for (ReaderType rtype = READERTYPE_RAW;
         rtype < READERTYPE_COUNT;
         rtype = (ReaderType)(rtype + 1)) {

      printf("Test %zu/%zu : %s (%s)\n",
             1 + count,
             ARRAY_SIZE(unit_tests),
             unit_tests[count].test_name,
             rtype_to_string(rtype));

      Fortify_EnterScope();

      unit_tests[count].test_func(rtype);

      Fortify_LeaveScope();
    }
  }
}
