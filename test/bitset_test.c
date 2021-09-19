// Copyright 2014-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "zix/bitset.h"

#include <assert.h>

#define N_BITS 256u
#define N_ELEMS (ZIX_BITSET_ELEMS(N_BITS))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

int
main(void)
{
  ZixBitset      b[N_ELEMS];
  ZixBitsetTally t[N_ELEMS];

  zix_bitset_clear(b, t, N_BITS);
  assert(zix_bitset_count_up_to(b, t, N_BITS) == 0);

  for (size_t i = 0; i < N_BITS; ++i) {
    zix_bitset_set(b, t, i);
    assert(zix_bitset_get(b, i));

    const size_t count = zix_bitset_count_up_to(b, t, N_BITS);
    assert(count == i + 1);
  }

  for (size_t i = 0; i <= N_BITS; ++i) {
    const size_t count = zix_bitset_count_up_to(b, t, i);
    assert(count == i);
  }

  for (size_t i = 0; i <= N_BITS; ++i) {
    if (i < N_BITS) {
      zix_bitset_reset(b, t, i);
    }

    const size_t count = zix_bitset_count_up_to(b, t, i);
    assert(count == 0);
  }

  zix_bitset_clear(b, t, N_BITS);
  for (size_t i = 0; i < N_BITS; i += 2) {
    zix_bitset_set(b, t, i);

    const size_t count  = zix_bitset_count_up_to(b, t, i + 1);
    const size_t result = MIN(N_BITS / 2, i / 2 + 1);

    assert(count == result);
  }

  zix_bitset_clear(b, t, N_BITS);
  for (size_t i = 0; i < N_BITS; ++i) {
    if (i % 2 == 0) {
      zix_bitset_set(b, t, i);

      const size_t count  = zix_bitset_count_up_to_if(b, t, i);
      const size_t result = MIN(N_BITS / 2, i / 2);
      assert(count == result);
    } else {
      assert(zix_bitset_count_up_to_if(b, t, i) == (size_t)-1);
    }
  }

  return 0;
}
