/*
  Copyright 2011-2019 David Robillard <http://drobilla.net>

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

#include "zix/btree.h"

#include "test_malloc.h"

#include "zix/common.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool expect_failure = false;

// Return a pseudo-pseudo-pseudo-random-ish integer with no duplicates
static uintptr_t
unique_rand(size_t i)
{
  i ^= 0x00005CA1Eu; // Juggle bits to avoid linear clumps

  // Largest prime < 2^32 which satisfies (2^32 = 3 mod 4)
  static const uint32_t prime = 4294967291;
  if (i >= prime) {
    return i; // Values >= prime are mapped to themselves
  }

  const uint64_t residue = ((uint64_t)i * i) % prime;
  return (i <= prime / 2) ? residue : prime - residue;
}

static int
int_cmp(const void* a, const void* b, const void* ZIX_UNUSED(user_data))
{
  const uintptr_t ia = (uintptr_t)a;
  const uintptr_t ib = (uintptr_t)b;

  return ia < ib ? -1 : ia > ib ? 1 : 0;
}

static uintptr_t
ith_elem(const unsigned test_num, const size_t n_elems, const size_t i)
{
  switch (test_num % 3) {
  case 0:
    return i + 1; // Increasing
  case 1:
    return n_elems - i; // Decreasing
  default:
    return unique_rand(i); // Pseudo-random
  }
}

static void
destroy(void* ZIX_UNUSED(ptr))
{
}

typedef struct {
  unsigned test_num;
  size_t   n_elems;
} TestContext;

static uintptr_t
wildcard_cut(unsigned test_num, size_t n_elems)
{
  return ith_elem(test_num, n_elems, n_elems / 3);
}

/** Wildcard comparator where 0 matches anything >= wildcard_cut(n_elems). */
static int
wildcard_cmp(const void* a, const void* b, const void* user_data)
{
  const TestContext* ctx      = (const TestContext*)user_data;
  const size_t       n_elems  = ctx->n_elems;
  const unsigned     test_num = ctx->test_num;
  const uintptr_t    ia       = (uintptr_t)a;
  const uintptr_t    ib       = (uintptr_t)b;

  if (ia == 0) {
    if (ib >= wildcard_cut(test_num, n_elems)) {
      return 0; // Wildcard match
    }

    return 1; // Wildcard a > b
  }

  if (ib == 0) {
    if (ia >= wildcard_cut(test_num, n_elems)) {
      return 0; // Wildcard match
    }

    return -1; // Wildcard b > a
  }

  return int_cmp(a, b, user_data);
}

ZIX_LOG_FUNC(2, 3)
static int
test_fail(ZixBTree* t, const char* fmt, ...)
{
  zix_btree_free(t);
  if (expect_failure) {
    return EXIT_SUCCESS;
  }
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, args);
  va_end(args);
  return EXIT_FAILURE;
}

