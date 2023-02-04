# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Project-specific warning suppressions.
#
# This should be used in conjunction with the generic "warnings" sibling that
# enables all reasonable warnings for the compiler.  It lives here just to keep
# the top-level meson.build more readable.

#####
# C #
#####

if is_variable('cc')
  c_suppressions = []

  if get_option('strict')
    if cc.get_id() in ['clang', 'emscripten']
      c_suppressions += [
        '-Wno-bad-function-cast',
        '-Wno-c11-extensions', # Glib
        '-Wno-declaration-after-statement',
        '-Wno-implicit-fallthrough', # Really for clang < 12
        '-Wno-padded',
      ]

      if host_machine.system() == 'windows'
        c_suppressions += [
          '-Wno-deprecated-declarations',
          '-Wno-nonportable-system-include-path',
        ]
      endif

    elif cc.get_id() == 'gcc'
      c_suppressions += [
        '-Wno-bad-function-cast',
        '-Wno-cast-function-type',
        '-Wno-inline',
        '-Wno-padded',
        '-Wno-strict-overflow',
        '-Wno-switch-default',
        '-Wno-unsuffixed-float-constants',
      ]

      if host_machine.system() == 'windows'
        c_suppressions += [
          '-Wno-format',
          '-Wno-suggest-attribute=const',
          '-Wno-suggest-attribute=format',
          '-Wno-suggest-attribute=pure',
        ]
      endif

    elif cc.get_id() == 'msvc'
      c_suppressions += [
        '/wd4114', # same type qualifier used more than once
        '/wd4191', # unsafe function conversion
        '/wd4200', # zero-sized array in struct/union
        '/wd4365', # signed/unsigned mismatch
        '/wd4464', # relative include path contains ".."
        '/wd4514', # unreferenced inline function has been removed
        '/wd4710', # function not inlined
        '/wd4711', # function selected for automatic inline expansion
        '/wd4777', # format string and argument mismatch
        '/wd4800', # implicit conversion to bool
        '/wd4820', # padding added after construct
        '/wd4996', # POSIX name for this item is deprecated
        '/wd5045', # will insert Spectre mitigation for memory load
      ]
    endif
  endif

  if cc.get_id() == 'clang'
    c_suppressions += [
      '-Wno-nullability-extension',
    ]
  elif cc.get_id() == 'emscripten'
    c_suppressions += [
      '-Wno-format',
      '-Wno-nullability-extension',
    ]
  elif cc.get_id() == 'msvc'
    c_suppressions += [
      '/wd4706', # assignment within conditional expression
    ]
  endif

  c_suppressions = cc.get_supported_arguments(c_suppressions)
endif
