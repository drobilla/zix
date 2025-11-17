// Copyright 2011-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_DEFAULT_HASH_H
#define ZIX_DEFAULT_HASH_H

// Default backwards-compatibility hash function types

#include <zix/attributes.h>

#include <stddef.h>

typedef size_t(DefaultHashFunc)(const void* key);

#define ZIX_HASH_EXT_DATA_TYPE DefaultHashFunc* ZIX_NONNULL

#endif // ZIX_DFAULT_HASH_H
