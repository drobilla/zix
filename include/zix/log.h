// Copyright 2011-2024 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_LOG_H
#define ZIX_LOG_H

#include "zix/attributes.h"
#include "zix/status.h"
#include "zix/string_view.h"

#include <stdarg.h>
#include <stddef.h>

ZIX_BEGIN_DECLS

/**
   @defgroup zix_log Log
   @ingroup zix_logging
   @{
*/

/// Log entry level, compatible with syslog
typedef enum {
  ZIX_LOG_LEVEL_EMERGENCY, ///< Emergency, system is unusable
  ZIX_LOG_LEVEL_ALERT,     ///< Action must be taken immediately
  ZIX_LOG_LEVEL_CRITICAL,  ///< Critical condition
  ZIX_LOG_LEVEL_ERROR,     ///< Error
  ZIX_LOG_LEVEL_WARNING,   ///< Warning
  ZIX_LOG_LEVEL_NOTICE,    ///< Normal but significant condition
  ZIX_LOG_LEVEL_INFO,      ///< Informational message
  ZIX_LOG_LEVEL_DEBUG,     ///< Debug message
} ZixLogLevel;

/**
   A structured log field.

   Fields are used to add metadata to log messages.  Syslog-compatible keys
   should be used where possible, otherwise, keys should be namespaced to
   prevent clashes.

   The standard output formatter uses the following extended keys:

   - ERRNO - The `errno` of the original system error if any (decimal string)
   - DATA_FILE - The file which caused this message (string)
   - DATA_LINE - The 1-based line number in the file (decimal string)
   - DATA_COL - The 1-based column number in the file (decimal string)
   - DATA_CHECK - The check/warning/etc that triggered this message (string)
*/
typedef struct {
  const char* ZIX_NONNULL key;   ///< Field name
  const char* ZIX_NONNULL value; ///< Field value
} ZixLogField;

typedef struct ZixLogImpl ZixLog;

ZIX_API
ZixLog* ZIX_ALLOCATED
zix_log_new(ZixAllocator* ZIX_NULLABLE allocator);

ZIX_API
ZixStatus
zix_log_open_std(ZixLog* ZIX_NONNULL log);

ZIX_API
zix_log_free(ZixLog* ZIX_NULLABLE log);

/**
   Write a log message with a format string and `va_list` of arguments.

   This is the fundamental and most powerful function for writing log entries,
   the others are convenience wrappers that ultimately call this.

   This writes a single complete entry to the log, and so may not be used to
   print parts of a line like a more general printf-like function.  There
   should be no trailing newline in `fmt`.  Arguments following `fmt` should
   correspond to conversion specifiers in the format string as in printf from
   the standard C library.

   @param world World to log to.
   @param level Log level.
   @param n_fields Number of entries in `fields`.
   @param fields An array of `n_fields` extra log fields.
   @param fmt Format string.
   @param args Arguments for `fmt`.

   @return A status code, which is always #ZIX_SUCCESS with the default log
   function.  If a custom log function is set with serd_set_log_func() and it
   returns an error, then that error is returned here.
*/
ZIX_LOG_FUNC(5, 0)
ZIX_API
ZixStatus
zix_vxlogf(ZixLog* ZIX_NONNULL             log,
           ZixLogLevel                     level,
           size_t                          n_fields,
           const ZixLogField* ZIX_NULLABLE fields,
           const char* ZIX_NONNULL         fmt,
           va_list                         args);

/**
   Write a log message with extra fields.

   This is a convenience wrapper for serd_vxlogf() that takes the format
   arguments directly.
*/
ZIX_LOG_FUNC(5, 6)
ZIX_API
ZixStatus
zix_xlogf(ZixLog* ZIX_NONNULL             log,
          ZixLogLevel                     level,
          size_t                          n_fields,
          const ZixLogField* ZIX_NULLABLE fields,
          const char* ZIX_NONNULL         fmt,
          ...);

/**
   Write a simple message to the log.

   This is a convenience wrapper for serd_vxlogf() which sets no extra fields.
*/
ZIX_LOG_FUNC(3, 0)
ZIX_API
ZixStatus
zix_vlogf(ZixLog* ZIX_NONNULL     log,
          ZixLogLevel             level,
          const char* ZIX_NONNULL fmt,
          va_list                 args);

/**
   Write a simple message to the log.

   This is a convenience wrapper for serd_vlogf() that takes the format
   arguments directly.
*/
ZIX_LOG_FUNC(3, 4)
ZIX_API
ZixStatus
zix_logf(ZixLog* ZIX_NONNULL     log,
         ZixLogLevel             level,
         const char* ZIX_NONNULL fmt,
         ...);

/**
   @}
*/

ZIX_END_DECLS

#endif // ZIX_LOG_H
