/*
  Copyright 2021 David Robillard <d@drobilla.net>

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

#ifndef ZIX_ATTRIBUTES_H
#define ZIX_ATTRIBUTES_H

/**
   @addtogroup zix
   @{
*/

// ZIX_API must be used to decorate things in the public API
#ifndef ZIX_API
#  if defined(_WIN32) && !defined(ZIX_STATIC) && defined(ZIX_INTERNAL)
#    define ZIX_API __declspec(dllexport)
#  elif defined(_WIN32) && !defined(ZIX_STATIC)
#    define ZIX_API __declspec(dllimport)
#  elif defined(__GNUC__)
#    define ZIX_API __attribute__((visibility("default")))
#  else
#    define ZIX_API
#  endif
#endif

// GCC pure/const/malloc attributes
#ifdef __GNUC__
#  define ZIX_PURE_FUNC __attribute__((pure))
#  define ZIX_CONST_FUNC __attribute__((const))
#  define ZIX_MALLOC_FUNC __attribute__((malloc))
#else
#  define ZIX_PURE_FUNC
#  define ZIX_CONST_FUNC
#  define ZIX_MALLOC_FUNC
#endif

#define ZIX_PURE_API \
  ZIX_API            \
  ZIX_PURE_FUNC

#define ZIX_CONST_API \
  ZIX_API             \
  ZIX_CONST_FUNC

#define ZIX_MALLOC_API \
  ZIX_API              \
  ZIX_MALLOC_FUNC

// Printf-like format functions
#ifdef __GNUC__
#  define ZIX_LOG_FUNC(fmt, arg1) __attribute__((format(printf, fmt, arg1)))
#else
#  define ZIX_LOG_FUNC(fmt, arg1)
#endif

// Unused parameter macro to suppresses warnings and make it impossible to use
#if defined(__cplusplus)
#  define ZIX_UNUSED(name)
#elif defined(__GNUC__)
#  define ZIX_UNUSED(name) name##_unused __attribute__((__unused__))
#else
#  define ZIX_UNUSED(name) name
#endif

/**
   @}
*/

#endif /* ZIX_ATTRIBUTES_H */
