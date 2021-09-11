// Copyright 2014-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef __APPLE__
#  define _GNU_SOURCE
#endif

#include "test_malloc.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void* (*test_malloc_sys_malloc)(size_t size) = NULL;

static size_t        test_malloc_n_allocs   = 0;
static size_t        test_malloc_fail_after = (size_t)-1;
static volatile bool in_test_malloc_init    = false;

static void*
test_malloc(size_t size)
{
  if (in_test_malloc_init) {
    return NULL; // dlsym is asking for memory, but handles this fine
  }

  if (!test_malloc_sys_malloc) {
    test_malloc_init();
  }

  if (test_malloc_n_allocs < test_malloc_fail_after) {
    ++test_malloc_n_allocs;
    return test_malloc_sys_malloc(size);
  }

  return NULL;
}

void*
malloc(size_t size)
{
  return test_malloc(size);
}

void*
calloc(size_t nmemb, size_t size)
{
  void* ptr = test_malloc(nmemb * size);
  if (ptr) {
    memset(ptr, 0, nmemb * size);
  }
  return ptr;
}

void
test_malloc_reset(size_t fail_after)
{
  test_malloc_fail_after = fail_after;
  test_malloc_n_allocs   = 0;
}

void
test_malloc_init(void)
{
  in_test_malloc_init = true;

  /* Avoid pedantic warnings about casting pointer to function pointer by
     casting dlsym instead. */

  typedef void* (*MallocFunc)(size_t);
  typedef MallocFunc (*MallocFuncGetter)(void*, const char*);

  MallocFuncGetter dlfunc = (MallocFuncGetter)dlsym;
  test_malloc_sys_malloc  = (MallocFunc)dlfunc(RTLD_NEXT, "malloc");

  in_test_malloc_init = false;
}

size_t
test_malloc_get_n_allocs(void)
{
  return test_malloc_n_allocs;
}
