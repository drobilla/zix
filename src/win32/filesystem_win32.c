// Copyright 2007-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#define _WIN32_WINNT 0x0600 // Vista

#include "zix/bump_allocator.h"
#include "zix/filesystem.h"

#include "../errno_status.h"
#include "../zix_config.h"

#include "zix/allocator.h"
#include "zix/path.h"
#include "zix/status.h"

#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <sys/stat.h>
#include <windows.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline ZixStatus
zix_winerror_status(const DWORD e)
{
  switch (e) {
  case ERROR_NOT_ENOUGH_MEMORY:
  case ERROR_OUTOFMEMORY:
    return ZIX_STATUS_NO_MEM;
  case ERROR_SEM_TIMEOUT:
    return ZIX_STATUS_TIMEOUT;
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    return ZIX_STATUS_NOT_FOUND;
  case ERROR_BUFFER_OVERFLOW:
    return ZIX_STATUS_OVERFLOW;
  case ERROR_DISK_FULL:
    return ZIX_STATUS_NO_SPACE;
  case ERROR_ALREADY_EXISTS:
  case ERROR_FILE_EXISTS:
    return ZIX_STATUS_EXISTS;
  case ERROR_PRIVILEGE_NOT_HELD:
    return ZIX_STATUS_BAD_PERMS;
  case ERROR_LOCK_VIOLATION:
    return ZIX_STATUS_UNAVAILABLE;
  }

  return ZIX_STATUS_ERROR;
}

static inline ZixStatus
zix_windows_status(const bool success)
{
  return success ? ZIX_STATUS_SUCCESS : zix_winerror_status(GetLastError());
}

ZixStatus
zix_copy_file(ZixAllocator* const  allocator,
              const char* const    src,
              const char* const    dst,
              const ZixCopyOptions options)
{
  (void)allocator;

  return zix_windows_status(
    CopyFile(src, dst, !(options & ZIX_COPY_OPTION_OVERWRITE_EXISTING)));
}

char*
zix_create_temporary_directory(ZixAllocator* const allocator,
                               const char* const   path_pattern)
{
  static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static const int  n_chars = sizeof(chars) - 1;

  // Ensure that the pattern ends with "XXXXXX"
  const size_t length = strlen(path_pattern);
  if (length < 7 || strcmp(path_pattern + length - 6, "XXXXXX")) {
    errno = EINVAL;
    return NULL;
  }

  // Allocate a result to manipulate as we search for paths
  char* const result = (char*)zix_calloc(allocator, length + 1U, 1U);
  if (!result) {
    return NULL;
  }

  // Repeatedly try creating a directory with random suffixes
  memcpy(result, path_pattern, length + 1U);
  char* const suffix = result + length - 6U;
  for (unsigned attempt = 0U; attempt < 128U; ++attempt) {
    for (unsigned i = 0U; i < 6U; ++i) {
      suffix[i] = chars[rand() % n_chars];
    }

    if (!_mkdir(result)) {
      return result;
    }
  }

  zix_free(allocator, result);
  return NULL;
}

ZixStatus
zix_remove(const char* const path)
{
  return ((zix_file_type(path) == ZIX_FILE_TYPE_DIRECTORY)
            ? zix_windows_status(RemoveDirectory(path))
          : remove(path) ? zix_errno_status(errno)
                         : ZIX_STATUS_SUCCESS);
}

void
zix_dir_for_each(const char* const path,
                 void* const       data,
                 void (*const f)(const char* path,
                                 const char* name,
                                 void*       data))
{
  const size_t path_len = strlen(path);
  char         pat[MAX_PATH + 2U];
  memcpy(pat, path, path_len + 1U);
  pat[path_len]      = '\\';
  pat[path_len + 1U] = '*';
  pat[path_len + 2U] = '\0';

  WIN32_FIND_DATA fd;
  HANDLE          fh = FindFirstFile(pat, &fd);
  if (fh != INVALID_HANDLE_VALUE) {
    do {
      if (!!strcmp(fd.cFileName, ".") && !!strcmp(fd.cFileName, "..")) {
        f(path, fd.cFileName, data);
      }
    } while (FindNextFile(fh, &fd));
  }
  FindClose(fh);
}

ZixStatus
zix_file_lock(FILE* const file, const ZixFileLockMode mode)
{
  HANDLE     handle     = (HANDLE)_get_osfhandle(fileno(file));
  OVERLAPPED overlapped = {0};

  const DWORD flags =
    (LOCKFILE_EXCLUSIVE_LOCK |
     (mode == ZIX_FILE_LOCK_TRY ? LOCKFILE_FAIL_IMMEDIATELY : 0));

  return zix_windows_status(
    LockFileEx(handle, flags, 0, UINT32_MAX, UINT32_MAX, &overlapped));
}

