# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

project('zix', ['c'],
        version: '0.0.3',
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
add_project_arguments(c_suppressions, language: ['c'])

##########################
# Platform Configuration #
##########################

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
  platform_c_args += ['-DZIX_NO_DEFAULT_CONFIG']

  mlock_code = '''#include <sys/mman.h>
int main(void) { return mlock(0, 0); }'''

  posix_memalign_code = '''#include <stdlib.h>
int main(void) { void* mem; posix_memalign(&mem, 8, 8); }'''

  platform_c_args += '-DHAVE_MLOCK=@0@'.format(
    cc.compiles(mlock_code,
                args: platform_c_args,
                name: 'mlock').to_int())

  platform_c_args += '-DHAVE_POSIX_MEMALIGN=@0@'.format(
    cc.compiles(posix_memalign_code,
                args: platform_c_args,
                name: 'posix_memalign').to_int())
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
  'include/zix/common.h',
  'include/zix/digest.h',
  'include/zix/hash.h',
  'include/zix/ring.h',
  'include/zix/sem.h',
  'include/zix/thread.h',
  'include/zix/tree.h',
)

sources = files(
  'src/allocator.c',
  'src/bitset.c',
  'src/btree.c',
  'src/bump_allocator.c',
  'src/digest.c',
  'src/hash.c',
  'src/ring.c',
  'src/status.c',
  'src/tree.c',
)

# Set appropriate arguments for building against the library type
subdir('meson/library')
if get_option('default_library') == 'static'
  add_project_arguments(['-DZIX_STATIC'], language: ['c'])
endif

# Set any additional arguments required for building libraries or programs
library_c_args = platform_c_args + ['-DZIX_INTERNAL']
library_link_args = []
program_c_args = []
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
  c_args: library_c_args,
  gnu_symbol_visibility: 'hidden',
  include_directories: include_dirs,
  install: true,
  link_args: library_link_args,
  version: meson.project_version())

# Declare dependency for internal meson dependants
zix_dep = declare_dependency(
  include_directories: include_dirs,
  link_with: libzix)

# Generage pkg-config file for external dependants
pkg.generate(
  libzix,
  description: 'A lightweight portability and data structure C library',
  filebase: versioned_name,
  name: 'Zix',
  subdirs: [versioned_name],
  version: meson.project_version())

# Install headers to a versioned include directory
install_headers(c_headers, subdir: versioned_name / 'zix')

#########
# Tests #
#########

sequential_tests = [
  'allocator_test',
  'bitset_test',
  'btree_test',
  'digest_test',
  'hash_test',
  'strerror_test',
  'tree_test',
]

threaded_tests = [
  'ring_test',
  'sem_test',
]

if not get_option('tests').disabled()
  # Check licensing metadata
  reuse = find_program('reuse', required: get_option('tests'))
  if reuse.found()
    test('REUSE',
         reuse,
         args: ['--root', meson.current_source_dir(), 'lint'],
         suite: 'data')
  endif

  common_test_sources = files('test/failing_allocator.c')

  foreach test : sequential_tests
    sources = common_test_sources + files('test/@0@.c'.format(test))

    test(test,
         executable(test,
                    sources,
                    c_args: program_c_args,
                    dependencies: [zix_dep],
                    include_directories: include_dirs,
                    link_args: program_link_args),
         timeout: 120)
  endforeach

  thread_dep = dependency('threads', required: get_option('tests'))
  if thread_dep.found()
    foreach test : threaded_tests
      sources = common_test_sources + files('test/@0@.c'.format(test))

      test(test,
           executable(test,
                      sources,
                      c_args: program_c_args,
                      dependencies: [zix_dep, thread_dep],
                      include_directories: include_dirs,
                      link_args: program_link_args),
           timeout: 120)
    endforeach
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
                        version: '>= 2.0.0')

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
      benchmark(benchmark,
                executable(benchmark,
                           'benchmark/@0@.c'.format(benchmark),
                           include_directories: include_dirs,
                           c_args: benchmark_c_args,
                           dependencies: [zix_dep, glib_dep]))
    endforeach
  endif
endif

if not meson.is_subproject()
  summary('Benchmarks', build_benchmarks, bool_yn: true)
  summary('Tests', not get_option('tests').disabled(), bool_yn: true)

  summary('Install prefix', get_option('prefix'))
  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))
endif
