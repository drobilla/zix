// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_THREAD_H
#define ZIX_THREAD_H

#include "zix/common.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <pthread.h>
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   @addtogroup zix
   @{
   @name Thread
   @{
*/

#ifdef _WIN32
#  define ZIX_THREAD_RESULT 0
#  define ZIX_THREAD_FUNC __stdcall

typedef HANDLE ZixThread;
typedef DWORD  ZixThreadResult;

#else
#  define ZIX_THREAD_RESULT NULL
#  define ZIX_THREAD_FUNC

typedef pthread_t ZixThread;
typedef void*     ZixThreadResult;
#endif

/**
   A thread function.

   For portability reasons, the return type varies between platforms, and the
   return value is ignored.  Thread functions should always return
   #ZIX_THREAD_RESULT.  This allows thread functions to be used directly by the
   system without any wrapper overhead.

   "Returning" a result, and communicating with the parent thread in general,
   can be done through the pointer argument.
*/
typedef ZixThreadResult(ZIX_THREAD_FUNC* ZixThreadFunc)(void*);

/**
   Initialize `thread` to a new thread.

   The thread will immediately be launched, calling `function` with `arg`
   as the only parameter.
*/
static inline ZixStatus
zix_thread_create(ZixThread*    thread,
                  size_t        stack_size,
                  ZixThreadFunc function,
                  void*         arg);

/// Join `thread` (block until `thread` exits)
static inline ZixStatus
zix_thread_join(ZixThread thread);

#ifdef _WIN32

static inline ZixStatus
zix_thread_create(ZixThread*    thread,
                  size_t        stack_size,
                  ZixThreadFunc function,
                  void*         arg)
{
  *thread = CreateThread(NULL, stack_size, function, arg, 0, NULL);
  return *thread ? ZIX_STATUS_SUCCESS : ZIX_STATUS_ERROR;
}

static inline ZixStatus
zix_thread_join(ZixThread thread)
{
  return (WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0)
           ? ZIX_STATUS_SUCCESS
           : ZIX_STATUS_ERROR;
}

#else /* !defined(_WIN32) */

static inline ZixStatus
zix_thread_create(ZixThread* thread,
                  size_t     stack_size,
                  void* (*function)(void*),
                  void* arg)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, stack_size);

  const int ret = pthread_create(thread, NULL, function, arg);
  pthread_attr_destroy(&attr);

  return zix_errno_status(ret);
}

static inline ZixStatus
zix_thread_join(ZixThread thread)
{
  return pthread_join(thread, NULL) ? ZIX_STATUS_ERROR : ZIX_STATUS_SUCCESS;
}

#endif

/**
   @}
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIX_THREAD_H */
