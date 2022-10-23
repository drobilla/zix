// Copyright 2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_SYSTEM_H
#define ZIX_SYSTEM_H

#include "zix/status.h"

#include <stdint.h>
#include <sys/types.h>

#ifdef _WIN32
typedef int ZixSystemCountReturn;
#  ifndef __GNUC__
typedef int mode_t;
#  endif
#else
typedef ssize_t ZixSystemCountReturn;
#endif

uint32_t
zix_system_page_size(void);

int
zix_system_open_fd(const char* path, int flags, mode_t mode);

ZixStatus
zix_system_close_fds(int fd1, int fd2);

#endif // ZIX_SYSTEM_H
