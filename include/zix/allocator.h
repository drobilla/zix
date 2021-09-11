// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_ALLOCATOR_H
#define ZIX_ALLOCATOR_H

#include "zix/attributes.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   @addtogroup zix
   @{
   @name Allocator
   @{
*/

/// Opaque user data for allocation functions
typedef void ZixAllocatorHandle;

/**
   General malloc-like memory allocation function.

   This works like the standard C malloc(), except has an additional handle
   parameter for implementing stateful allocators without static data.
*/
typedef void* ZIX_ALLOCATED (
  *ZixMallocFunc)(ZixAllocatorHandle* ZIX_NULLABLE handle, size_t size);

/**
   General calloc-like memory allocation function.

   This works like the standard C calloc(), except has an additional handle
   parameter for implementing stateful allocators without static data.
*/
typedef void* ZIX_ALLOCATED (*ZixCallocFunc)(ZixAllocatorHandle* ZIX_NULLABLE
                                                    handle,
                                             size_t nmemb,
                                             size_t size);

/**
   General realloc-like memory reallocation function.

   This works like the standard C remalloc(), except has an additional handle
   parameter for implementing stateful allocators without static data.
*/
typedef void* ZIX_ALLOCATED (*ZixReallocFunc)(ZixAllocatorHandle* ZIX_NULLABLE
                                                                 handle,
                                              void* ZIX_NULLABLE ptr,
                                              size_t             size);

/**
   General free-like memory deallocation function.

   This works like the standard C remalloc(), except has an additional handle
   parameter for implementing stateful allocators without static data.
*/
typedef void (*ZixFreeFunc)(ZixAllocatorHandle* ZIX_NULLABLE handle,
                            void* ZIX_NULLABLE               ptr);

/**
   A memory allocator.

   This object-like structure provides an interface like the standard C
   functions malloc(), calloc(), realloc(), and free().  It allows the user to
   pass a custom allocator to be used by data structures.
*/
typedef struct {
  ZixAllocatorHandle* ZIX_NULLABLE handle;
  ZixMallocFunc ZIX_NONNULL        malloc;
  ZixCallocFunc ZIX_NONNULL        calloc;
  ZixReallocFunc ZIX_NONNULL       realloc;
  ZixFreeFunc ZIX_NONNULL          free;
} ZixAllocator;

/// Return the default allocator which simply uses the system allocator
ZIX_CONST_API
const ZixAllocator* ZIX_NONNULL
zix_default_allocator(void);

/// Convenience wrapper that defers to malloc() if allocator is null
static inline void* ZIX_ALLOCATED
zix_malloc(const ZixAllocator* const ZIX_NULLABLE allocator, const size_t size)
{
  const ZixAllocator* const actual =
    allocator ? allocator : zix_default_allocator();

  return actual->malloc(actual->handle, size);
}

/// Convenience wrapper that defers to calloc() if allocator is null
static inline void* ZIX_ALLOCATED
zix_calloc(const ZixAllocator* const ZIX_NULLABLE allocator,
           const size_t                           nmemb,
           const size_t                           size)
{
  const ZixAllocator* const actual =
    allocator ? allocator : zix_default_allocator();

  return actual->calloc(actual->handle, nmemb, size);
}

/// Convenience wrapper that defers to realloc() if allocator is null
static inline void* ZIX_ALLOCATED
zix_realloc(const ZixAllocator* const ZIX_NULLABLE allocator,
            void* const ZIX_NULLABLE               ptr,
            const size_t                           size)
{
  const ZixAllocator* const actual =
    allocator ? allocator : zix_default_allocator();

  return actual->realloc(actual->handle, ptr, size);
}

/// Convenience wrapper that defers to free() if allocator is null
static inline void
zix_free(const ZixAllocator* const ZIX_NULLABLE allocator,
         void* const ZIX_NULLABLE               ptr)
{
  const ZixAllocator* const actual =
    allocator ? allocator : zix_default_allocator();

  actual->free(actual->handle, ptr);
}

/**
   @}
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIX_ALLOCATOR_H */
