// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "zix/thread.h"

#include "errno_status.h"

#include "zix/common.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <pthread.h>
#endif

#include <stddef.h>

#ifdef _WIN32

ZixStatus
zix_thread_create(ZixThread*    thread,
                  size_t        stack_size,
                  ZixThreadFunc function,
                  void*         arg)
{
  *thread = CreateThread(NULL, stack_size, function, arg, 0, NULL);
  return *thread ? ZIX_STATUS_SUCCESS : ZIX_STATUS_ERROR;
}

ZixStatus
zix_thread_join(ZixThread thread)
{
  return (WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0)
           ? ZIX_STATUS_SUCCESS
           : ZIX_STATUS_ERROR;
}

#else // !defined(_WIN32)

ZixStatus
zix_thread_create(ZixThread*    thread,
                  size_t        stack_size,
                  ZixThreadFunc function,
                  void*         arg)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, stack_size);

  const int ret = pthread_create(thread, NULL, function, arg);

  pthread_attr_destroy(&attr);
  return zix_errno_status(ret);
}

ZixStatus
zix_thread_join(ZixThread thread)
{
  return pthread_join(thread, NULL) ? ZIX_STATUS_ERROR : ZIX_STATUS_SUCCESS;
}

#endif
