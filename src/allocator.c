// Copyright 2011-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "zix/allocator.h"

#include <stdlib.h>

ZIX_MALLOC_FUNC
static void*
zix_default_malloc(ZixAllocatorHandle* const handle, const size_t size)
{
  (void)handle;
  return malloc(size);
}

ZIX_MALLOC_FUNC
static void*
zix_default_calloc(ZixAllocatorHandle* const handle,
                   const size_t              nmemb,
                   const size_t              size)
{
  (void)handle;
  return calloc(nmemb, size);
}

static void*
zix_default_realloc(ZixAllocatorHandle* const handle,
                    void* const               ptr,
                    const size_t              size)
{
  (void)handle;
  return realloc(ptr, size);
}

static void
zix_default_free(ZixAllocatorHandle* const handle, void* const ptr)
{
  (void)handle;
  free(ptr);
}

const ZixAllocator*
zix_default_allocator(void)
{
  static const ZixAllocator default_allocator = {
    NULL,
    zix_default_malloc,
    zix_default_calloc,
    zix_default_realloc,
    zix_default_free,
  };

  return &default_allocator;
}
