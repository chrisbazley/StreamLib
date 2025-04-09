/*
 * StreamLib test: File writer
 * Copyright (C) 2019 Christopher Bazley
 *
 * This library is free software; you can redistrbute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distrbuted in the hope that it will be useful,
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
#include <inttypes.h>

/* GKeyLib headers */
#include "GKeyDecomp.h"

/* StreamLib headers */
#include "WriterRaw.h"
#include "WriterGKey.h"
#include "WriterGKC.h"
#ifdef ACORN_FLEX
#include "WriterFlex.h"
#endif
#include "WriterMem.h"
#include "WriterHeap.h"
#include "WriterNull.h"

/* Local headers */
#include "Tests.h"

#define TEST_STR "qwerty"

enum
{
  NumberOfWriters = 5,
  HistoryLog2 = 9,
  FortifyAllocationLimit = 2048,
  BufferSize = 512,
  LongDataSize = 320, /* greater than internal buffer size */
  Offset = 2,
  HeadLen = 2,
  TailLen = 1,
};

typedef enum  {
  WRITERTYPE_RAW,
  WRITERTYPE_GKEY,
  WRITERTYPE_GKC,
#ifdef ACORN_FLEX
  WRITERTYPE_FLEX,
#endif
  WRITERTYPE_MEM,
  WRITERTYPE_HEAP,
  WRITERTYPE_NULL,
  WRITERTYPE_COUNT
} WriterType;

static _Optional void *anchors[NumberOfWriters];
static _Optional void *buffers[NumberOfWriters];
static _Optional FILE *f[NumberOfWriters];
static int wnum = 0;
static char file_names[NumberOfWriters][L_tmpnam];
static long int out_size;

static void close_file(WriterType const wtype, int const handle)
{
  printf("Closing file with handle %d\n", handle);
  assert(handle >= 0);
  assert(handle < NumberOfWriters);
  _Optional FILE *const fh = f[handle];

  switch (wtype) {
  case WRITERTYPE_RAW:
  case WRITERTYPE_GKEY:
    assert(fh);
    if (fclose(&*fh)) { perror("fclose failed"); }
    f[handle] = NULL;
    break;

#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
#endif
  case WRITERTYPE_MEM:
  case WRITERTYPE_HEAP:
  case WRITERTYPE_NULL:
  case WRITERTYPE_GKC:
    break;

  default:
    abort();
    break;
  }
}

static void delete_file(WriterType const wtype, int const handle)
{
  printf("Deleting file with handle %d\n", handle);
  assert(handle >= 0);
  assert(handle < NumberOfWriters);

  switch (wtype) {
  case WRITERTYPE_RAW:
  case WRITERTYPE_GKEY:
    remove(file_names[handle]);
    break;

#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
    if (anchors[handle]) {
      flex_free(&anchors[handle]);
    }
    break;
#endif

  case WRITERTYPE_MEM:
  case WRITERTYPE_HEAP:
    free(buffers[handle]);
    buffers[handle] = NULL;
    break;

  case WRITERTYPE_NULL:
  case WRITERTYPE_GKC:
    break;

  default:
    abort();
    break;
  }
}

static bool file_is_extensible(WriterType const wtype)
{
  bool is_ext = false;

  switch (wtype) {
  case WRITERTYPE_RAW:
  case WRITERTYPE_GKEY:
#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
#endif
  case WRITERTYPE_HEAP:
  case WRITERTYPE_NULL:
  case WRITERTYPE_GKC:
    is_ext = true;
    break;

  case WRITERTYPE_MEM:
    is_ext = false;
    break;

  default:
    abort();
    break;
  }

  return is_ext;
}

static bool trailing_zeros(WriterType const wtype)
{
  bool trail = false;

  switch (wtype) {
  case WRITERTYPE_GKEY:
    trail = true;
    break;

#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
#endif
  case WRITERTYPE_RAW:
  case WRITERTYPE_MEM:
  case WRITERTYPE_HEAP:
  case WRITERTYPE_NULL:
  case WRITERTYPE_GKC:
    trail = false;
    break;

  default:
    abort();
    break;
  }

  return trail;
}

static bool discards_writes(WriterType const wtype)
{
  bool discards = false;

  switch (wtype) {
  case WRITERTYPE_GKEY:
#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
#endif
  case WRITERTYPE_RAW:
  case WRITERTYPE_MEM:
  case WRITERTYPE_HEAP:
    discards = false;
    break;

  case WRITERTYPE_NULL:
  case WRITERTYPE_GKC:
    discards = true;
    break;

  default:
    abort();
    break;
  }

  return discards;
}

static bool can_seek_back(WriterType const wtype)
{
  bool seek_back = false;

  switch (wtype) {
  case WRITERTYPE_GKEY:
  case WRITERTYPE_GKC:
    seek_back = false;
    break;

#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
#endif
  case WRITERTYPE_RAW:
  case WRITERTYPE_MEM:
  case WRITERTYPE_HEAP:
  case WRITERTYPE_NULL:
    seek_back = true;
    break;

  default:
    abort();
    break;
  }

  return seek_back;
}

