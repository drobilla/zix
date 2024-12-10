// Copyright 2012-2024 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <zix/environment.h>

#include <windows.h>

char*
zix_expand_environment_strings(ZixAllocator* const allocator,
                               const char* const   string)
{
  const DWORD size = ExpandEnvironmentStrings(string, NULL, 0U);
  if (!size) {
    return NULL;
  }

  char* const out = (char*)zix_calloc(allocator, (size_t)size + 1U, 1U);
  if (out) {
    ExpandEnvironmentStrings(string, out, size + 1U);
  }
  return out;
}