ZixStatus
zix_file_unlock(FILE* const file, const ZixFileLockMode mode)
{
  (void)mode;

  HANDLE     handle     = (HANDLE)_get_osfhandle(fileno(file));
  OVERLAPPED overlapped = {0};

  return zix_windows_status(
    UnlockFileEx(handle, 0, UINT32_MAX, UINT32_MAX, &overlapped));
}

char*
zix_canonical_path(ZixAllocator* const allocator, const char* const path)
{
  char full[MAX_PATH] = {0};
  if (!path || !GetFullPathName(path, MAX_PATH, full, NULL)) {
    return NULL;
  }

  const HANDLE h =
    CreateFile(full,
               FILE_READ_ATTRIBUTES,
               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
               NULL,
               OPEN_EXISTING,
               FILE_FLAG_BACKUP_SEMANTICS,
               NULL);

  const DWORD flags      = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
  const DWORD final_size = GetFinalPathNameByHandle(h, NULL, 0U, flags);
  if (!final_size) {
    CloseHandle(h);
    return NULL;
  }

  char* const final = (char*)zix_calloc(allocator, final_size + 1U, 1U);
  if (final && !GetFinalPathNameByHandle(h, final, final_size + 1U, flags)) {
    zix_free(allocator, final);
    CloseHandle(h);
    return NULL;
  }

  if (final_size > 4U && !strncmp(final, "\\\\?\\", 4)) {
    memmove(final, final + 4U, final_size - 4U);
    final[final_size - 4U] = '\0';
  }

  CloseHandle(h);
  return final;
}

static ZixFileType
attrs_file_type(const DWORD attrs)
{
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return ZIX_FILE_TYPE_NONE;
  }

  if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
    return ZIX_FILE_TYPE_DIRECTORY;
  }

  if (attrs & FILE_ATTRIBUTE_REPARSE_POINT) {
    return ZIX_FILE_TYPE_SYMLINK;
  }

  if (attrs & (FILE_ATTRIBUTE_DEVICE)) {
    return ZIX_FILE_TYPE_UNKNOWN;
  }

  return ZIX_FILE_TYPE_REGULAR;
}

ZixFileType
zix_file_type(const char* const path)
{
  const ZixFileType type = attrs_file_type(GetFileAttributes(path));
  if (type != ZIX_FILE_TYPE_SYMLINK) {
    return type;
  }

  // Resolve symlink to find the canonical type
  char             buf[MAX_PATH];
  ZixBumpAllocator allocator = zix_bump_allocator(sizeof(buf), buf);
  char* const      canonical = zix_canonical_path(&allocator.base, path);
  return zix_file_type(canonical);
}

ZixFileType
zix_symlink_type(const char* const path)
{
  return attrs_file_type(GetFileAttributes(path));
}

ZixStatus
zix_create_directory(const char* const dir_path)
{
  return (!dir_path[0])     ? ZIX_STATUS_BAD_ARG
         : _mkdir(dir_path) ? zix_errno_status(errno)
                            : ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_create_directory_like(const char* const dir_path,
                          const char* const existing_path)
{
  return (zix_file_type(existing_path) != ZIX_FILE_TYPE_DIRECTORY)
           ? ZIX_STATUS_NOT_FOUND
           : zix_create_directory(dir_path);
}

ZixStatus
zix_create_symlink(const char* const target_path, const char* const link_path)
{
#if USE_CREATESYMBOLICLINK
  static const DWORD flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;

  return zix_windows_status(CreateSymbolicLink(link_path, target_path, flags));
#else
  (void)target_path;
  (void)link_path;
  return ZIX_STATUS_NOT_SUPPORTED;
#endif
}

ZixStatus
zix_create_directory_symlink(const char* const target_path,
                             const char* const link_path)
{
#if USE_CREATESYMBOLICLINK
  static const DWORD flags =
    SYMBOLIC_LINK_FLAG_DIRECTORY | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;

  return zix_windows_status(CreateSymbolicLink(link_path, target_path, flags));
#else
  (void)target_path;
  (void)link_path;
  return ZIX_STATUS_NOT_SUPPORTED;
#endif
}

ZixStatus
zix_create_hard_link(const char* const target_path, const char* const link_path)
{
  return zix_windows_status(CreateHardLink(link_path, target_path, NULL));
}

char*
zix_temp_directory_path(ZixAllocator* const allocator)
{
  const DWORD size = GetTempPath(0U, NULL);
  char* const buf  = (char*)zix_calloc(allocator, size, 1);
  if (buf && (GetTempPath(size, buf) != size - 1U)) {
    zix_free(allocator, buf);
    return NULL;
  }

  return buf;
}

char*
zix_current_path(ZixAllocator* const allocator)
{
  const DWORD size = GetCurrentDirectory(0U, NULL);
  char* const buf  = (char*)zix_calloc(allocator, size, 1);
  if (buf && (GetCurrentDirectory(size, buf) != size - 1U)) {
    zix_free(allocator, buf);
    return NULL;
  }

  return buf;
}
