/*
 * StreamLib test: Reader/Writer interoperability
 * Copyright (C) 2026 Christopher Bazley
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* ISO library headers */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* StreamLib headers */
#include "ReaderGKey.h"
#include "ReaderMem.h"
#include "ReaderRaw.h"
#include "WriterGKey.h"
#include "WriterMem.h"
#include "WriterRaw.h"
#ifdef ACORN_FLEX
#include "ReaderFlex.h"
#include "WriterFlex.h"
#endif

/* Local headers */
#include "Tests.h"

enum {
  MaxHistoryLog2 = 12,
  RandomSeed = 0x5eed,
  DataSize = 65536,
  MaxChunkSize = 1024
};

/* Exercise any compatible pair of initialized streams. Destroying the writer
 before reading is important because some writers buffer their output. */
typedef void InitReaderFn(Reader *reader, void *stream, unsigned int history_log2);

static void test_random_data(Writer *const writer,
                             InitReaderFn *const init_reader,
                             void *const stream,
                             const unsigned int history_log2)
{
  size_t chunk_size = 0;

  srand(RandomSeed);

  for (size_t done = 0; done < DataSize; done += chunk_size) {
    chunk_size = 1 + (size_t)rand() % (1 + MaxChunkSize);
    if (chunk_size > DataSize - done) {
      chunk_size = DataSize - done;
    }

    unsigned char expected[MaxChunkSize];

    for (size_t i = 0; i < chunk_size; ++i) {
      expected[i] = (unsigned char)(rand() & UCHAR_MAX);
    }

    assert(writer_fwrite(expected, 1, chunk_size, writer) == chunk_size);
  }
  assert(!writer_ferror(writer));
  assert(writer_destroy(writer) == DataSize);

  Reader reader;
  init_reader(&reader, stream, history_log2);

  srand(RandomSeed);
  
  for (size_t done = 0; done < DataSize; done += chunk_size) {
    chunk_size = 1 + (size_t)rand() % (1 + MaxChunkSize);
    if (chunk_size > DataSize - done) {
      chunk_size = DataSize - done;
    }

    unsigned char actual[MaxChunkSize];

    assert(reader_fread(actual, 1, chunk_size, &reader) == chunk_size);
    for (size_t i = 0; i < chunk_size; ++i) {
      assert(actual[i] == (unsigned char)(rand() & UCHAR_MAX));
    }
  }

  assert(!reader_ferror(&reader));
  assert(reader_fgetc(&reader) == EOF);
  assert(reader_feof(&reader));
  reader_destroy(&reader);
}

static void init_raw_reader(Reader *const reader, void *const stream,
                            const unsigned int history_log2)
{
  NOT_USED(history_log2);
  FILE *const file = stream;
  assert(!fseek(file, 0, SEEK_SET));
  reader_raw_init(reader, file);
}

static void init_gkey_reader(Reader *const reader, void *const stream,
                             const unsigned int history_log2)
{
  FILE *const file = stream;
  assert(!fseek(file, 0, SEEK_SET));
  assert(reader_gkey_init(reader, history_log2, file));
}

static void init_mem_reader(Reader *const reader, void *const stream,
                            const unsigned int history_log2)
{
  NOT_USED(history_log2);
  assert(reader_mem_init(reader, stream, DataSize));
}

static void test_raw(void)
{
  Writer writer;
  _Optional FILE *const file = tmpfile();
  assert(file != NULL);

  writer_raw_init(&writer, &*file);
  test_random_data(&writer, init_raw_reader, &*file, 0);

  assert(!fclose(&*file));
}

static void test_gkey(void)
{
  for (unsigned int history_log2 = 0; history_log2 < MaxHistoryLog2; ++history_log2)
  {
    _Optional FILE *const file = tmpfile();
    assert(file != NULL);

    Writer writer;
    assert(writer_gkey_init(&writer, history_log2, 0, &*file));
    test_random_data(&writer, init_gkey_reader, &*file, history_log2);

    assert(!fclose(&*file));
  }
}

static void test_mem(void)
{
  Writer writer;
  _Optional unsigned char *const buffer = malloc(DataSize);
  assert(buffer != NULL);

  assert(writer_mem_init(&writer, &*buffer, DataSize));
  test_random_data(&writer, init_mem_reader, &*buffer, 0);

  free(buffer);
}

#ifdef ACORN_FLEX
static void init_flex_reader(Reader *const reader, void *const stream,
                             const unsigned int history_log2)
{
  NOT_USED(history_log2);
  reader_flex_init(reader, stream);
}

static void test_flex(void)
{
  Writer writer;
  void *anchor = NULL;

  assert(flex_alloc(&anchor, 0));
  writer_flex_init(&writer, &anchor);
  test_random_data(&writer, init_flex_reader, &anchor);

  flex_free(&anchor);
}
#endif

void ReaderWriter_tests(void)
{
  static const struct {
    const char *test_name;
    void (*test_func)(void);
  } tests[] = {
    {"Write raw and read back", test_raw},
    {"Write compressed and read back", test_gkey},
    {"Write to memory and read back", test_mem},
#ifdef ACORN_FLEX
    {"Write to Flex and read back", test_flex},
#endif
  };

  for (size_t i = 0; i < ARRAY_SIZE(tests); ++i) {
    printf("Test %zu/%zu : %s\n", 1 + i, ARRAY_SIZE(tests),
           tests[i].test_name);

    Fortify_EnterScope();
    tests[i].test_func();
    Fortify_LeaveScope();
  }
}

