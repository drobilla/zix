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
