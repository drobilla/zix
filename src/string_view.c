// Copyright 2007-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "zix/string_view.h"
#include "zix/allocator.h"

#include <string.h>

char*
zix_string_view_copy(ZixAllocator* const allocator, const ZixStringView view)
{
  char* const copy = (char*)zix_malloc(allocator, view.length + 1U);
  if (copy) {
    memcpy(copy, view.data, view.length);
    copy[view.length] = '\0';
  }
  return copy;
}
