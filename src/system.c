// Copyright 2007-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "system.h"

#include "errno_status.h"

#include "zix/status.h"

#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif

#include <errno.h>
#include <fcntl.h>

int
zix_system_open_fd(const char* const path, const int flags, const mode_t mode)
{
#ifdef O_CLOEXEC
  return open(path, flags | O_CLOEXEC, mode); // NOLINT(hicpp-signed-bitwise)
#else
  return open(path, flags, mode);
#endif
}

ZixStatus
zix_system_close_fds(const int fd1, const int fd2)
{
  // Careful: we need to always close both files, but catch errno at any point

  const ZixStatus st0 = zix_errno_status(errno);
  const int       r1  = fd1 >= 0 ? close(fd1) : 0;
  const ZixStatus st1 = r1 ? ZIX_STATUS_SUCCESS : zix_errno_status(errno);
  const int       r2  = fd2 >= 0 ? close(fd2) : 0;
  const ZixStatus st2 = r2 ? ZIX_STATUS_SUCCESS : zix_errno_status(errno);

  return st0 ? st0 : st1 ? st1 : st2;
}
