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

#ifndef ZIX_TEST_DATA_H
#define ZIX_TEST_DATA_H

#include <stddef.h>
#include <stdint.h>

/// Linear Congruential Generator for making random 32-bit integers
static inline uint32_t
lcg32(const uint32_t i)
{
  static const uint32_t a = 134775813u;
  static const uint32_t c = 1u;

  return (a * i) + c;
}

/// Linear Congruential Generator for making random 64-bit integers
static inline uint64_t
lcg64(const uint64_t i)
{
  static const uint64_t a = 6364136223846793005ull;
  static const uint64_t c = 1ull;

  return (a * i) + c;
}

/// Linear Congruential Generator for making random pointer-sized integers
static inline uintptr_t
lcg(const uintptr_t i)
{
#if UINTPTR_MAX >= UINT64_MAX
  return lcg64(i);
#else
  return lcg32(i);
#endif
}

/// Return a pseudo-pseudo-pseudo-random-ish integer with no duplicates
static inline size_t
unique_rand(size_t i)
{
  i ^= 0x5CA1AB1Eu; // Juggle bits to avoid linear clumps

  // Largest prime < 2^32 which satisfies (2^32 = 3 mod 4)
  static const size_t prime = 4294967291u;
  if (i >= prime) {
    return i; // Values >= prime are mapped to themselves
  }

  const size_t residue = (size_t)(((uint64_t)i * i) % prime);
  return (i <= prime / 2) ? residue : prime - residue;
}

#endif // ZIX_TEST_DATA_H