static void read_file(WriterType const wtype, void *const data,
  size_t size, size_t const nmemb, int const handle)
{
  printf("Test reads %zu items of size %zu from handle %d\n",
         nmemb, size, handle);

  switch (wtype) {
  case WRITERTYPE_RAW:
    {
      FILE *const f = fopen(file_names[handle], "rb");
      if (f == NULL) perror("Failed to open file");
      assert(f != NULL);

      if (size > 0) {
        size_t const n = fread(data, size, nmemb, f);
        printf("Read %zu of %zu\n", n, nmemb);
        if (n != nmemb) { perror("Failed to read from file"); }
        assert(n == nmemb);
      }
      assert(!fclose(f));
    }
    break;

  case WRITERTYPE_GKEY:
    {
      FILE *const f = fopen(file_names[handle], "rb");
      if (f == NULL) perror("Failed to open file");
      assert(f != NULL);

      size *= nmemb;

      uint32_t decomp_size = 0;
      for (size_t n = 0; n < 4; ++n) {
        int const c = fgetc(f);
        assert(c >= 0);
        decomp_size |= (uint32_t)c << (CHAR_BIT * n);
      }
      assert(size == decomp_size);
      _Optional GKeyDecomp *const decomp = gkeydecomp_make(HistoryLog2);
      GKeyStatus stat = GKeyStatus_OK;
      GKeyParameters params = {
        .out_buffer = data,
        .out_size = size
      };
      do {
        char buf[BufferSize];
        size_t const n = fread(buf, 1, sizeof(buf), f);
        printf("Read %zu of %zu\n", n, sizeof(buf));
        if (!n) {
          assert(feof(f));
          break;
        }
        params.in_buffer = buf;
        params.in_size = n;
        stat = gkeydecomp_decompress(&*decomp, &params);
      } while (stat == GKeyStatus_OK ||
               stat == GKeyStatus_TruncatedInput);

      assert(stat == GKeyStatus_OK);
      assert(params.out_size == 0);
      gkeydecomp_destroy(decomp);
      assert(!fclose(f));
    }
    break;

#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
    size *= nmemb;
    assert(size == 0 || anchors[handle]);
    assert(!anchors[handle] || size == (unsigned)flex_size(&anchors[handle]));
    {
      int const bstate = flex_set_budge(0);
      memcpy(data, anchors[handle], size);
      flex_set_budge(bstate);
    }
    break;
#endif

  case WRITERTYPE_MEM:
  case WRITERTYPE_HEAP:
    size *= nmemb;
    if (size != 0) {
      _Optional void *const bh = buffers[handle];
      assert(bh);
      memcpy(data, &*bh, size);
    }
    break;

  case WRITERTYPE_NULL:
  case WRITERTYPE_GKC:
    break;

  default:
    abort();
    break;
  }
}

static int open_file(WriterType const wtype, size_t const min_size)
{
  assert(wnum >= 0);
  assert(wnum < NumberOfWriters);
  printf("Opening file of size %zu\n", min_size);

  switch (wtype) {
  case WRITERTYPE_RAW:
    tmpnam(file_names[wnum]);
    assert(f[wnum] == NULL);
    f[wnum] = fopen(file_names[wnum], "wb");
    assert(f[wnum] != NULL);
    break;

  case WRITERTYPE_GKEY:
    tmpnam(file_names[wnum]);
    assert(f[wnum] == NULL);
    f[wnum] = fopen(file_names[wnum], "wb");
    assert(f[wnum] != NULL);
    break;

#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
    assert(!anchors[wnum]);
    assert(min_size <= INT_MAX);
    if (min_size > 0) {
      assert(flex_alloc(&anchors[wnum], (int)min_size));
    } else {
      printf("No flex buffer input\n");
    }
    break;
#endif

  case WRITERTYPE_MEM:
  case WRITERTYPE_HEAP:
    assert(!buffers[wnum]);
    assert(min_size <= SIZE_MAX);
    if (min_size > 0) {
      buffers[wnum] = malloc(min_size);
      assert(buffers[wnum] != NULL);
    } else {
      printf("No malloc buffer input\n");
    }
    break;

  case WRITERTYPE_NULL:
  case WRITERTYPE_GKC:
    break;

  default:
    abort();
    break;
  }

  printf("Opened file with handle %d\n", wnum);
  return wnum++;
}

static bool init_writer(WriterType const wtype, Writer *const w,
  size_t min_size, int const handle)
{
  bool success = true;
  assert(handle >= 0);
  assert(handle < NumberOfWriters);
  printf("Init writer with size %zu and handle %d\n", min_size, handle);

  _Optional FILE *const fh = f[handle];
  _Optional void *const bh = buffers[handle];

  switch (wtype) {
  case WRITERTYPE_RAW:
    assert(fh);
    writer_raw_init(w, &*fh);
    break;

  case WRITERTYPE_GKEY:
  assert(fh);
  success = writer_gkey_init(w, HistoryLog2, min_size, &*fh);
    break;

  case WRITERTYPE_GKC:
    out_size = LONG_MIN;
    success = writer_gkc_init(w, HistoryLog2, &out_size);
    break;

#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
    writer_flex_init(w, &anchors[handle]);
    break;
#endif

  case WRITERTYPE_MEM:
    success = writer_mem_init(w, bh, min_size);
    break;

  case WRITERTYPE_HEAP:
    success = writer_heap_init(w, &buffers[handle], min_size);
    break;

  case WRITERTYPE_NULL:
    writer_null_init(w);
    break;

  default:
    abort();
    break;
  }

  printf("Init writer %s\n", success ? "OK" : "FAIL");
  return success;
}

