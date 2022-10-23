// Copyright 2007-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "../system.h"

#include <windows.h>

#include <limits.h>
#include <stdint.h>

uint32_t
zix_system_page_size(void)
{
  SYSTEM_INFO info;
  GetSystemInfo(&info);

  return (info.dwPageSize > 0 && info.dwPageSize < UINT32_MAX)
           ? (uint32_t)info.dwPageSize
           : 512U;
}
