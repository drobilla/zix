// Copyright 2011-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "test_data.h"

#include "zix/attributes.h"
#include "zix/common.h"
#include "zix/tree.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static unsigned seed = 1;

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

static int
test_fail(void)
{
  return EXIT_FAILURE;
}

static int
stress(unsigned test_num, size_t n_elems)
{
  uintptr_t    r  = 0U;
  ZixTreeIter* ti = NULL;
  ZixTree*     t  = zix_tree_new(NULL, true, int_cmp, NULL, NULL, NULL);

  // Insert n_elems elements
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);

    ZixStatus status = zix_tree_insert(t, (void*)r, &ti);
    if (status) {
      fprintf(stderr, "Insert failed\n");
      return test_fail();
    }

    if ((uintptr_t)zix_tree_get(ti) != r) {
      fprintf(stderr,
              "Data corrupt (%" PRIuPTR " != %" PRIuPTR ")\n",
              (uintptr_t)zix_tree_get(ti),
              r);
      return test_fail();
    }
  }

  // Ensure tree size is correct
  if (zix_tree_size(t) != n_elems) {
    fprintf(stderr,
            "Tree size %" PRIuPTR " != %" PRIuPTR "\n",
            zix_tree_size(t),
            n_elems);
    return test_fail();
  }

  // Search for all elements
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (zix_tree_find(t, (void*)r, &ti)) {
      fprintf(stderr, "Find failed\n");
      return test_fail();
    }
    if ((uintptr_t)zix_tree_get(ti) != r) {
      fprintf(stderr,
              "Data corrupt (%" PRIuPTR " != %" PRIuPTR ")\n",
              (uintptr_t)zix_tree_get(ti),
              r);
      return test_fail();
    }
  }

  // Iterate over all elements
  size_t    i    = 0;
  uintptr_t last = 0;
  for (ZixTreeIter* iter = zix_tree_begin(t); !zix_tree_iter_is_end(iter);
       iter              = zix_tree_iter_next(iter), ++i) {
    const uintptr_t iter_data = (uintptr_t)zix_tree_get(iter);
    if (iter_data < last) {
      fprintf(stderr,
              "Iter corrupt (%" PRIuPTR " < %" PRIuPTR ")\n",
              iter_data,
              last);
      return test_fail();
    }
    last = iter_data;
  }
  if (i != n_elems) {
    fprintf(stderr,
            "Iteration stopped at %" PRIuPTR "/%" PRIuPTR " elements\n",
            i,
            n_elems);
    return test_fail();
  }

  // Iterate over all elements backwards
  i    = 0;
  last = INTPTR_MAX;
  for (ZixTreeIter* iter = zix_tree_rbegin(t); !zix_tree_iter_is_rend(iter);
       iter              = zix_tree_iter_prev(iter), ++i) {
    const uintptr_t iter_data = (uintptr_t)zix_tree_get(iter);
    if (iter_data > last) {
      fprintf(stderr,
              "Iter corrupt (%" PRIuPTR " < %" PRIuPTR ")\n",
              iter_data,
              last);
      return test_fail();
    }
    last = iter_data;
  }

  // Delete all elements
  for (size_t e = 0; e < n_elems; e++) {
    r = ith_elem(test_num, n_elems, e);

    ZixTreeIter* item = NULL;
    if (zix_tree_find(t, (void*)r, &item) != ZIX_STATUS_SUCCESS) {
      fprintf(stderr, "Failed to find item to remove\n");
      return test_fail();
    }
    if (zix_tree_remove(t, item)) {
      fprintf(stderr, "Error removing item\n");
    }
  }

  // Ensure the tree is empty
  if (zix_tree_size(t) != 0) {
    fprintf(stderr, "Tree size %" PRIuPTR " != 0\n", zix_tree_size(t));
    return test_fail();
  }

  // Insert n_elems elements again (to test non-empty destruction)
  for (size_t e = 0; e < n_elems; ++e) {
    r = ith_elem(test_num, n_elems, e);

    ZixStatus status = zix_tree_insert(t, (void*)r, &ti);
    if (status) {
      fprintf(stderr, "Insert failed\n");
      return test_fail();
    }

    if ((uintptr_t)zix_tree_get(ti) != r) {
      fprintf(stderr,
              "Data corrupt (%" PRIuPTR " != %" PRIuPTR ")\n",
              (uintptr_t)zix_tree_get(ti),
              r);
      return test_fail();
    }
  }

  // Ensure tree size is correct
  if (zix_tree_size(t) != n_elems) {
    fprintf(stderr,
            "Tree size %" PRIuPTR " != %" PRIuPTR "\n",
            zix_tree_size(t),
            n_elems);
    return test_fail();
  }

  zix_tree_free(t);

  return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
  const unsigned n_tests = 3;
  unsigned       n_elems = 0;

  if (argc == 1) {
    n_elems = 100000;
  } else {
    n_elems = (unsigned)strtoul(argv[1], NULL, 10);
    if (argc > 2) {
      seed = (unsigned)strtoul(argv[2], NULL, 10);
    } else {
      seed = (unsigned)time(NULL);
    }
  }

  if (n_elems == 0) {
    fprintf(stderr, "USAGE: %s [N_ELEMS]\n", argv[0]);
    return 1;
  }

  printf("Running %u tests with %u elements (seed %u)", n_tests, n_elems, seed);

  for (unsigned i = 0; i < n_tests; ++i) {
    printf(".");
    fflush(stdout);
    if (stress(i, n_elems)) {
      fprintf(stderr, "FAIL: Random seed %u\n", seed);
      return test_fail();
    }
  }
  printf("\n");
  return EXIT_SUCCESS;
}
