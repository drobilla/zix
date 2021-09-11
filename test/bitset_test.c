/*
  Copyright 2014-2020 David Robillard <d@drobilla.net>

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

#include "zix/attributes.h"
#include "zix/bitset.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#define N_BITS 256u
#define N_ELEMS (ZIX_BITSET_ELEMS(N_BITS))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

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
main(void)
{
  ZixBitset      b[N_ELEMS];
  ZixBitsetTally t[N_ELEMS];

  zix_bitset_clear(b, t, N_BITS);
  if (zix_bitset_count_up_to(b, t, N_BITS) != 0) {
    return test_fail("Cleared bitset has non-zero count\n");
  }

  for (size_t i = 0; i < N_BITS; ++i) {
    zix_bitset_set(b, t, i);
    const size_t count = zix_bitset_count_up_to(b, t, N_BITS);
    if (count != i + 1) {
      return test_fail("Count %" PRIuPTR " != %" PRIuPTR "\n", count, i + 1);
    }

    if (!zix_bitset_get(b, i)) {
      return test_fail("Bit %" PRIuPTR " is not set\n", i);
    }
  }

  for (size_t i = 0; i <= N_BITS; ++i) {
    const size_t count = zix_bitset_count_up_to(b, t, i);
    if (count != i) {
      return test_fail(
        "Count to %" PRIuPTR " is %" PRIuPTR " != %" PRIuPTR "\n", i, count, i);
    }
  }

  for (size_t i = 0; i <= N_BITS; ++i) {
    if (i < N_BITS) {
      zix_bitset_reset(b, t, i);
    }
    const size_t count = zix_bitset_count_up_to(b, t, i);
    if (count != 0) {
      return test_fail(
        "Count to %" PRIuPTR " is %" PRIuPTR " != %d\n", i, count, 0);
    }
  }

  zix_bitset_clear(b, t, N_BITS);
  for (size_t i = 0; i < N_BITS; i += 2) {
    zix_bitset_set(b, t, i);

    const size_t count  = zix_bitset_count_up_to(b, t, i + 1);
    const size_t result = MIN(N_BITS / 2, i / 2 + 1);
    if (count != result) {
      return test_fail("Count to %" PRIuPTR " is %" PRIuPTR " != %" PRIuPTR
                       "\n",
                       i,
                       count,
                       result);
    }
  }

  zix_bitset_clear(b, t, N_BITS);
  for (size_t i = 0; i < N_BITS; ++i) {
    if (i % 2 == 0) {
      zix_bitset_set(b, t, i);

      const size_t count  = zix_bitset_count_up_to_if(b, t, i);
      const size_t result = MIN(N_BITS / 2, i / 2);
      if (count != result) {
        return test_fail("Count to %" PRIuPTR " is %" PRIuPTR " != %" PRIuPTR
                         "\n",
                         i,
                         count,
                         result);
      }
    } else {
      if (zix_bitset_count_up_to_if(b, t, i) != (size_t)-1) {
        return test_fail(
          "Got unexpected non-zero count at index %" PRIuPTR "\n", i);
      }
    }
  }

  return 0;
}
