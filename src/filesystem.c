// Copyright 2007-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "zix/filesystem.h"

#include "index_range.h"
#include "path_iter.h"
#include "system.h"

#include "zix/allocator.h"
#include "zix/status.h"

#ifdef _WIN32
#  include <direct.h>
#  include <io.h>
#else
#  include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

ZixStatus
zix_create_directories(ZixAllocator* const allocator,
                       const char* const   dir_path)
{
  if (!dir_path[0]) {
    return ZIX_STATUS_BAD_ARG;
  }

  // Allocate a working copy of the path to chop along the way
  const size_t path_len = strlen(dir_path);
  char* const  path     = (char*)zix_malloc(allocator, path_len + 1U);
  memcpy(path, dir_path, path_len + 1U);

  // Start at the root directory (past any name)
  ZixPathIter p = zix_path_begin(path);
  while (p.state < ZIX_PATH_FILE_NAME) {
    p = zix_path_next(path, p);
  }

  // Create each directory down the path
  ZixStatus st = ZIX_STATUS_SUCCESS;
  while (p.state != ZIX_PATH_END) {
    const char old_end = path[p.range.end];

    path[p.range.end] = '\0';
    if (zix_file_type(path) != ZIX_FILE_TYPE_DIRECTORY) {
      if ((st = zix_create_directory(path))) {
        break;
      }
    }

    path[p.range.end] = old_end;
    p                 = zix_path_next(path, p);
  }

  zix_free(allocator, path);
  return st;
}

ZixFileOffset
zix_file_size(const char* const path)
{
  struct stat sb;
  return stat(path, &sb) ? (off_t)0 : sb.st_size;
}

bool
zix_file_equals(ZixAllocator* const allocator,
                const char* const   path_a,
                const char* const   path_b)
{
  if (!strcmp(path_a, path_b)) {
    return true; // Paths match
  }

  errno = 0;

  // Open files and get file information
  const int   fd_a = zix_system_open_fd(path_a, O_RDONLY, 0);
  const int   fd_b = zix_system_open_fd(path_b, O_RDONLY, 0);
  struct stat stat_a;
  struct stat stat_b;
  if (fd_a < 0 || fd_b < 0 || fstat(fd_a, &stat_a) || fstat(fd_b, &stat_b)) {
    zix_system_close_fds(fd_b, fd_a);
    return false;
  }

  bool match = false;
  if (stat_a.st_dev == stat_b.st_dev && stat_a.st_ino && stat_b.st_ino &&
      stat_a.st_ino == stat_b.st_ino) {
    match = true; // Fast path: paths refer to the same file
  } else if (stat_a.st_size == stat_b.st_size) {
    // Slow path: files have equal size, compare contents
    const uint32_t size   = zix_system_page_size();
    void* const    page_a = zix_aligned_alloc(allocator, size, size);
    void* const    page_b = zix_aligned_alloc(allocator, size, size);

    if (page_a && page_b) {
      match = true;
      for (ZixSystemCountReturn n = 0; (n = read(fd_a, page_a, size)) > 0;) {
        if (read(fd_b, page_b, size) != n ||
            !!memcmp(page_a, page_b, (size_t)n)) {
          match = false;
          break;
        }
      }
    }

    zix_aligned_free(allocator, page_b);
    zix_aligned_free(allocator, page_a);
  }

  return !zix_system_close_fds(fd_b, fd_a) && match;
}
