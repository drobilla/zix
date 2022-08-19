// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "zix/common.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void
test_errno_status(void)
{
  assert(zix_errno_status(0) == ZIX_STATUS_SUCCESS);
  assert(zix_errno_status(INT_MAX) == ZIX_STATUS_ERROR);

#ifdef EEXIST
  assert(zix_errno_status(EEXIST) == ZIX_STATUS_EXISTS);
#endif
#ifdef EINVAL
  assert(zix_errno_status(EINVAL) == ZIX_STATUS_BAD_ARG);
#endif
#ifdef EPERM
  assert(zix_errno_status(EPERM) == ZIX_STATUS_BAD_PERMS);
#endif
#ifdef ETIMEDOUT
  assert(zix_errno_status(ETIMEDOUT) == ZIX_STATUS_TIMEOUT);
#endif
}

static void
test_strerror(void)
{
  const char* msg = zix_strerror(ZIX_STATUS_SUCCESS);
  assert(!strcmp(msg, "Success"));

  for (int i = ZIX_STATUS_ERROR; i <= ZIX_STATUS_NOT_SUPPORTED; ++i) {
    msg = zix_strerror((ZixStatus)i);
    assert(strcmp(msg, "Success"));
  }

  msg = zix_strerror((ZixStatus)-1);
  assert(!strcmp(msg, "Unknown error"));

  msg = zix_strerror((ZixStatus)1000000);
  assert(!strcmp(msg, "Unknown error"));

  printf("Success\n");
}

int
main(void)
{
  test_errno_status();
  test_strerror();
  return 0;
}
