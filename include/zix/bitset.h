// Copyright 2014-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_BITSET_H
#define ZIX_BITSET_H

#include "zix/attributes.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
   @addtogroup zix
   @{
   @name Bitset
   @{
*/

/// A bitset (always referred to by pointer, actually an array)
typedef unsigned long ZixBitset;

/// Tally of the number of bits in one ZixBitset element
typedef uint8_t ZixBitsetTally;

/// The number of bits per ZixBitset array element
#define ZIX_BITSET_BITS_PER_ELEM (CHAR_BIT * sizeof(ZixBitset))

/// The number of bitset elements needed for the given number of bits
#define ZIX_BITSET_ELEMS(n_bits)           \
  (((n_bits) / ZIX_BITSET_BITS_PER_ELEM) + \
   (((n_bits) % ZIX_BITSET_BITS_PER_ELEM) ? 1 : 0))

/// Clear a Bitset
ZIX_API
void
zix_bitset_clear(ZixBitset* ZIX_NONNULL      b,
                 ZixBitsetTally* ZIX_NONNULL t,
                 size_t                      n_bits);

/// Set bit `i` in `t` to 1
ZIX_API
void
zix_bitset_set(ZixBitset* ZIX_NONNULL      b,
               ZixBitsetTally* ZIX_NONNULL t,
               size_t                      i);

/// Clear bit `i` in `t` (set to 0)
ZIX_API
void
zix_bitset_reset(ZixBitset* ZIX_NONNULL      b,
                 ZixBitsetTally* ZIX_NONNULL t,
                 size_t                      i);

/// Return the `i`th bit in `t`
ZIX_PURE_API
bool
zix_bitset_get(const ZixBitset* ZIX_NONNULL b, size_t i);

/// Return the number of set bits in `b` up to bit `i` (non-inclusive)
ZIX_PURE_API
size_t
zix_bitset_count_up_to(const ZixBitset* ZIX_NONNULL      b,
                       const ZixBitsetTally* ZIX_NONNULL t,
                       size_t                            i);

/**
   Return the number of set bits in `b` up to bit `i` (non-inclusive) if bit
   `i` is set, or (size_t)-1 otherwise.
*/
ZIX_PURE_API
size_t
zix_bitset_count_up_to_if(const ZixBitset* ZIX_NONNULL      b,
                          const ZixBitsetTally* ZIX_NONNULL t,
                          size_t                            i);

/**
   @}
   @}
*/

#endif /* ZIX_BITSET_H */
