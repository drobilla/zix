// Copyright 2014-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "zix/common.h"

#include <errno.h>

const char*
zix_strerror(const ZixStatus status)
{
  switch (status) {
  case ZIX_STATUS_SUCCESS:
    return "Success";
  case ZIX_STATUS_ERROR:
    return "Unknown error";
  case ZIX_STATUS_NO_MEM:
    return "Out of memory";
  case ZIX_STATUS_NOT_FOUND:
    return "Not found";
  case ZIX_STATUS_EXISTS:
    return "Exists";
  case ZIX_STATUS_BAD_ARG:
    return "Bad argument";
  case ZIX_STATUS_BAD_PERMS:
    return "Bad permissions";
  case ZIX_STATUS_REACHED_END:
    return "Reached end";
  case ZIX_STATUS_TIMEOUT:
    return "Timeout";
  case ZIX_STATUS_OVERFLOW:
    return "Overflow";
  case ZIX_STATUS_NOT_SUPPORTED:
    return "Not supported";
  case ZIX_STATUS_UNAVAILABLE:
    return "Resource unavailable";
  }
  return "Unknown error";
}

ZixStatus
zix_errno_status_if(const int r)
{
  return r ? zix_errno_status(errno) : ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_errno_status(const int e)
{
  switch (e) {
  case 0:
    return ZIX_STATUS_SUCCESS;
#ifdef EAGAIN
  case EAGAIN:
    return ZIX_STATUS_UNAVAILABLE;
#endif
#ifdef EEXIST
  case EEXIST:
    return ZIX_STATUS_EXISTS;
#endif
#ifdef EINVAL
  case EINVAL:
    return ZIX_STATUS_BAD_ARG;
#endif
#ifdef EPERM
  case EPERM:
    return ZIX_STATUS_BAD_PERMS;
#endif
#ifdef ETIMEDOUT
  case ETIMEDOUT:
    return ZIX_STATUS_TIMEOUT;
#endif
  default:
    break;
  }

  return ZIX_STATUS_ERROR;
}
