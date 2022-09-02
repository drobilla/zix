// Copyright 2016-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_COMMON_H
#define ZIX_COMMON_H

#include "zix/attributes.h"

#include <stdbool.h>

/**
   @addtogroup zix
   @{
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ZIX_STATUS_SUCCESS,
  ZIX_STATUS_ERROR,
  ZIX_STATUS_NO_MEM,
  ZIX_STATUS_NOT_FOUND,
  ZIX_STATUS_EXISTS,
  ZIX_STATUS_BAD_ARG,
  ZIX_STATUS_BAD_PERMS,
  ZIX_STATUS_REACHED_END,
  ZIX_STATUS_TIMEOUT,
  ZIX_STATUS_OVERFLOW,
  ZIX_STATUS_NOT_SUPPORTED,
  ZIX_STATUS_UNAVAILABLE,
} ZixStatus;

/// Return a string describing a status code
ZIX_CONST_API
const char*
zix_strerror(ZixStatus status);

/// Return an errno value converted to a status code
ZIX_CONST_API
ZixStatus
zix_errno_status(int e);

/// Return success if `r` is non-zero, or `errno` as a status code otherwise
ZIX_PURE_API
ZixStatus
zix_errno_status_if(int r);

/// Function for comparing two elements
typedef int (*ZixComparator)(const void* a,
                             const void* b,
                             const void* user_data);

/// Function for testing equality of two elements
typedef bool (*ZixEqualFunc)(const void* a, const void* b);

/// Function to destroy an element
typedef void (*ZixDestroyFunc)(void* ptr, const void* user_data);

/**
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIX_COMMON_H */
