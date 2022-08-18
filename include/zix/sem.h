// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_SEM_H
#define ZIX_SEM_H

#include "zix/attributes.h"
#include "zix/common.h"

#ifdef __APPLE__
#  include <mach/mach.h>
#elif defined(_WIN32)
#  include <limits.h>
#  include <windows.h>
#else
#  include <errno.h>
#  include <semaphore.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
   @addtogroup zix
   @{
   @name Semaphore
   @{
*/

struct ZixSemImpl;

/**
   A counting semaphore.

   This is an integer that is always positive, and has two main operations:
   increment (post) and decrement (wait).  If a decrement can not be performed
   (i.e. the value is 0) the caller will be blocked until another thread posts
   and the operation can succeed.

   Semaphores can be created with any starting value, but typically this will
   be 0 so the semaphore can be used as a simple signal where each post
   corresponds to one wait.

   Semaphores are very efficient (much moreso than a mutex/cond pair).  In
   particular, at least on Linux, post is async-signal-safe, which means it
   does not block and will not be interrupted.  If you need to signal from
   a realtime thread, this is the most appropriate primitive to use.
*/
typedef struct ZixSemImpl ZixSem;

/**
   Create `sem` with the given `initial` value.

   @return #ZIX_STATUS_SUCCESS, or an unlikely error.
*/
static inline ZixStatus
zix_sem_init(ZixSem* ZIX_NONNULL sem, unsigned initial);

/**
   Destroy `sem`.

   @return #ZIX_STATUS_SUCCESS, or an error.
*/
static inline ZixStatus
zix_sem_destroy(ZixSem* ZIX_NONNULL sem);

/**
   Increment and signal any waiters.

   Realtime safe.

   @return #ZIX_STATUS_SUCCESS if `sem` was incremented, #ZIX_STATUS_OVERFLOW
   if the maximum possible value would have been exceeded, or
   #ZIX_STATUS_BAD_ARG if `sem` is invalid.
*/
static inline ZixStatus
zix_sem_post(ZixSem* ZIX_NONNULL sem);

/**
   Wait until count is > 0, then decrement.

   Obviously not realtime safe.

   @return #ZIX_STATUS_SUCCESS if `sem` was decremented, or #ZIX_STATUS_BAD_ARG
   if `sem` is invalid.
*/
static inline ZixStatus
zix_sem_wait(ZixSem* ZIX_NONNULL sem);

/**
   Non-blocking version of wait().

   @return #ZIX_STATUS_SUCCESS if `sem` was decremented, #ZIX_STATUS_TIMEOUT if
   it was already zero, or #ZIX_STATUS_BAD_ARG if `sem` is invalid.
*/
static inline ZixStatus
zix_sem_try_wait(ZixSem* ZIX_NONNULL sem);

/**
   @cond
*/

#ifdef __APPLE__

struct ZixSemImpl {
  semaphore_t sem;
};

static inline ZixStatus
zix_sem_init(ZixSem* ZIX_NONNULL sem, unsigned val)
{
  return semaphore_create(
           mach_task_self(), &sem->sem, SYNC_POLICY_FIFO, (int)val)
           ? ZIX_STATUS_ERROR
           : ZIX_STATUS_SUCCESS;
}

static inline ZixStatus
zix_sem_destroy(ZixSem* ZIX_NONNULL sem)
{
  return semaphore_destroy(mach_task_self(), sem->sem) ? ZIX_STATUS_ERROR
                                                       : ZIX_STATUS_SUCCESS;
}

static inline ZixStatus
zix_sem_post(ZixSem* ZIX_NONNULL sem)
{
  return semaphore_signal(sem->sem) ? ZIX_STATUS_ERROR : ZIX_STATUS_SUCCESS;
}

static inline ZixStatus
zix_sem_wait(ZixSem* ZIX_NONNULL sem)
{
  kern_return_t r = 0;
  while ((r = semaphore_wait(sem->sem)) && r == KERN_ABORTED) {
    // Interrupted, try again
  }

  return r ? ZIX_STATUS_ERROR : ZIX_STATUS_SUCCESS;
}

static inline ZixStatus
zix_sem_try_wait(ZixSem* ZIX_NONNULL sem)
{
  const mach_timespec_t zero = {0, 0};
  const kern_return_t   r    = semaphore_timedwait(sem->sem, zero);

  return (r == KERN_SUCCESS)               ? ZIX_STATUS_SUCCESS
         : (r == KERN_OPERATION_TIMED_OUT) ? ZIX_STATUS_TIMEOUT
                                           : ZIX_STATUS_ERROR;
}

#elif defined(_WIN32)

struct ZixSemImpl {
  HANDLE sem;
};

static inline ZixStatus
zix_sem_init(ZixSem* ZIX_NONNULL sem, unsigned initial)
{
  sem->sem = CreateSemaphore(NULL, (LONG)initial, LONG_MAX, NULL);
  return sem->sem ? ZIX_STATUS_SUCCESS : ZIX_STATUS_ERROR;
}

static inline ZixStatus
zix_sem_destroy(ZixSem* ZIX_NONNULL sem)
{
  return CloseHandle(sem->sem) ? ZIX_STATUS_SUCCESS : ZIX_STATUS_ERROR;
}

static inline ZixStatus
zix_sem_post(ZixSem* ZIX_NONNULL sem)
{
  return ReleaseSemaphore(sem->sem, 1, NULL) ? ZIX_STATUS_SUCCESS
                                             : ZIX_STATUS_ERROR;
}

static inline ZixStatus
zix_sem_wait(ZixSem* ZIX_NONNULL sem)
{
  return WaitForSingleObject(sem->sem, INFINITE) == WAIT_OBJECT_0
           ? ZIX_STATUS_SUCCESS
           : ZIX_STATUS_ERROR;
}

static inline ZixStatus
zix_sem_try_wait(ZixSem* ZIX_NONNULL sem)
{
  const DWORD r = WaitForSingleObject(sem->sem, 0);

  return (r == WAIT_OBJECT_0)  ? ZIX_STATUS_SUCCESS
         : (r == WAIT_TIMEOUT) ? ZIX_STATUS_TIMEOUT
                               : ZIX_STATUS_ERROR;
}

#else /* !defined(__APPLE__) && !defined(_WIN32) */

struct ZixSemImpl {
  sem_t sem;
};

static inline ZixStatus
zix_sem_init(ZixSem* ZIX_NONNULL sem, unsigned initial)
{
  return sem_init(&sem->sem, 0, initial) ? zix_errno_status(errno)
                                         : ZIX_STATUS_SUCCESS;
}

static inline ZixStatus
zix_sem_destroy(ZixSem* ZIX_NONNULL sem)
{
  return sem_destroy(&sem->sem) ? zix_errno_status(errno) : ZIX_STATUS_SUCCESS;
}

static inline ZixStatus
zix_sem_post(ZixSem* ZIX_NONNULL sem)
{
  return sem_post(&sem->sem) ? zix_errno_status(errno) : ZIX_STATUS_SUCCESS;
}

static inline ZixStatus
zix_sem_wait(ZixSem* ZIX_NONNULL sem)
{
  int r = 0;
  while ((r = sem_wait(&sem->sem)) && errno == EINTR) {
    // Interrupted, try again
  }

  return r ? zix_errno_status(errno) : ZIX_STATUS_SUCCESS;
}

static inline ZixStatus
zix_sem_try_wait(ZixSem* ZIX_NONNULL sem)
{
  int r = 0;
  while ((r = sem_trywait(&sem->sem)) && errno == EINTR) {
    // Interrupted, try again
  }

  return r ? (errno == EAGAIN ? ZIX_STATUS_TIMEOUT : zix_errno_status(errno))
           : ZIX_STATUS_SUCCESS;
}

#endif

/**
   @endcond
   @}
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIX_SEM_H */
