/*
  Copyright 2011-2021 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "bench.h"
#include "warnings.h"

#include "zix/common.h"
#include "zix/digest.h"
#include "zix/hash.h"

ZIX_DISABLE_GLIB_WARNINGS
#include <glib.h>
ZIX_RESTORE_WARNINGS

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
  char*  buf;
  size_t len;
} ZixChunk;

/// Linear Congruential Generator for making random 64-bit integers
static inline uint64_t
lcg64(const uint64_t i)
{
  static const uint64_t a = 6364136223846793005ull;
  static const uint64_t c = 1ull;

  return (a * i) + c;
}

ZIX_PURE_FUNC static const void*
identity(const void* record)
{
  return record;
}

static size_t
zix_chunk_hash(const ZixChunk* const chunk)
{
  return zix_digest(0u, chunk->buf, chunk->len);
}

static bool
zix_chunk_equal(const ZixChunk* a, const ZixChunk* b)
{
  return a->len == b->len && !memcmp(a->buf, b->buf, a->len);
}

static const unsigned seed = 1;

ZIX_LOG_FUNC(1, 2)
static int
test_fail(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, args);
  va_end(args);
  return 1;
}

int
main(int argc, char** argv)
{
  if (argc != 2) {
    return test_fail("Usage: %s INPUT_FILE\n", argv[0]);
  }

  const char* file = argv[1];
  FILE*       fd   = fopen(file, "r");
  if (!fd) {
    return test_fail("Failed to open file %s\n", file);
  }

  size_t max_n_strings = 1u << 20u;

  /* Read input strings */
  ZixChunk* chunks       = NULL;
  size_t    n_chunks     = 0;
  char*     buf          = (char*)calloc(1, 1);
  size_t    buf_len      = 1;
  size_t    this_str_len = 0;
  for (int c = 0; (c = fgetc(fd)) != EOF;) {
    if (c == '\n') {
      if (this_str_len == 0) {
        continue;
      }

      chunks = (ZixChunk*)realloc(chunks, (n_chunks + 1) * sizeof(ZixChunk));

      chunks[n_chunks].buf = (char*)malloc(buf_len);
      chunks[n_chunks].len = this_str_len;
      memcpy(chunks[n_chunks].buf, buf, buf_len);
      this_str_len = 0;
      if (++n_chunks == max_n_strings) {
        break;
      }
    } else {
      ++this_str_len;
      if (buf_len < this_str_len + 1) {
        buf_len = this_str_len + 1;
        buf     = (char*)realloc(buf, buf_len);
      }
      buf[this_str_len - 1] = (char)c;
      buf[this_str_len]     = '\0';
    }
  }

  fclose(fd);

  FILE* insert_dat = fopen("dict_insert.txt", "w");
  FILE* search_dat = fopen("dict_search.txt", "w");
  fprintf(insert_dat, "# n\tGHashTable\tZixHash\n");
  fprintf(search_dat, "# n\tGHashTable\tZixHash\n");

  for (size_t n = n_chunks / 16; n <= n_chunks; n *= 2) {
    printf("Benchmarking n = %zu\n", n);
    GHashTable* hash = g_hash_table_new(g_str_hash, g_str_equal);

    ZixHash* zhash = zix_hash_new(
      identity, (ZixHashFunc)zix_chunk_hash, (ZixEqualFunc)zix_chunk_equal);

    fprintf(insert_dat, "%zu", n);
    fprintf(search_dat, "%zu", n);

    // Benchmark insertion

    // GHashTable
    struct timespec insert_start = bench_start();
    for (size_t i = 0; i < n; ++i) {
      g_hash_table_insert(hash, chunks[i].buf, chunks[i].buf);
    }
    fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

    // ZixHash
    insert_start = bench_start();
    for (size_t i = 0; i < n; ++i) {
      ZixStatus st = zix_hash_insert(zhash, &chunks[i]);
      if (st && st != ZIX_STATUS_EXISTS) {
        return test_fail("Failed to insert `%s'\n", (const char*)chunks[i].buf);
      }
    }
    fprintf(insert_dat, "\t%lf\n", bench_end(&insert_start));

    // Benchmark search

    // GHashTable
    struct timespec search_start = bench_start();
    for (size_t i = 0; i < n; ++i) {
      const size_t index = (size_t)(lcg64(seed + i) % n);
      char*        match = (char*)g_hash_table_lookup(hash, chunks[index].buf);
      if (!!strcmp(match, chunks[index].buf)) {
        return test_fail(
          "GHashTable: Bad match `%s' for `%s'\n", match, chunks[index].buf);
      }
    }
    fprintf(search_dat, "\t%lf", bench_end(&search_start));

    // ZixHash
    search_start = bench_start();
    for (size_t i = 0; i < n; ++i) {
      const size_t    index = (size_t)(lcg64(seed + i) % n);
      const ZixChunk* match = NULL;
      if (!(match =
              (const ZixChunk*)zix_hash_find_record(zhash, &chunks[index]))) {
        return test_fail("Hash: Failed to find `%s'\n", chunks[index].buf);
      }

      if (!!strcmp(match->buf, chunks[index].buf)) {
        return test_fail(
          "ZixHash: Bad match `%s' for `%s'\n", match->buf, chunks[index].buf);
      }
    }
    fprintf(search_dat, "\t%lf\n", bench_end(&search_start));

    zix_hash_free(zhash);
    g_hash_table_unref(hash);
  }

  fclose(insert_dat);
  fclose(search_dat);

  fprintf(stderr, "Wrote dict_insert.txt dict_search.txt\n");

  return 0;
}
