// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_FAILING_ALLOCATOR_H
#define ZIX_FAILING_ALLOCATOR_H

#include "zix/allocator.h"

#include <stddef.h>

/// An allocator that fails after some number of successes for testing
typedef struct {
  ZixAllocator base;          ///< Base allocator instance
  size_t       n_allocations; ///< Number of attempted allocations
  size_t       n_remaining;   ///< Number of remaining successful allocations
} ZixFailingAllocator;

ZixFailingAllocator
zix_failing_allocator(void);

#endif // ZIX_FAILING_ALLOCATOR_H