static int open_file_and_init_writer(WriterType const wtype, Writer *const w,
  size_t const min_size)
{
  int const handle = open_file(wtype, min_size);
  assert(init_writer(wtype, w, min_size, handle));
  return handle;
}

static void destroy_and_check(WriterType const wtype, Writer *const w,
  long int const expected_len)
{
  if (writer_ferror(w)) {
    assert(writer_destroy(w) == -1l);
  } else {
    assert(writer_destroy(w) == expected_len);

    if (wtype == WRITERTYPE_GKC) {
      long int const worst_bits = expected_len * (CHAR_BIT + 1);
      long int const min = sizeof(int32_t);
      long int const max = min + (worst_bits + CHAR_BIT - 1) / CHAR_BIT;
      printf("out_size %ld should be in range [%ld,%ld]\n", out_size,
        min, max);
      assert(out_size >= min);
      assert(out_size <= max);
    }
  }
}

static void test1(WriterType const wtype)
{
  /* Init/term */
  Writer w[NumberOfWriters];
  int handles[NumberOfWriters];

  for (size_t i = 0; i < ARRAY_SIZE(w); i++) {
    handles[i] = open_file_and_init_writer(wtype, &w[i], 0);

    assert(!writer_ferror(&w[i]));
    assert(writer_ftell(&w[i]) == 0);
  }

  printf("All init complete\n");

  for (size_t i = 0; i < ARRAY_SIZE(w); i++) {
    assert(writer_destroy(&w[i]) == 0);

    close_file(wtype, handles[i]);
    delete_file(wtype, handles[i]);
  }
}

static void put_chars(WriterType const wtype, const char *const expected,
  size_t const nelems, size_t min_size)
{
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, min_size);

    for (size_t i = 0; i < nelems; ++i) {
      if (file_is_extensible(wtype) || i < min_size) {
        assert(writer_fputc(expected[i], &w) == expected[i]);
        assert(writer_ftell(&w) == (long)i+1l);
        assert(!writer_ferror(&w));
      } else {
        assert(writer_fputc(expected[i], &w) == EOF);
        assert(writer_ftell(&w) == (long)min_size);
        assert(writer_ferror(&w));
      }
    }

    destroy_and_check(wtype, &w, nelems);
  }

  close_file(wtype, handle);

  if (!discards_writes(wtype) &&
      (file_is_extensible(wtype) || nelems <= min_size)) {
    if (!trailing_zeros(wtype)) {
      min_size = nelems;
    }

    unsigned char buf[min_size > nelems ? min_size : nelems];
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);

    for (size_t i = 0; i < nelems; ++i) {
      assert(buf[i] == expected[i]);
    }
    for (size_t i = nelems; i < min_size; ++i) {
      assert(buf[i] == 0);
    }
  }

  delete_file(wtype, handle);
}

static void test2(WriterType const wtype)
{
  /* Put char */
  put_chars(wtype, TEST_STR, strlen(TEST_STR), strlen(TEST_STR));
}

static void test3(WriterType const wtype)
{
  /* Put char fail recovery */
  int handle;
  {
    Writer w;
    /* If writing to a buffer, reallocation should fail at the last
       byte */
    handle = open_file_and_init_writer(wtype, &w, LongDataSize-1);

    /* We need to write a lot of uncompressable data to fill the
       output buffer and ensure that some needs to written to file. */
    Fortify_SetNumAllocationsLimit(0);
    long int i;
    for (i = 0; i < LongDataSize; ++i) {
      int const b = rand() & UCHAR_MAX;
      int const c = writer_fputc(b, &w);

      if (c == EOF) {
        assert(writer_ferror(&w));
        assert(writer_ftell(&w) == i);
        break;
      }

      assert(c == b);
      assert(!writer_ferror(&w));
      assert(writer_ftell(&w) == i + 1l);
    }
    Fortify_SetNumAllocationsLimit(ULONG_MAX);
#ifdef FORTIFY
    assert(discards_writes(wtype) || i < LongDataSize);
#endif
    destroy_and_check(wtype, &w, LongDataSize);
  }

  close_file(wtype, handle);
  delete_file(wtype, handle);
}

static void test4(WriterType const wtype)
{
  /* Put more chars than expected */
  put_chars(wtype, TEST_STR, strlen(TEST_STR), 1);
}

static void test5(WriterType const wtype)
{
  /* Put fewer chars than expected */
  put_chars(wtype, TEST_STR, strlen(TEST_STR), strlen(TEST_STR) + TailLen);
}

