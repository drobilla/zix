// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "failing_allocator.h"

#include "zix/allocator.h"
#include "zix/attributes.h"

#include <stdbool.h>
#include <stddef.h>

static bool
attempt(ZixFailingAllocatorState* const state)
{
  ++state->n_allocations;

  if (!state->n_remaining) {
    return false;
  }

  --state->n_remaining;
  return true;
}

ZIX_MALLOC_FUNC
static void*
zix_failing_malloc(ZixAllocatorHandle* const handle, const size_t size)
{
  ZixFailingAllocatorState* const state = (ZixFailingAllocatorState*)handle;
  const ZixAllocator* const       base  = zix_default_allocator();

  return attempt(state) ? base->malloc(base->handle, size) : NULL;
}

ZIX_MALLOC_FUNC
static void*
zix_failing_calloc(ZixAllocatorHandle* const handle,
                   const size_t              nmemb,
                   const size_t              size)
{
  ZixFailingAllocatorState* const state = (ZixFailingAllocatorState*)handle;
  const ZixAllocator* const       base  = zix_default_allocator();

  return attempt(state) ? base->calloc(base->handle, nmemb, size) : NULL;
}

static void*
zix_failing_realloc(ZixAllocatorHandle* const handle,
                    void* const               ptr,
                    const size_t              size)
{
  ZixFailingAllocatorState* const state = (ZixFailingAllocatorState*)handle;
  const ZixAllocator* const       base  = zix_default_allocator();

  return attempt(state) ? base->realloc(base->handle, ptr, size) : NULL;
}

static void
zix_failing_free(ZixAllocatorHandle* const handle, void* const ptr)
{
  (void)handle;

  const ZixAllocator* const base = zix_default_allocator();

  base->free(base->handle, ptr);
}

ZIX_CONST_FUNC
ZixAllocator
zix_failing_allocator(ZixFailingAllocatorState* const state)
{
  const ZixAllocator failing_allocator = {
    state,
    zix_failing_malloc,
    zix_failing_calloc,
    zix_failing_realloc,
    zix_failing_free,
  };

  return failing_allocator;
}
