// Copyright 2011-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "failing_allocator.h"
#include "test_data.h"

#include "zix/allocator.h"
#include "zix/attributes.h"
#include "zix/status.h"
#include "zix/tree.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uintptr_t seed = 1;

static int
int_cmp(const void* a, const void* b, const void* ZIX_UNUSED(user_data))
{
  const uintptr_t ia = (uintptr_t)a;
  const uintptr_t ib = (uintptr_t)b;

  return ia < ib ? -1 : ia > ib ? 1 : 0;
}

static uintptr_t
ith_elem(unsigned test_num, size_t n_elems, size_t i)
{
  switch (test_num % 3) {
  case 0:
    return i; // Increasing (worst case)
  case 1:
    return n_elems - i; // Decreasing (worse case)
  case 2:
  default:
    return lcg(seed + i) % 100U; // Random
  }
}

ZIX_LOG_FUNC(1, 2)
static int
test_fail(const char* const fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, args);

  va_end(args);
  return EXIT_FAILURE;
}

static void
test_duplicate_insert(void)
{
  const uintptr_t r  = 0xDEADBEEF;
  ZixTreeIter*    ti = NULL;
  ZixTree*        t  = zix_tree_new(NULL, false, int_cmp, NULL, NULL, NULL);

  assert(!zix_tree_begin(t));
  assert(!zix_tree_end(t));
  assert(!zix_tree_rbegin(t));
  assert(!zix_tree_rend(t));

  assert(!zix_tree_insert(t, (void*)r, &ti));
  assert((uintptr_t)zix_tree_get(ti) == r);
  assert(zix_tree_insert(t, (void*)r, &ti) == ZIX_STATUS_EXISTS);

  zix_tree_free(t);
}

static int
check_value(const uintptr_t actual, const uintptr_t expected)
{
  return (expected == actual)
           ? 0
           : test_fail("Data corrupt (%" PRIuPTR " != %" PRIuPTR ")\n",
                       actual,
                       expected);
}

static int
check_tree_size(const size_t actual, const size_t expected)
{
  return (expected == actual)
           ? 0
           : test_fail(
               "Tree size %" PRIuPTR " != %" PRIuPTR "\n", actual, expected);
}

static int
insert_elements(ZixTree* const t, const unsigned test_num, const size_t n_elems)
{
  ZixTreeIter* ti = NULL;

  for (size_t i = 0; i < n_elems; ++i) {
    const uintptr_t r = ith_elem(test_num, n_elems, i);

    ZixStatus status = zix_tree_insert(t, (void*)r, &ti);
    if (status) {
      return test_fail("Insert failed\n");
    }

    if (check_value((uintptr_t)zix_tree_get(ti), r)) {
      return EXIT_FAILURE;
    }
  }

  return 0;
}