static void test8(WriterType const wtype)
{
  /* Write one */
  static const int expected[] = {1232, -24243443, 0, -13};
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, sizeof(expected));

    for (size_t i = 0; i < ARRAY_SIZE(expected); ++i) {
      assert(writer_fwrite(&expected[i], sizeof(expected[i]), 1, &w) == 1);
      assert(writer_ftell(&w) == (long)sizeof(expected[i]) * ((long)i+1l));
      assert(!writer_ferror(&w));
    }

    destroy_and_check(wtype, &w, sizeof(expected));
  }
  close_file(wtype, handle);

  if (!discards_writes(wtype)) {
    int buf[ARRAY_SIZE(expected)];
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);
    delete_file(wtype, handle);

    for (size_t i = 0; i < ARRAY_SIZE(expected); ++i) {
      assert(expected[i] == buf[i]);
    }
  }
}

static void write_mul(WriterType const wtype, const int *const expected,
  size_t const nelems, size_t min_size)
{
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w,
      min_size * sizeof(expected[0]));

    if (file_is_extensible(wtype) || nelems <= min_size) {
      assert(writer_fwrite(expected, sizeof(expected[0]), nelems, &w) == nelems);
      assert(writer_ftell(&w) == (long)sizeof(expected[0]) * (long)nelems);
      assert(!writer_ferror(&w));
    } else {
      assert(writer_fwrite(expected, sizeof(expected[0]), nelems, &w) <= min_size);
      assert(writer_ftell(&w) <= (long)sizeof(expected[0] * (long)min_size));
      assert(writer_ferror(&w));
    }

    destroy_and_check(wtype, &w, (long)sizeof(expected[0]) * (long)nelems);
  }
  close_file(wtype, handle);

  if (!discards_writes(wtype) &&
      (file_is_extensible(wtype) || nelems <= min_size)) {
    if (!trailing_zeros(wtype)) {
      min_size = nelems;
    }

    int buf[min_size > nelems ? min_size : nelems];
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);

    for (size_t i = 0; i < nelems; ++i) {
      assert(expected[i] == buf[i]);
    }

    for (size_t i = nelems; i < min_size; ++i) {
      assert(buf[i] == 0);
    }
  }
  delete_file(wtype, handle);
}

static void test9(WriterType const wtype)
{
  /* Write multiple */
  static const int expected[] = {1232, -24243443, 0, -13};
  write_mul(wtype, expected, ARRAY_SIZE(expected), ARRAY_SIZE(expected));
}

static void test10(WriterType const wtype)
{
  /* Write zero */
  static const int expected[] = {1232, -24243443, 0, -13};
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, 0);

    assert(writer_fwrite(expected, sizeof(expected[0]), 0, &w) == 0);
    /* fwrite returns zero and the contents of the array
       and the state of the stream remain unchanged */
    assert(writer_ftell(&w) == 0);
    assert(!writer_ferror(&w));

    destroy_and_check(wtype, &w, 0);
  }
  close_file(wtype, handle);

  int buf[ARRAY_SIZE(expected)];
  read_file(wtype, buf, sizeof(buf[0]), 0, handle);
  delete_file(wtype, handle);
}

static void test11(WriterType const wtype)
{
  /* Write zero size */
  static const int expected[] = {1232, -24243443, 0, -13};
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, 0);

    assert(writer_fwrite(expected, 0, ARRAY_SIZE(expected), &w) == 0);
    /* fwrite returns zero and the contents of the array
       and the state of the stream remain unchanged */
    assert(writer_ftell(&w) == 0);
    assert(!writer_ferror(&w));

    destroy_and_check(wtype, &w, 0);
  }
  close_file(wtype, handle);

  int buf[ARRAY_SIZE(expected)];
  read_file(wtype, buf, 0, ARRAY_SIZE(buf), handle);
  delete_file(wtype, handle);
}

static void test12(WriterType const wtype)
{
  /* Write beyond expected end */
  static const int expected[] = {1232, -24243443, 0, -13};
  write_mul(wtype, expected, ARRAY_SIZE(expected), 0);
}

static void test12b(WriterType const wtype)
{
  /* Write beyond expected end (non-zero) */
  static const int expected[] = {1232, -24243443, 0, -13};
  write_mul(wtype, expected, ARRAY_SIZE(expected), 1);
}

static void test13(WriterType const wtype)
{
  /* Write less than expected */
  static const int expected[] = {1232, -24243443, 0, -13};
  write_mul(wtype, expected, ARRAY_SIZE(expected), ARRAY_SIZE(expected) + TailLen);
}

static void test14(WriterType const wtype)
{
  /* Write fail recovery */
  unsigned long limit;
  unsigned char data[LongDataSize];
  for (size_t n = 0; n < sizeof(data); ++n) {
    data[n] = rand();
  }

  for (limit = 0; limit < FortifyAllocationLimit; ++limit) {
    Writer w;
    int const handle = open_file_and_init_writer(wtype, &w, LongDataSize);

    /* We need to write a large amount of data to fill the output buffer
       and ensure that some needs to written to the file. */
    Fortify_SetNumAllocationsLimit(limit);
    size_t const n = writer_fwrite(data, sizeof(data[0]),
      ARRAY_SIZE(data), &w);

    Fortify_SetNumAllocationsLimit(ULONG_MAX);

    if (n < ARRAY_SIZE(data)) {
      assert(writer_ferror(&w));
    } else {
      assert(n == ARRAY_SIZE(data));
      assert(!writer_ferror(&w));
    }

    destroy_and_check(wtype, &w, sizeof(data));
    close_file(wtype, handle);
    delete_file(wtype, handle);

    if (n > 0)
      break;
  }
  assert(limit != FortifyAllocationLimit);

}

