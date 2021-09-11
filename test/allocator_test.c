/*
  Copyright 2014-2021 David Robillard <d@drobilla.net>

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

#include "zix/allocator.h"

#include <assert.h>

static void
test_allocator(void)
{
  // Just a basic smoke test to check that things seem to be working

  const ZixAllocator* const allocator = zix_default_allocator();

  char* const malloced = (char*)zix_malloc(allocator, 4);
  malloced[0]          = 0;
  malloced[3]          = 3;
  assert(malloced[0] == 0);
  assert(malloced[3] == 3);

  char* const calloced = (char*)zix_calloc(allocator, 4, 1);
  assert(calloced[0] == 0);
  assert(calloced[1] == 0);
  assert(calloced[2] == 0);
  assert(calloced[3] == 0);

  char* const realloced = (char*)zix_realloc(allocator, calloced, 8);
  assert(realloced[0] == 0);
  assert(realloced[1] == 0);
  assert(realloced[2] == 0);
  assert(realloced[3] == 0);
  realloced[4] = 4;
  realloced[5] = 5;
  realloced[6] = 6;
  realloced[7] = 7;
  assert(realloced[4] == 4);
  assert(realloced[5] == 5);
  assert(realloced[6] == 6);
  assert(realloced[7] == 7);

  zix_free(allocator, realloced);
  zix_free(allocator, malloced);
}

int
main(void)
{
  test_allocator();
  return 0;
}
