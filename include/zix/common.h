// Copyright 2016-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_COMMON_H
#define ZIX_COMMON_H

#include "zix/attributes.h"

#include <stdbool.h>

/**
   @defgroup zix Zix C API
   @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/// A status code returned by functions
typedef enum {
  ZIX_STATUS_SUCCESS,       ///< Success
  ZIX_STATUS_ERROR,         ///< Unknown error
  ZIX_STATUS_NO_MEM,        ///< Out of memory
  ZIX_STATUS_NOT_FOUND,     ///< Not found
  ZIX_STATUS_EXISTS,        ///< Exists
  ZIX_STATUS_BAD_ARG,       ///< Bad argument
  ZIX_STATUS_BAD_PERMS,     ///< Bad permissions
  ZIX_STATUS_REACHED_END,   ///< Reached end
  ZIX_STATUS_TIMEOUT,       ///< Timeout
  ZIX_STATUS_OVERFLOW,      ///< Overflow
  ZIX_STATUS_NOT_SUPPORTED, ///< Not supported
  ZIX_STATUS_UNAVAILABLE,   ///< Resource unavailable
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

/// Function to destroy an element
typedef void (*ZixDestroyFunc)(void* ptr, const void* user_data);

/**
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIX_COMMON_H */
