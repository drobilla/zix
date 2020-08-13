/*
  Copyright 2020 David Robillard <http://drobilla.net>

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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char** argv)
{
  static const uint16_t max_value = 1024; // 1u << 15u;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s SEED N_OPS OUTPUT_FILE\n", argv[0]);
    return 1;
  }

  const int         seed         = atoi(argv[1]);
  const size_t      n_ops        = (size_t)atol(argv[2]);
  const char* const out_filename = argv[3];
  FILE* const       output       = fopen(out_filename, "w");
  if (!output) {
    fprintf(stderr, "Usage: %s SEED OUTPUT_FILE\n", argv[0]);
    return 2;
  }

  srand((unsigned)seed);

  for (size_t i = 0; i < n_ops; ++i) {
    const uint16_t remove = (uint16_t)(rand() % 2);
    const uint16_t value  = (uint16_t)(rand() % max_value);

    assert(remove == 0 || remove == 1);
    assert(value < max_value);

    const uint16_t instruction = (uint16_t)(remove << 15u) | value;
    if (fwrite(&instruction, sizeof(instruction), 1, output) != 1) {
      fprintf(stderr, "error: Error writing instruction\n");
      return 3;
    }
  }

  fclose(output);
  return 0;
}