static void test15(WriterType const wtype)
{
  /* Write ui16 */
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

  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, sizeof(expected));

    for (size_t x = 0; x < ARRAY_SIZE(e); ++x) {
      assert(writer_fwrite_uint16(e[x], &w));
      assert(writer_ftell(&w) == ((long)x + 1l) * (long)sizeof(e[0]));
      assert(!writer_ferror(&w));
    }

    destroy_and_check(wtype, &w, j);
  }
  close_file(wtype, handle);

  if (!discards_writes(wtype)) {
    unsigned char buf[j];
    read_file(wtype, buf, sizeof(buf[0]), j, handle);

    for (size_t n = 0; n < j; ++n) {
      assert(buf[n] == expected[n]);
    }
  }
  delete_file(wtype, handle);
}

static void test16(WriterType const wtype)
{
  /* Write i32 */
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

  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, sizeof(expected));

    for (size_t x = 0; x < ARRAY_SIZE(e); ++x) {
      assert(writer_fwrite_int32(e[x], &w));
      assert(writer_ftell(&w) == ((long)x+1l) * (long)sizeof(e[0]));
      assert(!writer_ferror(&w));
    }

    destroy_and_check(wtype, &w, j);
  }
  close_file(wtype, handle);

  if (!discards_writes(wtype)) {
    unsigned char buf[j];
    read_file(wtype, buf, sizeof(buf[0]), j, handle);

    for (size_t n = 0; n < j; ++n) {
      assert(buf[n] == expected[n]);
    }
  }
  delete_file(wtype, handle);
}

static void cur_forward(WriterType const wtype, size_t const min_size)
{
  assert(min_size >= HeadLen);
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, min_size);

    for (size_t n = 0; n < HeadLen; ++n) {
      assert(writer_fputc(TEST_STR[n], &w) == TEST_STR[n]);
      assert(writer_ftell(&w) == (long)n + 1l);
      assert(!writer_ferror(&w));
    }

    const size_t seek_pos = strlen(TEST_STR) - TailLen;
    assert(!writer_fseek(&w, (long)seek_pos - HeadLen, SEEK_CUR));
    assert(writer_ftell(&w) == (long)seek_pos);
    assert(!writer_ferror(&w));

    for (size_t n = seek_pos; n < strlen(TEST_STR); ++n) {
      if (file_is_extensible(wtype) || n < min_size) {
        assert(writer_fputc(TEST_STR[n], &w) == TEST_STR[n]);
        assert(writer_ftell(&w) == (long)n + 1l);
        assert(!writer_ferror(&w));
      } else {
        assert(writer_fputc(TEST_STR[n], &w) == EOF);
        if (min_size < seek_pos) {
          assert(writer_ftell(&w) == (long)seek_pos);
        } else {
          assert(writer_ftell(&w) == (long)min_size);
        }
        assert(writer_ferror(&w));
      }
    }

    destroy_and_check(wtype, &w, strlen(TEST_STR));
  }

  close_file(wtype, handle);

  if (!discards_writes(wtype) &&
      (file_is_extensible(wtype) || strlen(TEST_STR) <= min_size)) {
    unsigned char buf[strlen(TEST_STR)];
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);

    for (size_t n = 0; n < strlen(TEST_STR); ++n) {
      if (n < HeadLen || n >= strlen(TEST_STR) - TailLen) {
        assert(buf[n] == TEST_STR[n]);
      } else {
        assert(buf[n] == 0);
      }
    }
  }

  delete_file(wtype, handle);
}

static void test17(WriterType const wtype)
{
  /* Seek forward from current */
  cur_forward(wtype, strlen(TEST_STR));
}

static void test18(WriterType const wtype)
{
  /* Seek beyond expected end from current */
  cur_forward(wtype, HeadLen);
}

static void test19(WriterType const wtype)
{
  /* Seek current */
  unsigned char buf[2];
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, sizeof(buf));

    assert(writer_fputc(TEST_STR[0], &w) == TEST_STR[0]);

    assert(!writer_fseek(&w, 0, SEEK_CUR));
    assert(writer_ftell(&w) == 1);
    assert(!writer_ferror(&w));

    assert(writer_fputc(TEST_STR[1], &w) == TEST_STR[1]);

    destroy_and_check(wtype, &w, sizeof(buf));
  }
  close_file(wtype, handle);

  if (!discards_writes(wtype)) {
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);

    for (size_t n = 0; n < ARRAY_SIZE(buf); ++n) {
      assert(buf[n] == TEST_STR[n]);
    }
  }
  delete_file(wtype, handle);
}

