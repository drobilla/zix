// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "zix_config.h"

#include "zix/common.h"
#include "zix/sem.h"

#ifdef __APPLE__
#  include <mach/mach.h>
#elif defined(_WIN32)
#  include <limits.h>
#  include <windows.h>
#else
#  include <errno.h>
#  include <semaphore.h>
#endif

#include <stdint.h>
#include <time.h>

#ifdef __APPLE__

ZixStatus
zix_sem_init(ZixSem* sem, unsigned val)
{
  return semaphore_create(
           mach_task_self(), &sem->sem, SYNC_POLICY_FIFO, (int)val)
           ? ZIX_STATUS_ERROR
           : ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_sem_destroy(ZixSem* sem)
{
  return semaphore_destroy(mach_task_self(), sem->sem) ? ZIX_STATUS_ERROR
                                                       : ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_sem_post(ZixSem* sem)
{
  return semaphore_signal(sem->sem) ? ZIX_STATUS_ERROR : ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_sem_wait(ZixSem* sem)
{
  kern_return_t r = 0;
  while ((r = semaphore_wait(sem->sem)) && r == KERN_ABORTED) {
    // Interrupted, try again
  }

  return r ? ZIX_STATUS_ERROR : ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_sem_try_wait(ZixSem* sem)
{
  const mach_timespec_t zero = {0, 0};
  const kern_return_t   r    = semaphore_timedwait(sem->sem, zero);

  return (r == KERN_SUCCESS)               ? ZIX_STATUS_SUCCESS
         : (r == KERN_OPERATION_TIMED_OUT) ? ZIX_STATUS_UNAVAILABLE
                                           : ZIX_STATUS_ERROR;
}

ZixStatus
zix_sem_timed_wait(ZixSem*        sem,
                   const uint32_t seconds,
                   const uint32_t nanoseconds)
{
  const mach_timespec_t interval = {seconds, (clock_res_t)nanoseconds};
  const kern_return_t   r        = semaphore_timedwait(sem->sem, interval);

  return (r == KERN_SUCCESS)               ? ZIX_STATUS_SUCCESS
         : (r == KERN_OPERATION_TIMED_OUT) ? ZIX_STATUS_TIMEOUT
                                           : ZIX_STATUS_ERROR;
}

#elif defined(_WIN32)

ZixStatus
zix_sem_init(ZixSem* sem, unsigned initial)
{
  sem->sem = CreateSemaphore(NULL, (LONG)initial, LONG_MAX, NULL);
  return sem->sem ? ZIX_STATUS_SUCCESS : ZIX_STATUS_ERROR;
}

ZixStatus
zix_sem_destroy(ZixSem* sem)
{
  return CloseHandle(sem->sem) ? ZIX_STATUS_SUCCESS : ZIX_STATUS_ERROR;
}

ZixStatus
zix_sem_post(ZixSem* sem)
{
  return ReleaseSemaphore(sem->sem, 1, NULL) ? ZIX_STATUS_SUCCESS
                                             : ZIX_STATUS_ERROR;
}

ZixStatus
zix_sem_wait(ZixSem* sem)
{
  return WaitForSingleObject(sem->sem, INFINITE) == WAIT_OBJECT_0
           ? ZIX_STATUS_SUCCESS
           : ZIX_STATUS_ERROR;
}

ZixStatus
zix_sem_try_wait(ZixSem* sem)
{
  const DWORD r = WaitForSingleObject(sem->sem, 0);

  return (r == WAIT_OBJECT_0)  ? ZIX_STATUS_SUCCESS
         : (r == WAIT_TIMEOUT) ? ZIX_STATUS_UNAVAILABLE
                               : ZIX_STATUS_ERROR;
}

ZixStatus
zix_sem_timed_wait(ZixSem*        sem,
                   const uint32_t seconds,
                   const uint32_t nanoseconds)
{
  const uint32_t milliseconds = seconds * 1000U + nanoseconds / 1000000U;
  const DWORD    r            = WaitForSingleObject(sem->sem, milliseconds);

  return (r == WAIT_OBJECT_0)  ? ZIX_STATUS_SUCCESS
         : (r == WAIT_TIMEOUT) ? ZIX_STATUS_TIMEOUT
                               : ZIX_STATUS_ERROR;
}

#else /* !defined(__APPLE__) && !defined(_WIN32) */

ZixStatus
zix_sem_init(ZixSem* sem, unsigned initial)
{
  return zix_errno_status_if(sem_init(&sem->sem, 0, initial));
}

ZixStatus
zix_sem_destroy(ZixSem* sem)
{
  return zix_errno_status_if(sem_destroy(&sem->sem));
}

ZixStatus
zix_sem_post(ZixSem* sem)
{
  return zix_errno_status_if(sem_post(&sem->sem));
}

ZixStatus
zix_sem_wait(ZixSem* sem)
{
  int r = 0;
  while ((r = sem_wait(&sem->sem)) && errno == EINTR) {
    // Interrupted, try again
  }

  return zix_errno_status_if(r);
}

ZixStatus
zix_sem_try_wait(ZixSem* sem)
{
  int r = 0;
  while ((r = sem_trywait(&sem->sem)) && errno == EINTR) {
    // Interrupted, try again
  }

  return zix_errno_status_if(r);
}

ZixStatus
zix_sem_timed_wait(ZixSem*        sem,
                   const uint32_t seconds,
                   const uint32_t nanoseconds)
{
#  if !USE_CLOCK_GETTIME || !USE_SEM_TIMEDWAIT
  return ZIX_STATUS_NOT_SUPPORTED;
#  else

  struct timespec ts = {0, 0};

  int r = 0;
  if (!(r = clock_gettime(CLOCK_REALTIME, &ts))) {
    ts.tv_sec += (time_t)seconds;
    ts.tv_nsec += (long)nanoseconds;

    while ((r = sem_timedwait(&sem->sem, &ts)) && errno == EINTR) {
      // Interrupted, try again
    }
  }

  return zix_errno_status_if(r);
#  endif
}

#endif
