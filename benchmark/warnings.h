// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef WARNINGS_H
#define WARNINGS_H

#if defined(__clang__)

#  define ZIX_DISABLE_GLIB_WARNINGS                                         \
    _Pragma("clang diagnostic push")                                        \
    _Pragma("clang diagnostic ignored \"-Wdocumentation-unknown-command\"") \
    _Pragma("clang diagnostic ignored \"-Wdocumentation\"")

#  define ZIX_RESTORE_WARNINGS _Pragma("clang diagnostic pop")

#elif defined(__GNUC__)

#  define ZIX_DISABLE_GLIB_WARNINGS _Pragma("GCC diagnostic push")

#  define ZIX_RESTORE_WARNINGS _Pragma("GCC diagnostic pop")

#else

#  define ZIX_DISABLE_GLIB_WARNINGS
#  define ZIX_RESTORE_WARNINGS

#endif

#endif // WARNINGS_H
