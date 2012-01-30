/*
  Copyright 2012 David Robillard <http://drobilla.net>

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

#ifndef ZIX_SEM_H
#define ZIX_SEM_H

#include <stdbool.h>

#ifdef __APPLE__
#    include <limits.h>
#    include <CoreServices/CoreServices.h>
#else
#    include <semaphore.h>
#endif

#include "zix/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   @addtogroup zix
   @{
   @name Semaphore
   @{
*/

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
   Create and initialize @c sem to @c initial.
*/
static inline ZixStatus
zix_sem_init(ZixSem* sem, unsigned initial);

/**
   Destroy @c sem.
*/
static inline void
zix_sem_destroy(ZixSem* sem);

/**
   Increment (and signal any waiters).
   Realtime safe.
*/
static inline void
zix_sem_post(ZixSem* sem);

/**
   Wait until count is > 0, then decrement.
   Obviously not realtime safe.
*/
static inline void
zix_sem_wait(ZixSem* sem);

/**
   Non-blocking version of wait().

   @return true if decrement was successful (lock was acquired).
*/
static inline bool
zix_sem_try_wait(ZixSem* sem);

#ifdef __APPLE__

struct ZixSemImpl {
	MPSemaphoreID sem;
};

static inline ZixStatus
zix_sem_init(ZixSem* sem, unsigned initial)
{
	MPCreateSemaphore(UINT_MAX, initial, &_sem);
}

static inline void
zix_sem_destroy(ZixSem* sem)
{
	MPDeleteSemaphore(_sem);
}

static inline void
zix_sem_post(ZixSem* sem)
{
	MPSignalSemaphore(_sem);
}

static inline void
zix_sem_wait(ZixSem* sem)
{
	MPWaitOnSemaphore(sem->sem, kDurationForever);
}

static inline bool
zix_sem_try_wait(ZixSem* sem)
{
	return MPWaitOnSemaphore(sem->sem, kDurationImmediate) == noErr;
}

#else  /* !defined(__APPLE__) */

struct ZixSemImpl {
	sem_t sem;
};

static inline ZixStatus
zix_sem_init(ZixSem* sem, unsigned initial)
{
	return sem_init(&sem->sem, 0, initial)
		? ZIX_STATUS_SUCCESS : ZIX_STATUS_ERROR;
}

static inline void
zix_sem_destroy(ZixSem* sem)
{
	sem_destroy(&sem->sem);
}

static inline void
zix_sem_post(ZixSem* sem)
{
	sem_post(&sem->sem);
}

static inline void
zix_sem_wait(ZixSem* sem)
{
	/* Note that sem_wait always returns 0 in practice, except in
	   gdb (at least), where it returns nonzero, so the while is
	   necessary (and is the correct/safe solution in any case).
	*/
	while (sem_wait(&sem->sem) != 0) {}
}

static inline bool
zix_sem_try_wait(ZixSem* sem)
{
	return (sem_trywait(&sem->sem) == 0);
}

#endif

/**
   @}
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ZIX_SEM_H */
