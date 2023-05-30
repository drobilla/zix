# Copyright 2020-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

if not meson.is_subproject() and get_option('lint')
  # Check release metadata
  autoship = find_program('autoship', required: get_option('tests'))
  if autoship.found()
    test(
      'autoship',
      autoship,
      args: ['test', zix_src_root],
      suite: 'data',
    )
  endif

  # Check licensing metadata
  reuse = find_program('reuse', required: get_option('tests'))
  if reuse.found()
    test(
      'REUSE',
      reuse,
      args: ['--root', zix_src_root, 'lint'],
      suite: 'data',
    )
  endif
endif

# Set warning suppression flags specific to tests
test_suppressions = []
if cc.get_id() in ['clang', 'emscripten']
  if host_machine.system() == 'windows'
    test_suppressions += [
      '-Wno-format-nonliteral',
    ]
  endif
endif

common_test_sources = files('failing_allocator.c')

# Single-threaded tests that should run everywhere
sequential_tests = [
  'allocator',
  'btree',
  'digest',
  'hash',
  'path',
  'status',
  'tree',
]

# Multi-threaded tests that require thread support
threaded_tests = [
  'ring',
  'sem',
  'thread',
]

foreach test : sequential_tests
  sources = common_test_sources + files('test_@0@.c'.format(test))

  test(
    test,
    executable(
      'test_@0@'.format(test),
      sources,
      c_args: c_suppressions + program_c_args + test_suppressions,
      dependencies: [zix_dep],
      include_directories: include_dirs,
      link_args: program_link_args,
    ),
    suite: 'unit',
    timeout: 120,
  )
endforeach

test(
  'filesystem',
  executable(
    'test_filesystem',
    files('test_filesystem.c'),
    c_args: c_suppressions + program_c_args,
    dependencies: [zix_dep],
    include_directories: include_dirs,
    link_args: program_link_args,
  ),
  args: files('../README.md'),
  suite: 'unit',
  timeout: 120,
)

if thread_dep.found()
  foreach test : threaded_tests
    sources = common_test_sources + files('test_@0@.c'.format(test))

    test(
      test,
      executable(
        'test_@0@'.format(test),
        sources,
        c_args: c_suppressions + program_c_args + test_suppressions,
        dependencies: [zix_dep, thread_dep],
        include_directories: include_dirs,
        link_args: program_link_args,
      ),
      suite: 'unit',
      timeout: 120,
    )
  endforeach
endif

# Test that headers have no warnings (ignoring the usual suppressions)
if cc.get_id() != 'emscripten'
  header_suppressions = []
  if cc.get_id() in ['clang', 'emscripten']
    header_suppressions += [
      '-Wno-declaration-after-statement',
      '-Wno-nullability-extension',
      '-Wno-padded',
    ]

    if not meson.is_cross_build()
      header_suppressions += [
        '-Wno-poison-system-directories',
      ]
    endif

    if host_machine.system() == 'windows'
      header_suppressions += [
        '-Wno-nonportable-system-include-path',
      ]
    endif

  elif cc.get_id() == 'gcc'
    header_suppressions += [
      '-Wno-padded',
      '-Wno-unused-const-variable',
    ]

  elif cc.get_id() == 'msvc'
    header_suppressions += [
      '/experimental:external',
      '/external:W0',
      '/external:anglebrackets',

      '/wd4820', # padding added after construct
    ]
  endif

  test(
    'headers',
    executable(
      'test_headers',
      files('headers/test_headers.c'),
      c_args: header_suppressions + program_c_args,
      dependencies: zix_dep,
      include_directories: include_dirs,
    ),
    suite: 'build',
  )
endif

if not get_option('tests_cpp').disabled() and add_languages(
  ['cpp'],
  native: false,
  required: get_option('tests_cpp').enabled(),
)
  cpp = meson.get_compiler('cpp')

  cpp_test_args = []
  if cpp.get_id() in ['clang', 'emscripten']
    cpp_test_args = [
      '-Weverything',
      '-Wno-c++98-compat',
      '-Wno-c++98-compat-pedantic',
      '-Wno-nullability-extension',
      '-Wno-padded',
      '-Wno-zero-as-null-pointer-constant',
    ]

    if not meson.is_cross_build()
      cpp_test_args += [
        '-Wno-poison-system-directories',
      ]
    endif

    if host_machine.system() == 'windows'
      cpp_test_args += [
        '-Wno-nonportable-system-include-path',
      ]
    endif

  elif cpp.get_id() == 'gcc'
    cpp_test_args = [
      '-Wall',
      '-Wno-padded',
      '-Wno-unused-const-variable',
    ]

  elif cpp.get_id() == 'msvc'
    cpp_test_args = [
      '/Wall',
      '/experimental:external',
      '/external:W0',
      '/external:anglebrackets',

      '/wd4514', # unreferenced inline function has been removed
      '/wd4710', # function not inlined
      '/wd4711', # function selected for automatic inline expansion
      '/wd4820', # padding added after construct
      '/wd5039', # throwing function passed to C (winbase.h)
      '/wd5262', # implicit fall-through
      '/wd5264', # const variable is not used
    ]
  endif

  test(
    'headers_cpp',
    executable(
      'test_headers_cpp',
      files('cpp/test_headers_cpp.cpp'),
      cpp_args: cpp_test_args + program_c_args,
      dependencies: [zix_dep],
      include_directories: include_dirs,
      link_args: program_link_args,
    ),
    suite: 'build',
  )

  filesystem_code = '''#include <filesystem>
int main(void) { return 0; }'''

  if cpp.links(filesystem_code, name: 'filesystem')
    test(
      'path_std',
      executable(
        'test_path_std',
        files('cpp/test_path_std.cpp'),
        cpp_args: cpp_test_args + program_c_args,
        dependencies: [zix_dep],
        include_directories: include_dirs,
        link_args: program_link_args,
      ),
      suite: 'unit',
    )
  endif
endif