// Copyright 2014-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "errno_status.h"

#include "zix/common.h"

#include <errno.h>
#include <stddef.h>

ZixStatus
zix_errno_status_if(const int r)
{
  return r ? zix_errno_status(errno) : ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_errno_status(const int e)
{
  typedef struct {
    int       code;
    ZixStatus status;
  } Mapping;

  static const Mapping map[] = {
    {0, ZIX_STATUS_SUCCESS},
    {EACCES, ZIX_STATUS_BAD_PERMS},
    {EAGAIN, ZIX_STATUS_UNAVAILABLE},
    {EEXIST, ZIX_STATUS_EXISTS},
    {EINVAL, ZIX_STATUS_BAD_ARG},
    {ENOENT, ZIX_STATUS_NOT_FOUND},
    {ENOMEM, ZIX_STATUS_NO_MEM},
    {ENOSYS, ZIX_STATUS_NOT_SUPPORTED},
    {EPERM, ZIX_STATUS_BAD_PERMS},
    {ETIMEDOUT, ZIX_STATUS_TIMEOUT},
    {0, ZIX_STATUS_ERROR}, // Fallback mapping
  };

  static const size_t n_mappings = sizeof(map) / sizeof(Mapping);

  // Find the index of the matching mapping (or leave it at the fallback entry)
  size_t m = 0;
  while (m < n_mappings && map[m].code != e) {
    ++m;
  }

  return map[m].status;
}
