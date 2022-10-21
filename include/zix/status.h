// Copyright 2016-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_STATUS_H
#define ZIX_STATUS_H

#include "zix/attributes.h"

ZIX_BEGIN_DECLS

/**
   @defgroup zix Zix C API
   @{
   @defgroup zix_status Status Codes
   @{
*/

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

/**
   @}
   @}
*/

ZIX_END_DECLS

#endif /* ZIX_STATUS_H */