static void test20(WriterType const wtype)
{
  /* Seek back from current */
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, strlen(TEST_STR));

    for (size_t n = 0; n < strlen(TEST_STR); ++n) {
      assert(writer_fputc(TEST_STR[n], &w) == TEST_STR[n]);
    }

    /* We should always be able to move the write position */
    assert(!writer_fseek(&w, -(long)strlen(TEST_STR) + Offset, SEEK_CUR));
    assert(writer_ftell(&w) == Offset);
    assert(!writer_ferror(&w));

    /* Subsequent writes may fail */
    if (can_seek_back(wtype)) {
      assert(writer_fputc('9', &w) == '9');
      assert(writer_ftell(&w) == Offset + 1l);
      assert(!writer_ferror(&w));
    } else {
      assert(writer_fputc('9', &w) == EOF);
      assert(writer_ftell(&w) == Offset);
      assert(writer_ferror(&w));
    }

    destroy_and_check(wtype, &w, strlen(TEST_STR));
  }

  close_file(wtype, handle);

  if (!discards_writes(wtype) && can_seek_back(wtype)) {
    unsigned char buf[strlen(TEST_STR)];
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);

    for (size_t n = 0; n < strlen(TEST_STR); ++n) {
      if (n == Offset) {
        assert(buf[n] == '9');
      } else {
        assert(buf[n] == TEST_STR[n]);
      }
    }
  }

  delete_file(wtype, handle);
}

static void test21(WriterType const wtype)
{
  /* Seek forward then back from current. */
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, strlen(TEST_STR));

    /* Write head of string */
    for (long int n = 0; n < HeadLen; ++n) {
      assert(writer_fputc(TEST_STR[n], &w) == TEST_STR[n]);
      assert(writer_ftell(&w) == n + 1l);
      assert(!writer_ferror(&w));
    }

    /* Seek end of string */
    assert(!writer_fseek(&w, strlen(TEST_STR) - (long)HeadLen, SEEK_CUR));
    assert(writer_ftell(&w) == strlen(TEST_STR));
    assert(!writer_ferror(&w));

    /* Seek start of tail */
    assert(!writer_fseek(&w, -(long)TailLen, SEEK_CUR));
    assert(writer_ftell(&w) == strlen(TEST_STR) - (long)TailLen);
    assert(!writer_ferror(&w));

    /* Write tail of string */
    for (size_t n = strlen(TEST_STR) - TailLen; n < strlen(TEST_STR); ++n) {
      assert(writer_fputc(TEST_STR[n], &w) == TEST_STR[n]);
      assert(writer_ftell(&w) == (long)n + 1l);
      assert(!writer_ferror(&w));
    }

    destroy_and_check(wtype, &w, strlen(TEST_STR));
  }

  close_file(wtype, handle);

  if (!discards_writes(wtype)) {
    unsigned char buf[strlen(TEST_STR)];
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);

    for (size_t n = 0; n < strlen(TEST_STR); ++n) {
      if (n < HeadLen || n >= strlen(TEST_STR) - TailLen) {
        assert(buf[n] == TEST_STR[n]);
      } else {
        assert(buf[n] == 0);
      }
    }
  }

  delete_file(wtype, handle);
}

static void test22(WriterType const wtype)
{
  /* Seek beyond start from current */
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, 1);

    assert(writer_fputc(TEST_STR[0], &w) == TEST_STR[0]);
    assert(writer_fseek(&w, -2, SEEK_CUR));
    assert(writer_ftell(&w) == 1);
    assert(writer_ferror(&w));

    destroy_and_check(wtype, &w, 1);
  }

  if (wtype == WRITERTYPE_RAW) {
    _Optional FILE *const fh = f[handle];
    assert(fh);
    rewind(&*fh);
    assert(!ferror(&*fh));
    assert(fputc(TEST_STR[0], &*fh) == TEST_STR[0]);
    assert(fseek(&*fh, -2, SEEK_CUR));
    /* Result depends on standard C library */
    assert(ftell(&*fh) == 1);
#ifdef ACORN_C
    assert(ferror(fh));
#endif
    /* End of C library-dependent code */
  }

  close_file(wtype, handle);
  delete_file(wtype, handle);
}

static void test23(WriterType const wtype)
{
  /* Seek back relative to start */
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, strlen(TEST_STR));

    for (size_t n = 0; n < strlen(TEST_STR); ++n) {
      assert(writer_fputc(TEST_STR[n], &w) == TEST_STR[n]);
    }

    /* We should always be able to move the write position */
    assert(!writer_fseek(&w, Offset, SEEK_SET));
    assert(writer_ftell(&w) == Offset);
    assert(!writer_ferror(&w));

    /* Subsequent writes may fail */
    if (can_seek_back(wtype)) {
      assert(writer_fputc('9', &w) == '9');
      assert(writer_ftell(&w) == Offset + 1l);
      assert(!writer_ferror(&w));
    } else {
      assert(writer_fputc('9', &w) == EOF);
      assert(writer_ftell(&w) == Offset);
      assert(writer_ferror(&w));
    }

    destroy_and_check(wtype, &w, strlen(TEST_STR));
  }

  close_file(wtype, handle);

  if (!discards_writes(wtype) && can_seek_back(wtype)) {
    unsigned char buf[strlen(TEST_STR)];
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);

    for (size_t n = 0; n < strlen(TEST_STR); ++n) {
      if (n == Offset) {
        assert(buf[n] == '9');
      } else {
        assert(buf[n] == TEST_STR[n]);
      }
    }
  }

  delete_file(wtype, handle);
}

