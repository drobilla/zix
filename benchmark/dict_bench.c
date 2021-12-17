// Copyright 2011-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "bench.h"
#include "warnings.h"

#include "zix/attributes.h"
#include "zix/common.h"
#include "zix/digest.h"
#include "zix/hash.h"

ZIX_DISABLE_GLIB_WARNINGS
#include <glib.h>
ZIX_RESTORE_WARNINGS

#include <assert.h>
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

typedef struct {
  ZixChunk* chunks;
  size_t    n_chunks;
  char*     buf;
} Inputs;

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

static Inputs
read_inputs(FILE* const fd)
{
  static const size_t max_n_strings = 1u << 20u;
  static const Inputs no_inputs     = {NULL, 0u, NULL};

  Inputs inputs       = {NULL, 0u, NULL};
  size_t buf_len      = 1;
  size_t this_str_len = 0;
  for (int c = 0; (c = fgetc(fd)) != EOF;) {
    if (c == '\n') {
      if (this_str_len == 0) {
        continue;
      }

      ZixChunk* const new_chunks = (ZixChunk*)realloc(
        inputs.chunks, (inputs.n_chunks + 1) * sizeof(ZixChunk));

      if (!new_chunks) {
        free(inputs.chunks);
        free(inputs.buf);
        return no_inputs;
      }

      inputs.chunks                      = new_chunks;
      inputs.chunks[inputs.n_chunks].buf = (char*)malloc(buf_len);
      inputs.chunks[inputs.n_chunks].len = this_str_len;
      memcpy(inputs.chunks[inputs.n_chunks].buf, inputs.buf, buf_len);
      this_str_len = 0;
      if (++inputs.n_chunks == max_n_strings) {
        break;
      }
    } else {
      ++this_str_len;
      if (buf_len < this_str_len + 1) {
        buf_len = this_str_len + 1;

        char* const new_buf = (char*)realloc(inputs.buf, buf_len);
        if (!new_buf) {
          free(inputs.chunks);
          free(inputs.buf);
          return no_inputs;
        }

        inputs.buf = new_buf;
      }
      inputs.buf[this_str_len - 1] = (char)c;
      inputs.buf[this_str_len]     = '\0';
    }
  }

  return inputs;
}

static int
run(FILE* const fd)
{
  Inputs inputs = read_inputs(fd);

  fclose(fd);

  FILE* insert_dat = fopen("dict_insert.txt", "w");
  FILE* search_dat = fopen("dict_search.txt", "w");
  fprintf(insert_dat, "# n\tGHashTable\tZixHash\n");
  fprintf(search_dat, "# n\tGHashTable\tZixHash\n");

  for (size_t n = inputs.n_chunks / 16; n <= inputs.n_chunks; n *= 2) {
    printf("Benchmarking n = %zu\n", n);
    GHashTable* hash = g_hash_table_new(g_str_hash, g_str_equal);

    ZixHash* zhash = zix_hash_new(NULL,
                                  identity,
                                  (ZixHashFunc)zix_chunk_hash,
                                  (ZixEqualFunc)zix_chunk_equal);

    fprintf(insert_dat, "%zu", n);
    fprintf(search_dat, "%zu", n);

    // Benchmark insertion

    // GHashTable
    struct timespec insert_start = bench_start();
    for (size_t i = 0; i < n; ++i) {
      g_hash_table_insert(hash, inputs.chunks[i].buf, inputs.chunks[i].buf);
    }
    fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

    // ZixHash
    insert_start = bench_start();
    for (size_t i = 0; i < n; ++i) {
      ZixStatus st = zix_hash_insert(zhash, &inputs.chunks[i]);
      assert(!st || st == ZIX_STATUS_EXISTS);
      (void)st;
    }
    fprintf(insert_dat, "\t%lf\n", bench_end(&insert_start));

    // Benchmark search

    // GHashTable
    struct timespec search_start = bench_start();
    for (size_t i = 0; i < n; ++i) {
      const size_t index = (size_t)(lcg64(seed + i) % n);
      char* volatile match =
        (char*)g_hash_table_lookup(hash, inputs.chunks[index].buf);

      assert(!strcmp(match, inputs.chunks[index].buf));
      (void)match;
    }
    fprintf(search_dat, "\t%lf", bench_end(&search_start));

    // ZixHash
    search_start = bench_start();
    for (size_t i = 0; i < n; ++i) {
      const size_t index = (size_t)(lcg64(seed + i) % n);
      const ZixChunk* volatile match =
        (const ZixChunk*)zix_hash_find_record(zhash, &inputs.chunks[index]);

      assert(match);
      assert(!strcmp(match->buf, inputs.chunks[index].buf));
      (void)match;
    }
    fprintf(search_dat, "\t%lf\n", bench_end(&search_start));

    zix_hash_free(zhash);
    g_hash_table_unref(hash);
  }

  fclose(insert_dat);
  fclose(search_dat);

  for (size_t i = 0; i < inputs.n_chunks; ++i) {
    free(inputs.chunks[i].buf);
  }

  free(inputs.chunks);
  free(inputs.buf);

  fprintf(stderr, "Wrote dict_insert.txt dict_search.txt\n");
  return 0;
}

int
main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s INPUT_FILE\n", argv[0]);
    return 1;
  }

  const char* file = argv[1];
  FILE*       fd   = fopen(file, "r");
  if (!fd) {
    fprintf(stderr, "error: Failed to open file %s\n", file);
    return 1;
  }

  return run(fd);
}