static int
stress(ZixAllocator* allocator, unsigned test_num, size_t n_elems)
{
  uintptr_t    r  = 0U;
  ZixTreeIter* ti = NULL;
  ZixTree*     t  = zix_tree_new(allocator, true, int_cmp, NULL, NULL, NULL);

  assert(t);
  assert(!zix_tree_begin(t));
  assert(!zix_tree_end(t));
  assert(!zix_tree_rbegin(t));
  assert(!zix_tree_rend(t));

  // Insert n_elems elements
  if (insert_elements(t, test_num, n_elems)) {
    return EXIT_FAILURE;
  }

  // Ensure tree size is correct
  if (check_tree_size(zix_tree_size(t), n_elems)) {
    return EXIT_FAILURE;
  }

  // Search for all elements
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (zix_tree_find(t, (void*)r, &ti)) {
      return test_fail("Find failed\n");
    }

    if (check_value((uintptr_t)zix_tree_get(ti), r)) {
      return EXIT_FAILURE;
    }
  }

  // Iterate over all elements
  size_t    i    = 0;
  uintptr_t last = 0;
  for (ZixTreeIter* iter = zix_tree_begin(t); !zix_tree_iter_is_end(iter);
       iter              = zix_tree_iter_next(iter), ++i) {
    const uintptr_t iter_data = (uintptr_t)zix_tree_get(iter);
    if (iter_data < last) {
      return test_fail(
        "Iter corrupt (%" PRIuPTR " < %" PRIuPTR ")\n", iter_data, last);
    }
    last = iter_data;
  }
  if (i != n_elems) {
    return test_fail(
      "Iteration stopped at %" PRIuPTR "/%" PRIuPTR " elements\n", i, n_elems);
  }

  // Iterate over all elements backwards
  i    = 0;
  last = INTPTR_MAX;
  for (ZixTreeIter* iter = zix_tree_rbegin(t); !zix_tree_iter_is_rend(iter);
       iter              = zix_tree_iter_prev(iter), ++i) {
    const uintptr_t iter_data = (uintptr_t)zix_tree_get(iter);
    if (iter_data > last) {
      return test_fail(
        "Iter corrupt (%" PRIuPTR " < %" PRIuPTR ")\n", iter_data, last);
    }
    last = iter_data;
  }

  // Delete all elements
  for (size_t e = 0; e < n_elems; e++) {
    r = ith_elem(test_num, n_elems, e);

    ZixTreeIter* item = NULL;
    if (zix_tree_find(t, (void*)r, &item) != ZIX_STATUS_SUCCESS) {
      return test_fail("Failed to find item to remove\n");
    }
    if (zix_tree_remove(t, item)) {
      return test_fail("Error removing item\n");
    }
  }

  // Ensure the tree is empty
  if (check_tree_size(zix_tree_size(t), 0)) {
    return EXIT_FAILURE;
  }

  // Insert n_elems elements again (to test non-empty destruction)
  if (insert_elements(t, test_num, n_elems)) {
    return EXIT_FAILURE;
  }

  // Ensure tree size is correct
  const int ret = check_tree_size(zix_tree_size(t), n_elems);

  zix_tree_free(t);

  return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}

static void
test_failed_alloc(void)
{
  static const size_t n_insertions = 4096;

  ZixFailingAllocator allocator = zix_failing_allocator();

  // Successfully test insertions to count the number of allocations
  ZixTree* t = zix_tree_new(&allocator.base, false, int_cmp, NULL, NULL, NULL);
  for (size_t i = 0U; i < n_insertions; ++i) {
    assert(!zix_tree_insert(t, (void*)i, NULL));
  }

  // Test that tree allocation failure is handled gracefully
  allocator.n_remaining = 0;
  zix_tree_free(t);
  assert(!zix_tree_new(&allocator.base, false, int_cmp, NULL, NULL, NULL));

  // Allocate a new tree to try the same test again, but with allocation failure
  allocator.n_remaining = 1;
  t = zix_tree_new(&allocator.base, false, int_cmp, NULL, NULL, NULL);
  assert(t);

  // Test that each insertion allocation failing is handled gracefully
  for (size_t i = 0U; i < n_insertions; ++i) {
    allocator.n_remaining = 0;

    assert(zix_tree_insert(t, (void*)i, NULL) == ZIX_STATUS_NO_MEM);
  }

  zix_tree_free(t);
}

int
main(int argc, char** argv)
{
  const unsigned n_tests = 3;
  unsigned       n_elems = 0;

  assert(!zix_tree_iter_next(NULL));
  assert(!zix_tree_iter_prev(NULL));

  test_duplicate_insert();
  test_failed_alloc();

  if (argc == 1) {
    n_elems = 100000;
  } else {
    n_elems = (unsigned)strtoul(argv[1], NULL, 10);
    if (argc > 2) {
      seed = strtoul(argv[2], NULL, 10);
    } else {
      seed = (uintptr_t)time(NULL);
    }
  }

  if (n_elems == 0) {
    fprintf(stderr, "USAGE: %s [N_ELEMS]\n", argv[0]);
    return 1;
  }

  printf(
    "Running %u tests with %u elements (seed %zu)", n_tests, n_elems, seed);

  for (unsigned i = 0; i < n_tests; ++i) {
    printf(".");
    fflush(stdout);
    if (stress(NULL, i, n_elems)) {
      return test_fail("Failure with random seed %zu\n", seed);
    }
  }

  printf("\n");
  return EXIT_SUCCESS;
}
