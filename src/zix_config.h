// Copyright 2021-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Configuration header that defines reasonable defaults at compile-time.

  This allows configuration from the command-line (usually by the build system)
  while still allowing the code to compile "as-is" with reasonable default
  features on supported platforms.

  This system is designed so that, ideally, no command-line or build-system
  configuration is needed, but automatic feature detection can be disabled or
  overridden for maximum control.  It should never be necessary to edit the
  source code to achieve a given configuration.

  Usage:

  - By default, features are enabled if they can be detected or assumed to be
    available from the build environment, unless `ZIX_NO_DEFAULT_CONFIG` is
    defined, which disables everything by default.

  - If a symbol like `HAVE_SOMETHING` is defined to non-zero, then the
    "something" feature is assumed to be available.

  Code rules:

  - To check for a feature, this header must be included, and the symbol
    `USE_SOMETHING` used as a boolean in an `#if` expression.

  - None of the other configuration symbols described here may be used
    directly.  In particular, this header should be the only place in the
    source code that touches `HAVE` symbols.
*/

#ifndef ZIX_CONFIG_H
#define ZIX_CONFIG_H

#if !defined(ZIX_NO_DEFAULT_CONFIG)

// We need unistd.h to check _POSIX_VERSION
#  ifndef ZIX_NO_POSIX
#    ifdef __has_include
#      if __has_include(<unistd.h>)
#        include <unistd.h>
#      endif
#    elif defined(__APPLE__) || defined(__unix__)
#      include <unistd.h>
#    endif
#  endif

// Define ZIX_POSIX_VERSION unconditionally for convenience
#  if defined(_POSIX_VERSION)
#    define ZIX_POSIX_VERSION _POSIX_VERSION
#  else
#    define ZIX_POSIX_VERSION 0
#  endif

// POSIX.1-2001: clock_gettime()
#  ifndef HAVE_CLOCK_GETTIME
#    if ZIX_POSIX_VERSION >= 200112L
#      define HAVE_CLOCK_GETTIME 1
#    endif
#  endif

// MacOS: clonefile()
#  ifndef HAVE_CLONEFILE
#    if defined(__APPLE__) && __has_include(<sys/clonefile.h>)
#      define HAVE_CLONEFILE 1
#    endif
#  endif

// FreeBSD 13, Linux 4.5, and glibc 2.27: copy_file_range()
#  ifndef HAVE_COPY_FILE_RANGE
#    if (defined(__FreeBSD__) && __FreeBSD__ >= 13) || defined(__linux__) || \
      (defined(__GLIBC__) &&                                                 \
       (__GLIBC__ > 2 || __GLIBC__ == 2 && __GLIBC_MINOR__ >= 27))
#      define HAVE_COPY_FILE_RANGE 1
#    endif
#  endif

// Windows: CreateSymbolicLink()
#  ifndef HAVE_CREATESYMBOLICLINK
#    if defined(_MSC_VER) && _MSC_VER >= 1910
#      define HAVE_CREATESYMBOLICLINK 1
#    endif
#  endif

// POSIX.1-2001, Windows: fileno()
#  ifndef HAVE_FILENO
#    if defined(_WIN32) || defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#      define HAVE_FILENO 1
#    endif
#  endif

// Classic UNIX: flock()
#  ifndef HAVE_FLOCK
#    if defined(__APPLE__) || defined(__unix__)
#      define HAVE_FLOCK 1
#    endif
#  endif

// POSIX.1-2001: mlock()
#  ifndef HAVE_MLOCK
#    if ZIX_POSIX_VERSION >= 200112L
#      define HAVE_MLOCK 1
#    endif
#  endif

// POSIX.1-2001: pathconf()
#  ifndef HAVE_PATHCONF
#    if ZIX_POSIX_VERSION >= 200112L
#      define HAVE_PATHCONF 1
#    endif
#  endif

// POSIX.1-2001: posix_fadvise()
#  ifndef HAVE_POSIX_FADVISE
#    if ZIX_POSIX_VERSION >= 200112L
#      define HAVE_POSIX_FADVISE 1
#    endif
#  endif

