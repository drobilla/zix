# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project('zix', ['c'],
        version: '0.3.0',
        license: 'ISC',
        meson_version: '>= 0.56.0',
        default_options: [
          'b_ndebug=if-release',
          'buildtype=release',
          'c_std=c99',
        ])

major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'zix' + version_suffix

#######################
# Compilers and Flags #
#######################

# Required tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# Set global warning flags
if get_option('strict') and not meson.is_subproject()
  subdir('meson/warnings')
  add_project_arguments(all_c_warnings, language: ['c'])
endif
subdir('meson/suppressions')

##########################
# Platform Configuration #
##########################

thread_dep = dependency('threads', required: get_option('tests'))

platform_c_args = []

# Determine whether to use POSIX
no_posix = get_option('posix').disabled() or host_machine.system() == 'windows'
if no_posix
  platform_c_args += ['-DZIX_NO_POSIX']
else
  platform_c_args += ['-D_POSIX_C_SOURCE=200809L']
endif

# Check for platform features with the build system
if get_option('checks')
  clock_gettime_code = '''#include <time.h>
int main(void) { struct timespec t; return clock_gettime(CLOCK_MONOTONIC, &t); }
'''

  mlock_code = '''#include <sys/mman.h>
int main(void) { return mlock(0, 0); }'''

  posix_memalign_code = '''#include <stdlib.h>
int main(void) { void* mem; posix_memalign(&mem, 8, 8); }'''

  sem_timedwait_code = '''#include <semaphore.h>
#include <time.h>
int main(void) { sem_t s; struct timespec t; return sem_timedwait(&s, &t); }'''

  platform_c_args += [
    '-DZIX_NO_DEFAULT_CONFIG',
    '-DHAVE_CLOCK_GETTIME=@0@'.format(
      cc.links(clock_gettime_code,
               args: platform_c_args,
               name: 'clock_gettime').to_int()),
    '-DHAVE_MLOCK=@0@'.format(
      cc.links(mlock_code,
               args: platform_c_args,
               name: 'mlock').to_int()),
    '-DHAVE_POSIX_MEMALIGN=@0@'.format(
      cc.links(posix_memalign_code,
               args: platform_c_args,
               name: 'posix_memalign').to_int()),
    '-DHAVE_SEM_TIMEDWAIT=@0@'.format(
      (host_machine.system() not in ['darwin', 'windows'] and
       cc.links(sem_timedwait_code,
                args: platform_c_args,
                dependencies: [thread_dep],
                name: 'sem_timedwait')).to_int()),
  ]
endif

###########
# Library #
###########

include_dirs = include_directories(['include'])

c_headers = files(
  'include/zix/allocator.h',
  'include/zix/attributes.h',
  'include/zix/bitset.h',
  'include/zix/btree.h',
  'include/zix/bump_allocator.h',
  'include/zix/digest.h',
  'include/zix/function_types.h',
  'include/zix/hash.h',
  'include/zix/ring.h',
  'include/zix/sem.h',
  'include/zix/status.h',
  'include/zix/string_view.h',
  'include/zix/thread.h',
  'include/zix/tree.h',
  'include/zix/zix.h',
)

sources = files(
  'src/allocator.c',
  'src/bitset.c',
  'src/btree.c',
  'src/bump_allocator.c',
  'src/digest.c',
  'src/errno_status.c',
  'src/hash.c',
  'src/ring.c',
  'src/status.c',
  'src/string_view.c',
  'src/tree.c',
)

if thread_dep.found()
  if host_machine.system() == 'darwin'
    sources += files(
      'src/darwin/sem_darwin.c',
      'src/posix/thread_posix.c',
    )

  elif host_machine.system() == 'windows'
    sources += files(
      'src/win32/sem_win32.c',
      'src/win32/thread_win32.c',
    )
  else
    sources += files(
      'src/posix/sem_posix.c',
      'src/posix/thread_posix.c',
    )
  endif
endif

# Set appropriate arguments for building against the library type
subdir('meson/library')
extra_c_args = []
if get_option('default_library') == 'static'
  extra_c_args = ['-DZIX_STATIC']
endif

# Set any additional arguments required for building libraries or programs
library_c_args = platform_c_args + extra_c_args + ['-DZIX_INTERNAL']
library_link_args = []
program_c_args = platform_c_args + extra_c_args
program_link_args = []
if cc.get_id() == 'emscripten'
  wasm_c_args = [
    '-matomics',
    '-mbulk-memory',
    '-pthread',
  ]

  wasm_link_args = [
    '-matomics',
    '-mbulk-memory',
    '-pthread',
    ['-s', 'ENVIRONMENT=node,worker'],
  ]

  library_c_args += wasm_c_args
  program_c_args += wasm_c_args
  library_link_args += wasm_link_args
  program_link_args += wasm_link_args
  program_link_args += [
    ['-s', 'EXIT_RUNTIME'],
    ['-s', 'PROXY_TO_PTHREAD'],
  ]
