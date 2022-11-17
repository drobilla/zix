// Copyright 2014-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_BITSET_H
#define ZIX_BITSET_H

#include "zix/attributes.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
   @defgroup zix_bitset Bitset
   @ingroup zix_data_structures
   @{
*/

/**
   A bitset element.

   A bitset is an array that is always referred to by pointer.  This type is
   the type of an element of that array (which stores some number of bits), so
   a pointer to this type is a pointer to a bitset.
*/
typedef unsigned long ZixBitset;

/**
   Tally of the number of bits in one ZixBitset element.

   Like #ZixBitset, this is the type of one element of a tally, which is a
   parallel array to the bitset one, also always referred to by pointer.
*/
typedef uint8_t ZixBitsetTally;

/// The number of bits per ZixBitset array element
#define ZIX_BITSET_BITS_PER_ELEM (CHAR_BIT * sizeof(ZixBitset))

/// The number of bitset elements needed for the given number of bits
#define ZIX_BITSET_ELEMS(n_bits)           \
  (((n_bits) / ZIX_BITSET_BITS_PER_ELEM) + \
   (((n_bits) % ZIX_BITSET_BITS_PER_ELEM) ? 1 : 0))

/**
   Clear a bitset.

   @param b The bitset to clear.
   @param t The tally for `b` to update.
   @param n_bits The number of bits in `b`.
*/
ZIX_API
void
zix_bitset_clear(ZixBitset* ZIX_NONNULL      b,
                 ZixBitsetTally* ZIX_NONNULL t,
                 size_t                      n_bits);

/**
   Set bit `i` in `b`.

   @param b The bitset to set bit `i` in.
   @param t The tally for `b` to update.
   @param i The index of the bit to set.
*/
ZIX_API
void
zix_bitset_set(ZixBitset* ZIX_NONNULL      b,
               ZixBitsetTally* ZIX_NONNULL t,
               size_t                      i);

/**
   Clear bit `i` in `b`.

   @param b The bitset to set bit `i` in.
   @param t The tally for `b` to update.
   @param i The index of the bit to set.
*/
ZIX_API
void
zix_bitset_reset(ZixBitset* ZIX_NONNULL      b,
                 ZixBitsetTally* ZIX_NONNULL t,
                 size_t                      i);

/**
   Return the bit at index `i` in `b`.

   @param b The bitset to access.
   @param i The index of the bit to return.
   @return True if the bit is set (1), false otherwise (0).
*/
ZIX_PURE_API
bool
zix_bitset_get(const ZixBitset* ZIX_NONNULL b, size_t i);

/**
   Return the number of set bits in `b` up to bit `i`.

   The returned count is non-inclusive, that is, doesn't include bit `i`.

   @param b The bitset to count.
   @param t The tally for `b`.
   @param i The end index to stop counting at.
   @return The number of set bits in `b` from bit 0 to bit `i - 1`.
*/
ZIX_PURE_API
size_t
zix_bitset_count_up_to(const ZixBitset* ZIX_NONNULL      b,
                       const ZixBitsetTally* ZIX_NONNULL t,
                       size_t                            i);

/**
   Return the number of set bits in `b` up to bit `i` if it is set.

   The returned count is non-inclusive, that is, doesn't include bit `i`.

   @return The number of set bits in `b` from bit 0 to bit `i - 1` if bit `i`
   is set, otherwise, `(size_t)-1`.
*/
ZIX_PURE_API
size_t
zix_bitset_count_up_to_if(const ZixBitset* ZIX_NONNULL      b,
                          const ZixBitsetTally* ZIX_NONNULL t,
                          size_t                            i);

/**
   @}
*/

#endif /* ZIX_BITSET_H */