static void test24(WriterType const wtype)
{
  /* Seek back from start */
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, 1);

    assert(writer_fputc(TEST_STR[0], &w) == TEST_STR[0]);
    assert(writer_fseek(&w, -1, SEEK_SET));
    assert(writer_ftell(&w) == 1);
    assert(writer_ferror(&w));

    destroy_and_check(wtype, &w, 1);
  }

  if (wtype == WRITERTYPE_RAW) {
    _Optional FILE *const fh = f[handle];
    assert(fh);
    rewind(&*fh);
    assert(fputc(TEST_STR[0], &*fh) == TEST_STR[0]);
    assert(fseek(&*fh, -1, SEEK_SET));
    /* Result depends on standard C library */
    assert(ftell(&*fh) == 1);
#ifdef ACORN_C
    assert(ferror(&*fh));
#endif
    /* End of C library-dependent code */
  }

  close_file(wtype, handle);
  delete_file(wtype, handle);
}

static void set_forward(WriterType const wtype, size_t const min_size)
{
  assert(min_size >= HeadLen);
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, min_size);

    for (size_t n = 0; n < HeadLen; ++n) {
      assert(writer_fputc(TEST_STR[n], &w) == TEST_STR[n]);
    }

    const size_t seek_pos = strlen(TEST_STR) - TailLen;
    assert(!writer_fseek(&w, seek_pos, SEEK_SET));
    assert(writer_ftell(&w) == (long)seek_pos);
    assert(!writer_ferror(&w));

    for (size_t n = seek_pos; n < strlen(TEST_STR); ++n) {
      if (file_is_extensible(wtype) || n < min_size) {
        assert(writer_fputc(TEST_STR[n], &w) == TEST_STR[n]);
        assert(writer_ftell(&w) == (long)n + 1l);
        assert(!writer_ferror(&w));
      } else {
        assert(writer_fputc(TEST_STR[n], &w) == EOF);
        if (min_size < seek_pos) {
          assert(writer_ftell(&w) == (long)seek_pos);
        } else {
          assert(writer_ftell(&w) == (long)min_size);
        }
        assert(writer_ferror(&w));
      }
    }

    destroy_and_check(wtype, &w, strlen(TEST_STR));
  }
  close_file(wtype, handle);

  if (!discards_writes(wtype) &&
      (file_is_extensible(wtype) || strlen(TEST_STR) <= min_size)) {
    unsigned char buf[strlen(TEST_STR)];
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);

    for (size_t n = 0; n < strlen(TEST_STR); ++n) {
      if (n < HeadLen || n >= strlen(TEST_STR) - TailLen) {
        assert(buf[n] == TEST_STR[n]);
      } else {
        assert(buf[n] == 0);
      }
    }
  }

  delete_file(wtype, handle);
}

static void test25(WriterType const wtype)
{
  /* Seek forward from start */
  set_forward(wtype, strlen(TEST_STR));
}

static void test26(WriterType const wtype)
{
  /* Seek beyond expected end from start. */
  set_forward(wtype, HeadLen);
}

static void test27(WriterType const wtype)
{
  /* Seek from end (expected to fail) */
  unsigned char buf[1];
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, sizeof(buf));

    assert(writer_fputc(TEST_STR[0], &w) == TEST_STR[0]);

    assert(writer_fseek(&w, 0, SEEK_END));
    assert(writer_ftell(&w) == 1);
    assert(!writer_ferror(&w));

    destroy_and_check(wtype, &w, 1);
  }
  close_file(wtype, handle);

  if (!discards_writes(wtype)) {
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);
    assert(buf[0] == TEST_STR[0]);
  }
  delete_file(wtype, handle);
}

static void test28(WriterType const wtype)
{
  /* Write after seek forward fail recovery */
  unsigned long limit;
  unsigned char buf[Offset+1];
  int handle = 0;

  for (limit = 0; limit < FortifyAllocationLimit; ++limit)
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, sizeof(buf));

    assert(!writer_fseek(&w, Offset, SEEK_SET));

    Fortify_SetNumAllocationsLimit(limit);
    int const c = writer_fputc(TEST_STR[Offset], &w);
    Fortify_SetNumAllocationsLimit(ULONG_MAX);

    if (c == EOF) {
      assert(writer_ferror(&w));
      assert(writer_ftell(&w) == Offset);
    } else {
      assert(c == TEST_STR[Offset]);
      assert(!writer_ferror(&w));
      assert(writer_ftell(&w) == Offset + 1l);
    }

    destroy_and_check(wtype, &w, Offset + 1l);
    close_file(wtype, handle);

    if (c != EOF)
      break;

    delete_file(wtype, handle);
  }
  assert(limit != FortifyAllocationLimit);

  if (!discards_writes(wtype)) {
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);
    for (size_t n = 0; n < Offset; ++n) {
      assert(buf[n] == 0);
    }
    assert(buf[Offset] == TEST_STR[Offset]);
  }

  delete_file(wtype, handle);
}