static int
stress(const unsigned test_num, const size_t n_elems)
{
  if (n_elems == 0) {
    return 0;
  }

  uintptr_t r  = 0;
  ZixBTree* t  = zix_btree_new(int_cmp, NULL, NULL);
  ZixStatus st = ZIX_STATUS_SUCCESS;

  if (!t) {
    return test_fail(t, "Failed to allocate tree\n");
  }

  // Ensure begin iterator is end on empty tree
  ZixBTreeIter* ti  = zix_btree_begin(t);
  ZixBTreeIter* end = zix_btree_end(t);
  if (!ti) {
    return test_fail(t, "Failed to allocate iterator\n");
  }

  if (!zix_btree_iter_is_end(ti)) {
    return test_fail(t, "Begin iterator on empty tree is not end\n");
  }

  if (!zix_btree_iter_equals(ti, end)) {
    return test_fail(t, "Begin and end of empty tree are not equal\n");
  }

  zix_btree_iter_free(end);
  zix_btree_iter_free(ti);

  if (zix_btree_iter_copy(NULL)) {
    return test_fail(t, "Copy of null iterator returned non-null\n");
  }

  // Insert n_elems elements
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (!zix_btree_find(t, (void*)r, &ti)) {
      return test_fail(t, "%" PRIuPTR " already in tree\n", (uintptr_t)r);
    }

    if ((st = zix_btree_insert(t, (void*)r))) {
      return test_fail(
        t, "Insert %" PRIuPTR " failed (%s)\n", (uintptr_t)r, zix_strerror(st));
    }
  }

  // Ensure tree size is correct
  if (zix_btree_size(t) != n_elems) {
    return test_fail(t, "Tree size %zu != %zu\n", zix_btree_size(t), n_elems);
  }

  // Ensure begin no longer equals end
  ti  = zix_btree_begin(t);
  end = zix_btree_end(t);
  if (zix_btree_iter_equals(ti, end)) {
    return test_fail(t, "Begin and end of non-empty tree are equal\n");
  }
  zix_btree_iter_free(end);

  // Ensure non-null iterator copying works
  ZixBTreeIter* begin_copy = zix_btree_iter_copy(ti);
  if (!zix_btree_iter_equals(ti, begin_copy)) {
    return test_fail(t, "Iterator copy is not equal to original\n");
  }
  zix_btree_iter_free(begin_copy);
  zix_btree_iter_free(ti);

  // Search for all elements
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (zix_btree_find(t, (void*)r, &ti)) {
      return test_fail(t, "Find %" PRIuPTR " @ %zu failed\n", (uintptr_t)r, i);
    }

    if ((uintptr_t)zix_btree_get(ti) != r) {
      return test_fail(t,
                       "Search data corrupt (%" PRIuPTR " != %" PRIuPTR ")\n",
                       (uintptr_t)zix_btree_get(ti),
                       r);
    }

    zix_btree_iter_free(ti);
  }

  if (zix_btree_lower_bound(NULL, (void*)r, &ti) != ZIX_STATUS_BAD_ARG) {
    return test_fail(t, "Lower bound on NULL tree succeeded\n");
  }

  // Find the lower bound of all elements and ensure it's exact
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (zix_btree_lower_bound(t, (void*)r, &ti)) {
      return test_fail(
        t, "Lower bound %" PRIuPTR " @ %zu failed\n", (uintptr_t)r, i);
    }

    if (zix_btree_iter_is_end(ti)) {
      return test_fail(
        t, "Lower bound %" PRIuPTR " @ %zu hit end\n", (uintptr_t)r, i);
    }

    if ((uintptr_t)zix_btree_get(ti) != r) {
      return test_fail(t,
                       "Lower bound corrupt (%" PRIuPTR " != %" PRIuPTR "\n",
                       (uintptr_t)zix_btree_get(ti),
                       r);
    }

    zix_btree_iter_free(ti);
  }

  // Search for elements that don't exist
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems * 3, n_elems + i);
    if (!zix_btree_find(t, (void*)r, &ti)) {
      return test_fail(t, "Unexpectedly found %" PRIuPTR "\n", (uintptr_t)r);
    }
  }

  // Iterate over all elements
  size_t    i    = 0;
  uintptr_t last = 0;
  for (ti = zix_btree_begin(t); !zix_btree_iter_is_end(ti);
       zix_btree_iter_increment(ti), ++i) {
    const uintptr_t iter_data = (uintptr_t)zix_btree_get(ti);
    if (iter_data < last) {
      return test_fail(t,
                       "Iter @ %zu corrupt (%" PRIuPTR " < %" PRIuPTR ")\n",
                       i,
                       iter_data,
                       last);
    }
    last = iter_data;
  }
  zix_btree_iter_free(ti);
  if (i != n_elems) {
    return test_fail(t, "Iteration stopped at %zu/%zu elements\n", i, n_elems);
  }

  // Insert n_elems elements again, ensuring duplicates fail
  for (i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (!zix_btree_insert(t, (void*)r)) {
      return test_fail(t, "Duplicate insert succeeded\n");
    }
  }

  // Search for the middle element then iterate from there
  r = ith_elem(test_num, n_elems, n_elems / 2);
  if (zix_btree_find(t, (void*)r, &ti)) {
    return test_fail(t, "Find %" PRIuPTR " failed\n", (uintptr_t)r);
  }
  last = (uintptr_t)zix_btree_get(ti);
  zix_btree_iter_increment(ti);
  for (i = 1; !zix_btree_iter_is_end(ti); zix_btree_iter_increment(ti), ++i) {
    if ((uintptr_t)zix_btree_get(ti) == last) {
      return test_fail(
        t, "Duplicate element @ %" PRIuPTR " %" PRIuPTR "\n", i, last);
    }
    last = (uintptr_t)zix_btree_get(ti);
  }
  zix_btree_iter_free(ti);

  // Delete all elements
  ZixBTreeIter* next = NULL;
  for (size_t e = 0; e < n_elems; e++) {
    r = ith_elem(test_num, n_elems, e);

    uintptr_t removed = 0;
    if (zix_btree_remove(t, (void*)r, (void**)&removed, &next)) {
      return test_fail(t, "Error removing item %" PRIuPTR "\n", (uintptr_t)r);
    }

    if (removed != r) {
      return test_fail(t,
                       "Removed wrong item %" PRIuPTR " != %" PRIuPTR "\n",
                       removed,
                       (uintptr_t)r);
    }

    if (test_num == 0) {
      const uintptr_t next_value = ith_elem(test_num, n_elems, e + 1);
      if (!((zix_btree_iter_is_end(next) && e == n_elems - 1) ||
            (uintptr_t)zix_btree_get(next) == next_value)) {
        return test_fail(t,
                         "Delete all next iterator %" PRIuPTR " != %" PRIuPTR
                         "\n",
                         (uintptr_t)zix_btree_get(next),
                         next_value);
      }
    }
  }
  zix_btree_iter_free(next);
  next = NULL;

  // Ensure the tree is empty
  if (zix_btree_size(t) != 0) {
    return test_fail(t, "Tree size %zu != 0\n", zix_btree_size(t));
  }

  // Insert n_elems elements again (to test non-empty destruction)
  for (size_t e = 0; e < n_elems; ++e) {
    r = ith_elem(test_num, n_elems, e);
    if (zix_btree_insert(t, (void*)r)) {
      return test_fail(t, "Post-deletion insert failed\n");
    }
  }

  // Delete elements that don't exist
  for (size_t e = 0; e < n_elems; e++) {
    r                 = ith_elem(test_num, n_elems * 3, n_elems + e);
    uintptr_t removed = 0;
    if (!zix_btree_remove(t, (void*)r, (void**)&removed, &next)) {
      return test_fail(
        t, "Non-existant deletion of %" PRIuPTR " succeeded\n", (uintptr_t)r);
    }
  }
  zix_btree_iter_free(next);
  next = NULL;

  // Ensure tree size is still correct
  if (zix_btree_size(t) != n_elems) {
    return test_fail(t, "Tree size %zu != %zu\n", zix_btree_size(t), n_elems);
  }

  // Delete some elements towards the end
  for (size_t e = 0; e < n_elems / 4; e++) {
    r = ith_elem(test_num, n_elems, n_elems - (n_elems / 4) + e);
    uintptr_t removed = 0;
    if (zix_btree_remove(t, (void*)r, (void**)&removed, &next)) {
      return test_fail(t, "Deletion of %" PRIuPTR " failed\n", (uintptr_t)r);
    }

    if (removed != r) {
      return test_fail(t,
                       "Removed wrong item %" PRIuPTR " != %" PRIuPTR "\n",
                       removed,
                       (uintptr_t)r);
    }

    if (test_num == 0) {
      const uintptr_t next_value = ith_elem(test_num, n_elems, e + 1);
      if (!zix_btree_iter_is_end(next) &&
          (uintptr_t)zix_btree_get(next) == next_value) {
        return test_fail(t,
                         "Next iterator %" PRIuPTR " != %" PRIuPTR "\n",
                         (uintptr_t)zix_btree_get(next),
                         next_value);
      }
    }
  }
  zix_btree_iter_free(next);
  next = NULL;

  // Check tree size
  if (zix_btree_size(t) != n_elems - (n_elems / 4)) {
    return test_fail(t, "Tree size %zu != %zu\n", zix_btree_size(t), n_elems);
  }

  // Delete some elements in a random order
  for (size_t e = 0; e < zix_btree_size(t) / 2; e++) {
    r = ith_elem(test_num, n_elems, rand() % n_elems);

    uintptr_t removed = 0;
    ZixStatus rst     = zix_btree_remove(t, (void*)r, (void**)&removed, &next);
    if (rst != ZIX_STATUS_SUCCESS && rst != ZIX_STATUS_NOT_FOUND) {
      return test_fail(t, "Error deleting %" PRIuPTR "\n", (uintptr_t)r);
    }
  }
  zix_btree_iter_free(next);
  next = NULL;

  // Delete all remaining elements via next iterator
  next                 = zix_btree_begin(t);
  uintptr_t last_value = 0;
  while (!zix_btree_iter_is_end(next)) {
    const uintptr_t value   = (uintptr_t)zix_btree_get(next);
    uintptr_t       removed = 0;
    if (zix_btree_remove(t, zix_btree_get(next), (void**)&removed, &next)) {
      return test_fail(
        t, "Error removing next item %" PRIuPTR "\n", (uintptr_t)r);
    }

    if (removed != value) {
      return test_fail(t,
                       "Removed wrong next item %" PRIuPTR " != %" PRIuPTR "\n",
                       removed,
                       (uintptr_t)value);
    }

    if (removed < last_value) {
      return test_fail(t,
                       "Removed unordered next item %" PRIuPTR " < %" PRIuPTR
                       "\n",
                       removed,
                       (uintptr_t)value);
    }

    last_value = removed;
  }
  assert(!next || zix_btree_size(t) == 0);
  zix_btree_iter_free(next);
  next = NULL;

  zix_btree_free(t);

  // Test lower_bound with wildcard comparator

  TestContext ctx = {test_num, n_elems};
  if (!(t = zix_btree_new(wildcard_cmp, &ctx, destroy))) {
    return test_fail(t, "Failed to allocate tree\n");
  }

  // Insert n_elems elements
  for (i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (r != 0) { // Can't insert wildcards
      if ((st = zix_btree_insert(t, (void*)r))) {
        return test_fail(t,
                         "Insert %" PRIuPTR " failed (%s)\n",
                         (uintptr_t)r,
                         zix_strerror(st));
      }
    }
  }

  // Find lower bound of wildcard
  const uintptr_t wildcard = 0;
  if (zix_btree_lower_bound(t, (void*)wildcard, &ti)) {
    return test_fail(t, "Lower bound failed\n");
  }

  if (zix_btree_iter_is_end(ti)) {
    return test_fail(t, "Lower bound reached end\n");
  }

  // Check value
  const uintptr_t iter_data = (uintptr_t)zix_btree_get(ti);
  const uintptr_t cut       = wildcard_cut(test_num, n_elems);
  if (iter_data != cut) {
    return test_fail(
      t, "Lower bound %" PRIuPTR " != %" PRIuPTR "\n", iter_data, cut);
  }

  if (wildcard_cmp((void*)wildcard, (void*)iter_data, &ctx)) {
    return test_fail(
      t, "Wildcard lower bound %" PRIuPTR " != %" PRIuPTR "\n", iter_data, cut);
  }

  zix_btree_iter_free(ti);

  // Find lower bound of value past end
  const uintptr_t max = (uintptr_t)-1;
  if (zix_btree_lower_bound(t, (void*)max, &ti)) {
    return test_fail(t, "Lower bound failed\n");
  }

  if (!zix_btree_iter_is_end(ti)) {
    return test_fail(t, "Lower bound of maximum value is not end\n");
  }

  zix_btree_iter_free(ti);
  zix_btree_free(t);

  return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [N_ELEMS]\n", argv[0]);
    return EXIT_FAILURE;
  }

  const unsigned n_tests = 3;
  unsigned       n_elems = (argc > 1) ? (unsigned)atol(argv[1]) : 524288u;

  printf("Running %u tests with %u elements", n_tests, n_elems);
  for (unsigned i = 0; i < n_tests; ++i) {
    printf(".");
    fflush(stdout);
    if (stress(i, n_elems)) {
      return EXIT_FAILURE;
    }
  }
  printf("\n");

#ifdef ZIX_WITH_TEST_MALLOC
  const size_t total_n_allocs = test_malloc_get_n_allocs();
  const size_t fail_n_elems   = 1000;
  printf("Testing 0 ... %zu failed allocations\n", total_n_allocs);
  expect_failure = true;
  for (size_t i = 0; i < total_n_allocs; ++i) {
    test_malloc_reset(i);
    stress(0, fail_n_elems);
    if (i > test_malloc_get_n_allocs()) {
      break;
    }
  }

  test_malloc_reset((size_t)-1);
#endif

  return EXIT_SUCCESS;
}
