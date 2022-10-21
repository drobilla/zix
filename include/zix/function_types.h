// Copyright 2016-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_FUNCTION_TYPES_H
#define ZIX_FUNCTION_TYPES_H

#include "zix/attributes.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   @defgroup zix_function_types Function Types
   @ingroup zix
   @{
*/

/// Function for comparing two elements
typedef int (*ZixCompareFunc)(const void* a,
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

#endif /* ZIX_FUNCTION_TYPES_H */