// POSIX.1-2001: posix_memalign()
#  ifndef HAVE_POSIX_MEMALIGN
#    if ZIX_POSIX_VERSION >= 200112L
#      define HAVE_POSIX_MEMALIGN 1
#    endif
#  endif

// POSIX.1-2001: realpath()
#  ifndef HAVE_REALPATH
#    if ZIX_POSIX_VERSION >= 200112L
#      define HAVE_REALPATH 1
#    endif
#  endif

// POSIX.1-2001: sem_timedwait()
#  ifndef HAVE_SEM_TIMEDWAIT
#    if ZIX_POSIX_VERSION >= 200112L
#      define HAVE_SEM_TIMEDWAIT 1
#    endif
#  endif

// POSIX.1-2001: sysconf()
#  ifndef HAVE_SYSCONF
#    if ZIX_POSIX_VERSION >= 200112L
#      define HAVE_SYSCONF 1
#    endif
#  endif

#endif // !defined(ZIX_NO_DEFAULT_CONFIG)

/*
  Make corresponding USE_FEATURE defines based on the HAVE_FEATURE defines from
  above or the command line.  The code checks for these using #if (not #ifdef),
  so there will be an undefined warning if it checks for an unknown feature,
  and this header is always required by any code that checks for features, even
  if the build system defines them all.
*/

#if defined(HAVE_CLOCK_GETTIME) && HAVE_CLOCK_GETTIME
#  define USE_CLOCK_GETTIME 1
#else
#  define USE_CLOCK_GETTIME 0
#endif

#if defined(HAVE_CLONEFILE) && HAVE_CLONEFILE
#  define USE_CLONEFILE 1
#else
#  define USE_CLONEFILE 0
#endif

#if defined(HAVE_COPY_FILE_RANGE) && HAVE_COPY_FILE_RANGE
#  define USE_COPY_FILE_RANGE 1
#else
#  define USE_COPY_FILE_RANGE 0
#endif

#if defined(HAVE_CREATESYMBOLICLINK) && HAVE_CREATESYMBOLICLINK
#  define USE_CREATESYMBOLICLINK 1
#else
#  define USE_CREATESYMBOLICLINK 0
#endif

#if defined(HAVE_FILENO) && HAVE_FILENO
#  define USE_FILENO 1
#else
#  define USE_FILENO 0
#endif

#if defined(HAVE_FLOCK) && HAVE_FLOCK
#  define USE_FLOCK 1
#else
#  define USE_FLOCK 0
#endif

#if defined(HAVE_MLOCK) && HAVE_MLOCK
#  define USE_MLOCK 1
#else
#  define USE_MLOCK 0
#endif

#if defined(HAVE_PATHCONF) && HAVE_PATHCONF
#  define USE_PATHCONF 1
#else
#  define USE_PATHCONF 0
#endif

#if defined(HAVE_POSIX_FADVISE) && HAVE_POSIX_FADVISE
#  define USE_POSIX_FADVISE 1
#else
#  define USE_POSIX_FADVISE 0
#endif

#if defined(HAVE_POSIX_MEMALIGN) && HAVE_POSIX_MEMALIGN
#  define USE_POSIX_MEMALIGN 1
#else
#  define USE_POSIX_MEMALIGN 0
#endif

#if defined(HAVE_REALPATH) && HAVE_REALPATH
#  define USE_REALPATH 1
#else
#  define USE_REALPATH 0
#endif

#if defined(HAVE_SEM_TIMEDWAIT) && HAVE_SEM_TIMEDWAIT
#  define USE_SEM_TIMEDWAIT 1
#else
#  define USE_SEM_TIMEDWAIT 0
#endif

#if defined(HAVE_SYSCONF) && HAVE_SYSCONF
#  define USE_SYSCONF 1
#else
#  define USE_SYSCONF 0
#endif

#endif // ZIX_CONFIG_H
