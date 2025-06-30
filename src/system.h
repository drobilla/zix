// Copyright 2022-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_SYSTEM_H
#define ZIX_SYSTEM_H

#include <zix/attributes.h>
#include <zix/status.h>

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef _WIN32
#  ifndef __GNUC__
typedef long ssize_t;
typedef int  mode_t;
#  endif
#else
typedef ssize_t ZixSystemCountReturn;
#endif

uint32_t
zix_system_page_size(void);

int
zix_system_open(const char* ZIX_NONNULL path, int flags, mode_t mode);

int
zix_system_close(int fd);

ZixStatus
zix_system_close_fds(int fd1, int fd2);

ssize_t
zix_system_read(int fd, void* ZIX_NONNULL buf, size_t count);

#endif // ZIX_SYSTEM_H
