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

#ifndef ZIX_THREAD_H
#define ZIX_THREAD_H

#include <errno.h>
#include <pthread.h>

#include "zix/common.h"

#ifdef __cplusplus
extern "C" {
#else
#    include <stdbool.h>
#endif

/**
   @addtogroup zix
   @{
   @name Thread
   @{
*/

typedef pthread_t ZixThread;

/**
   Initialize @c thread to a new thread.

   The thread will immediately be launched, calling @c function with @c arg
   as the only parameter.
*/
static inline ZixStatus
zix_thread_create(ZixThread* thread,
                  size_t     stack_size,
                  void*      (*function)(void*),
                  void*      arg);

/**
   Join @c thread (block until @c thread exits).
*/
static inline ZixStatus
zix_thread_join(ZixThread thread, void** retval);

static inline ZixStatus
zix_thread_create(ZixThread* thread,
                  size_t     stack_size,
                  void*      (*function)(void*),
                  void*      arg)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stack_size);

	const int ret = pthread_create(thread, NULL, function, arg);
	pthread_attr_destroy(&attr);

	if (ret == EAGAIN) {
		return ZIX_STATUS_NO_MEM;
	} else if (ret == EINVAL) {
		return ZIX_STATUS_BAD_ARG;
	} else if (ret == EPERM) {
		return ZIX_STATUS_BAD_PERMS;
	} else if (ret) {
		return ZIX_STATUS_ERROR;
	}

	return ZIX_STATUS_SUCCESS;
}

static inline ZixStatus
zix_thread_join(ZixThread thread, void** retval)
{
	return pthread_join(thread, retval)
		? ZIX_STATUS_ERROR : ZIX_STATUS_SUCCESS;
}

/**
   @}
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ZIX_THREAD_H */
