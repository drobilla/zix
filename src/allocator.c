// Copyright 2011-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "zix/allocator.h"

#include <stdlib.h>

ZIX_MALLOC_FUNC
static void*
zix_default_malloc(ZixAllocator* const allocator, const size_t size)
{
  (void)allocator;
  return malloc(size);
}

ZIX_MALLOC_FUNC
static void*
zix_default_calloc(ZixAllocator* const allocator,
                   const size_t        nmemb,
                   const size_t        size)
{
  (void)allocator;
  return calloc(nmemb, size);
}

static void*
zix_default_realloc(ZixAllocator* const allocator,
                    void* const         ptr,
                    const size_t        size)
{
  (void)allocator;
  return realloc(ptr, size);
}

static void
zix_default_free(ZixAllocator* const allocator, void* const ptr)
{
  (void)allocator;
  free(ptr);
}

ZixAllocator*
zix_default_allocator(void)
{
  static ZixAllocator default_allocator = {
    zix_default_malloc,
    zix_default_calloc,
    zix_default_realloc,
    zix_default_free,
  };

  return &default_allocator;
}
