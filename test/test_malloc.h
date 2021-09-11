// Copyright 2014-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "zix/attributes.h"

#include <stddef.h>

void
test_malloc_init(void);

void
test_malloc_reset(size_t fail_after);

ZIX_PURE_API
size_t
test_malloc_get_n_allocs(void);
