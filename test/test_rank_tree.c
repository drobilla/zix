// Copyright 2011-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "failing_allocator.h"
#include "test_args.h"

#include <zix/rank_tree.h>
#include <zix/status.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void
test_new_failed_alloc(void)
{
  ZixFailingAllocator allocator = zix_failing_allocator();

  // Successfully allocate a itree to count the number of allocations
  ZixRankTree* const tree = zix_rank_tree_new(&allocator.base);
  assert(tree);
  assert(!zix_rank_tree_size(tree));
  assert(!zix_rank_tree_height(tree));
  zix_rank_tree_free(tree, NULL, NULL);

  // Test that each allocation failing is handled gracefully
  const size_t n_new_allocs = zix_failing_allocator_reset(&allocator, 0);
  for (size_t i = 0U; i < n_new_allocs; ++i) {
    zix_failing_allocator_reset(&allocator, i);
    assert(!zix_rank_tree_new(&allocator.base));
  }
}

static void
test_push_failed_alloc(void)
{
  static const size_t n_elems = 1024U;

  ZixFailingAllocator allocator = zix_failing_allocator();

  ZixRankTree* const tree = zix_rank_tree_new(&allocator.base);
  assert(tree);

  // Successfully push elements to count the number of allocations
  for (uintptr_t i = 0U; i < n_elems; ++i) {
    assert(!zix_rank_tree_push(tree, i));
  }

  zix_rank_tree_clear(tree, NULL, NULL);

  // Test that each allocation failing is handled gracefully
  const size_t n_new_allocs = zix_failing_allocator_reset(&allocator, 0);
  for (size_t i = 0U; i < n_new_allocs; ++i) {
    zix_failing_allocator_reset(&allocator, i);

    for (uintptr_t j = 0U; j < n_elems; ++j) {
      const ZixStatus st = zix_rank_tree_push(tree, j);
      if (st) {
        assert(st == ZIX_STATUS_NO_MEM);
        break;
      }
    }
  }

  zix_rank_tree_free(tree, NULL, NULL);
}

static void
test_push_pop(const size_t n_elems)
{
  // Create a new tree and check that no elements are accessible
  ZixRankTree* const tree = zix_rank_tree_new(NULL);
  assert(tree);
  assert(!zix_rank_tree_at(tree, 0U));
  assert(zix_rank_tree_pop(tree) == ZIX_STATUS_NOT_FOUND);

  // Push elements and check that they're accessible as we go
  for (uintptr_t i = 0U; i < n_elems; ++i) {
    assert(!zix_rank_tree_push(tree, i));
    assert(zix_rank_tree_size(tree) == i + 1);
    assert(zix_rank_tree_at(tree, i) == i);
  }

  // Check that all those elements are present in the final tree
  for (uintptr_t i = 0U; i < n_elems; ++i) {
    assert(zix_rank_tree_at(tree, i) == i);
  }

  // Check that the tree has the expected size
  assert(zix_rank_tree_size(tree) == n_elems);
  assert(!zix_rank_tree_at(tree, n_elems + 1U));

  // Pop elements
  for (uintptr_t i = 0U; i < n_elems; ++i) {
    const size_t rank = n_elems - i;
    assert(!zix_rank_tree_pop(tree));
    assert(zix_rank_tree_size(tree) == rank - 1U);
    assert(!zix_rank_tree_at(tree, rank));
  }

  // Check that the tree is empty again
  assert(!zix_rank_tree_size(tree));
  assert(!zix_rank_tree_height(tree));
  assert(!zix_rank_tree_at(tree, 0U));

  zix_rank_tree_free(tree, NULL, NULL);
}

static void
destroy(const uintptr_t val, void* const user_data)
{
  size_t* const count = (size_t*)user_data;
  assert(val == *count);
  ++*count;
}

static void
test_clear(const size_t n_elems)
{
  ZixRankTree* const tree = zix_rank_tree_new(NULL);
  assert(tree);

  // Push elements
  for (uintptr_t i = 0U; i < n_elems; ++i) {
    assert(!zix_rank_tree_push(tree, i));
  }

  // Clear tree and record number of destructions
  size_t destroy_count = 0U;
  zix_rank_tree_clear(tree, destroy, &destroy_count);

  // Check that the tree is empty
  assert(!zix_rank_tree_size(tree));
  assert(!zix_rank_tree_height(tree));
  assert(destroy_count == n_elems);

  zix_rank_tree_free(tree, NULL, NULL);
}

static void
test_free(void)
{
  ZixRankTree* const tree = zix_rank_tree_new(NULL);
  assert(tree);

  // Test that the destroy function isn't called for an empty tree
  size_t destroy_count = 0U;
  zix_rank_tree_free(tree, destroy, &destroy_count);
  assert(!destroy_count);

  // Test that free handles null gracefully
  zix_rank_tree_free(NULL, NULL, NULL);
  zix_rank_tree_free(NULL, destroy, &destroy_count);
  zix_rank_tree_free(NULL, destroy, NULL);
  zix_rank_tree_free(NULL, NULL, &destroy_count);
  assert(!destroy_count);
}

int
main(int argc, char** argv)
{
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [N_ELEMS]\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char* const size_arg = (argc > 1) ? argv[1] : "262145";
  const size_t      n_elems  = zix_test_size_arg(size_arg, 1U, 1U << 30U);

  printf("Running tests with %zu elements\n", n_elems);

  test_new_failed_alloc();
  test_free();
  test_push_failed_alloc();
  test_push_pop(n_elems);
  test_clear(n_elems);
  return 0;
}
