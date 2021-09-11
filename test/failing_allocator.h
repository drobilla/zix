// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_FAILING_ALLOCATOR_H
#define ZIX_FAILING_ALLOCATOR_H

#include "zix/allocator.h"

#include <stddef.h>

/// An allocator that fails after some number of successes for testing
typedef struct {
  size_t n_allocations; ///< The number of allocations that have been attempted
  size_t n_remaining;   ///< The number of remaining successful allocations
} ZixFailingAllocatorState;

ZixAllocator
zix_failing_allocator(ZixFailingAllocatorState* state);

#endif // ZIX_FAILING_ALLOCATOR_H