endif

# Build shared and/or static library
libzix = library(
  meson.project_name() + library_suffix,
  sources,
  c_args: c_suppressions + library_c_args,
  dependencies: [thread_dep],
  gnu_symbol_visibility: 'hidden',
  include_directories: include_dirs,
  install: true,
  link_args: library_link_args,
  version: meson.project_version(),
)

# Declare dependency for internal meson dependants
zix_dep = declare_dependency(
  compile_args: extra_c_args,
  include_directories: include_dirs,
  link_with: libzix,
)

# Generage pkg-config file for external dependants
pkg.generate(
  libzix,
  description: 'Lightweight C library of portability wrappers and data structures',
  extra_cflags: extra_c_args,
  filebase: versioned_name,
  name: 'Zix',
  requires: [thread_dep],
  subdirs: [versioned_name],
  version: meson.project_version(),
)

# Override pkg-config dependency for internal meson dependants
meson.override_dependency(versioned_name, zix_dep)

# Install headers to a versioned include directory
install_headers(c_headers, subdir: versioned_name / 'zix')

#########
# Tests #
#########

sequential_tests = [
  'test_allocator',
  'test_bitset',
  'test_btree',
  'test_digest',
  'test_hash',
  'test_status',
  'test_tree',
]

threaded_tests = [
  'test_ring',
  'test_sem',
  'test_thread',
]

if not get_option('tests').disabled()
  if not meson.is_subproject() and get_option('strict')
    # Check release metadata
    autoship = find_program('autoship', required: get_option('tests'))
    if autoship.found()
      test('autoship', autoship,
           args: ['test', meson.current_source_dir()],
           suite: 'data')
    endif

    # Check licensing metadata
    reuse = find_program('reuse', required: get_option('tests'))
    if reuse.found()
      test(
        'REUSE',
        reuse,
        args: ['--root', meson.current_source_dir(), 'lint'],
        suite: 'data',
      )
    endif
  endif

  common_test_sources = files('test/failing_allocator.c')

  foreach test : sequential_tests
    sources = common_test_sources + files('test/@0@.c'.format(test))

    test(
      test,
      executable(
        test,
        sources,
        c_args: c_suppressions + program_c_args,
        dependencies: [zix_dep],
        include_directories: include_dirs,
        link_args: program_link_args,
      ),
      timeout: 120,
    )
  endforeach

  if thread_dep.found()
    foreach test : threaded_tests
      sources = common_test_sources + files('test/@0@.c'.format(test))

      test(
        test,
        executable(
          test,
          sources,
          c_args: c_suppressions + program_c_args,
          dependencies: [zix_dep, thread_dep],
          include_directories: include_dirs,
          link_args: program_link_args,
        ),
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
    elif cc.get_id() == 'gcc'
      header_suppressions += [
        '-Wno-padded',
        '-Wno-unused-const-variable',
      ]
    elif cc.get_id() == 'msvc'
      header_suppressions += [
        '/wd4820', # padding added after construct
      ]
    endif

    test(
      'test_headers',
      executable(
        'test_headers',
        files('test/headers/test_headers.c'),
        c_args: header_suppressions + program_c_args,
        dependencies: zix_dep,
        include_directories: include_dirs,
      ),
    )
  endif
endif

##############
# Benchmarks #
##############

benchmarks = [
  'dict_bench',
  'tree_bench',
]

build_benchmarks = false
if not get_option('benchmarks').disabled()
  glib_dep = dependency('glib-2.0',
                        required: get_option('benchmarks'),
                        version: '>= 2.0.0',
                        include_type: 'system')

  if glib_dep.found()
    build_benchmarks = true
    benchmark_c_args = platform_c_args

    if cc.get_id() == 'clang'
      benchmark_c_suppressions = [
        '-Wno-reserved-identifier',
      ]

      benchmark_c_args += cc.get_supported_arguments(benchmark_c_suppressions)
    endif

    foreach benchmark : benchmarks
      benchmark(
        benchmark,
        executable(
          benchmark,
          'benchmark/@0@.c'.format(benchmark),
          include_directories: include_dirs,
          c_args: c_suppressions + benchmark_c_args,
          dependencies: [zix_dep, glib_dep]),
      )
    endforeach
  endif
endif

subdir('scripts')

if not meson.is_subproject()
  summary('Benchmarks', build_benchmarks, bool_yn: true)
  summary('Tests', not get_option('tests').disabled(), bool_yn: true)

  summary('Install prefix', get_option('prefix'))
  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))
endif
