// Copyright 2007-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "../system.h"
#include "../zix_config.h"

#include <unistd.h>

#include <stdint.h>

#if defined(PAGE_SIZE)
#  define ZIX_DEFAULT_PAGE_SIZE PAGE_SIZE
#else
#  define ZIX_DEFAULT_PAGE_SIZE 4096U
#endif

uint32_t
zix_system_page_size(void)
{
#if USE_SYSCONF
  const long r = sysconf(_SC_PAGE_SIZE);
  return r > 0L ? (uint32_t)r : ZIX_DEFAULT_PAGE_SIZE;
#else
  return (uint32_t)ZIX_DEFAULT_PAGE_SIZE;
#endif
}