static void test30(WriterType const wtype)
{
  /* Seek forward far from current */
  int handle;
  {
    Writer w;
    handle = open_file_and_init_writer(wtype, &w, LongDataSize);

    assert(writer_fputc('y', &w) == 'y');

    assert(!writer_fseek(&w, LongDataSize - 2l, SEEK_CUR));
    assert(writer_ftell(&w) == LongDataSize - 1l);
    assert(!writer_ferror(&w));

    assert(writer_fputc('x', &w) == 'x');

    destroy_and_check(wtype, &w, LongDataSize);
  }

  close_file(wtype, handle);

  if (!discards_writes(wtype)) {
    unsigned char buf[LongDataSize];
    read_file(wtype, buf, sizeof(buf[0]), ARRAY_SIZE(buf), handle);
    assert(buf[0] == 'y');
    for (size_t n = 1; n < LongDataSize - 1; ++n) {
      assert(buf[n] == 0);
    }
    assert(buf[LongDataSize - 1] == 'x');
  }

  delete_file(wtype, handle);
}

static void test31(WriterType const wtype)
{
  /* Init fail recovery */
  unsigned long limit;
  for (limit = 0; limit < FortifyAllocationLimit; ++limit)
  {
    Writer w;
    wnum = 0;
    int const handle = open_file(wtype, 1);

    Fortify_SetNumAllocationsLimit(limit);
    bool const success = init_writer(wtype, &w, 1, handle);
    Fortify_SetNumAllocationsLimit(ULONG_MAX);

    if (success) {
      assert(writer_fputc('y', &w) == 'y');
      destroy_and_check(wtype, &w, 1);
    }

    close_file(wtype, handle);
    delete_file(wtype, handle);

    if (success) {
      break;
    }
  }
  assert(limit != FortifyAllocationLimit);
}

static void test32(WriterType const wtype)
{
  /* Destroy fail recovery */
  unsigned long limit;
  for (limit = 0; limit < FortifyAllocationLimit; ++limit)
  {
    Writer w;
    wnum = 0;
    int const handle = open_file_and_init_writer(wtype, &w, 1);
    assert(writer_fputc('y', &w) == 'y');

    Fortify_SetNumAllocationsLimit(limit);
    long int const len = writer_destroy(&w);
    assert(len == -1l || len == 1l);
    Fortify_SetNumAllocationsLimit(ULONG_MAX);

    close_file(wtype, handle);
    delete_file(wtype, handle);

    if (len >= 0) {
      break;
    }
  }
  assert(limit != FortifyAllocationLimit);
}

static const char *wtype_to_string(WriterType const wtype)
{
  const char *s;
  switch (wtype) {
  case WRITERTYPE_RAW:
    s = "Raw";
    break;
  case WRITERTYPE_GKEY:
    s = "GKey";
    break;
  case WRITERTYPE_GKC:
    s = "GKC";
    break;
#ifdef ACORN_FLEX
  case WRITERTYPE_FLEX:
    s = "Flex";
    break;
#endif
  case WRITERTYPE_MEM:
    s = "Mem";
    break;
  case WRITERTYPE_HEAP:
    s = "Heap";
    break;
  case WRITERTYPE_NULL:
    s = "Null";
    break;
  default:
    s = "Unknown";
    break;
  }
  return s;
}

void Writer_tests(void)
{
  static const struct
  {
    const char *test_name;
    void (*test_func)(WriterType const wtype);
  }
  unit_tests[] =
  {
    { "Init/term", test1 },
    { "Put char", test2 },
    { "Put char fail recovery", test3 },
    { "Put more chars than expected", test4 },
    { "Put fewer chars than expected", test5 },
    { "Write one", test8 },
    { "Write multiple", test9 },
    { "Write zero", test10 },
    { "Write zero size", test11 },
    { "Write beyond buffer or expected end", test12 },
    { "Write beyond buffer or expected end (non-zero)", test12b },
    { "Write less than expected", test13 },
    { "Write fail recovery", test14 },
    { "Write ui16", test15 },
    { "Write i32", test16 },
    { "Seek forward from current", test17 },
    { "Seek beyond expected end from current", test18 },
    { "Seek current", test19 },
    { "Seek back from current", test20 },
    { "Seek forward then back from current", test21 },
    { "Seek beyond start from current", test22 },
    { "Seek back relative to start", test23 },
    { "Seek back from start", test24 },
    { "Seek forward from start", test25 },
    { "Seek beyond expected end from start", test26 },
    { "Seek from end", test27 },
    { "Write after seek forward fail recovery", test28 },
    { "Seek forward far from current", test30 },
    { "Init fail recovery", test31 },
    { "Destroy fail recovery", test32 },
  };

  /* Due to a static initialization bug in gcc, zero-initialization
     doesn't happen. Any other static initializer would be wrong. */
  for (size_t i = 0; i < NumberOfWriters; ++i) {
    anchors[i] = NULL;
    buffers[i] = NULL;
    f[i] = NULL;
  }

  for (size_t count = 0; count < ARRAY_SIZE(unit_tests); count ++)
  {
    for (WriterType wtype = WRITERTYPE_RAW;
         wtype < WRITERTYPE_COUNT;
         wtype = (WriterType)(wtype + 1)) {
      printf("Test %zu/%zu : %s (%s)\n",
             1 + count,
             ARRAY_SIZE(unit_tests),
             unit_tests[count].test_name,
             wtype_to_string(wtype));

      Fortify_EnterScope();
      wnum = 0;
      unit_tests[count].test_func(wtype);
      Fortify_LeaveScope();
    }
  }
}
