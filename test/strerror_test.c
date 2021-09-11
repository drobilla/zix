/*
  Copyright 2021 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#undef NDEBUG

#include "zix/common.h"

#include <assert.h>
#include <string.h>

static void
test_strerror(void)
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
}

int
main(void)
{
  test_strerror();
  return 0;
}
