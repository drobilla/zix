// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "zix/attributes.h"
#include "zix/common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int
main(void)
{
  const char* msg = zix_strerror(ZIX_STATUS_SUCCESS);
  assert(!strcmp(msg, "Success"));

  for (int i = ZIX_STATUS_ERROR; i <= ZIX_STATUS_REACHED_END; ++i) {
    msg = zix_strerror((ZixStatus)i);
    assert(strcmp(msg, "Success"));
  }

  msg = zix_strerror((ZixStatus)-1);
  assert(!strcmp(msg, "Unknown error"));

  msg = zix_strerror((ZixStatus)1000000);
  assert(!strcmp(msg, "Unknown error"));

  printf("Success\n");
  return 0;
}
