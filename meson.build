# Copyright 2020-2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

project('zix', ['c'],
        version: '0.0.3',
        license: 'ISC',
        meson_version: '>= 0.49.0',
        default_options: [
          'b_ndebug=if-release',
          'buildtype=release',
          'c_std=c99',
          'default_library=shared',
          'warning_level=3',
        ])

major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'zix' + version_suffix

#
# General Setup
#

# Load build tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# Suppress unavoidable present even in default build
c_warnings = []
c_suppressions = []
if cc.get_id() == 'clang'
  c_warnings = [
    '-Wno-nullability-extension',
  ]
elif cc.get_id() == 'msvc'
  c_warnings += [
    '/wd4706',  # assignment within conditional expression
  ]
endif

# Set ultra strict warnings for developers, if requested
if get_option('strict')
  if cc.get_id() == 'clang'
    c_warnings += [
      '-Weverything',
      '-Wno-bad-function-cast',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
      '-Wno-implicit-fallthrough', # Really for clang < 12
    ]
  elif cc.get_id() == 'gcc'
    c_warnings += [
      # '-Waggregate-return',
      '-Walloc-size-larger-than=16384',
      '-Walloc-zero',
      '-Walloca',
      '-Wanalyzer-too-complex',
      '-Warith-conversion',
      '-Warray-bounds=2',
      '-Wattribute-alias=2',
      '-Wcast-align=strict',
      '-Wcast-qual',
      '-Wconversion',
      '-Wdate-time',
      '-Wdisabled-optimization',
      '-Wdouble-promotion',
      '-Wduplicated-branches',
      '-Wduplicated-cond',
      '-Wfloat-equal',
      '-Wformat-overflow=2',
      '-Wformat-signedness',
      '-Wformat-truncation=2',
      '-Wformat=2',
      '-Wframe-larger-than=2048',
      '-Wimplicit-fallthrough=2',
      '-Winit-self',
      # '-Winline',
      '-Winvalid-pch',
      # '-Wlarger-than=',
      '-Wlogical-op',
      '-Wmissing-declarations',
      '-Wmissing-include-dirs',
      '-Wmultichar',
      '-Wnormalized=nfc',
      '-Wnull-dereference',
      '-Wpacked',
      # '-Wpadded',
      '-Wredundant-decls',
      '-Wscalar-storage-order',
      '-Wshadow',
      '-Wshift-overflow=2',
      '-Wsizeof-array-argument',
      '-Wstack-protector',
      '-Wstack-usage=2048',
      '-Wstrict-aliasing=3',
      # '-Wstrict-overflow=5',
      '-Wstringop-overflow=3',
      '-Wsuggest-attribute=cold',
      '-Wsuggest-attribute=const',
      '-Wsuggest-attribute=format',
      '-Wsuggest-attribute=malloc',
      '-Wsuggest-attribute=noreturn',
      '-Wsuggest-attribute=pure',
      # '-Wswitch-default',
      '-Wswitch-enum',
      '-Wsync-nand',
      # '-Wsystem-headers',
      # '-Wtraditional',
      # '-Wtraditional-conversion',
      '-Wtrampolines',
      '-Wundef',
      '-Wunused-const-variable=2',
      '-Wunused-macros',
      '-Wvarargs',
      '-Wvector-operation-performance',
      '-Wvla',
      '-Wwrite-strings',
    ]
  elif cc.get_id() == 'msvc'
    c_warnings += [
      '/Wall',    # everything, except...
      '/wd4191',  # unsafe function conversion
      '/wd4200',  # zero-sized array in struct/union
      '/wd4365',  # signed/unsigned mismatch
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4710',  # function not inlined
      '/wd4711',  # function selected for automatic inline expansion
      '/wd4777',  # format string and argument mismatch
      '/wd4800',  # implicit conversion to bool
      '/wd4820',  # padding added after construct
      '/wd5045',  # will insert Spectre mitigation for memory load
    ]
  endif

  if cc.get_id() == 'gcc' and host_machine.system() == 'windows'
    c_warnings += [
      '-Wno-cast-function-type',
      '-Wno-format',
    ]
  endif

endif

add_project_arguments(cc.get_supported_arguments(c_warnings), language: ['c'])

if cc.get_id() == 'msvc'
  # Build as C++
  add_project_arguments(['/TP'], language: ['c'])

  # Suppress warnings in system headers
  add_project_arguments(['/experimental:external',
                         '/external:W0',
                         '/external:anglebrackets'],
                        language: ['c'])
endif

# Determine library type and the flags needed to build it
if get_option('default_library') == 'both'
  if host_machine.system() == 'windows'
    error('default_library=both is not supported on Windows')
  endif

  library_type = 'both_libraries'
  library_args = ['-DZIX_INTERNAL']
elif get_option('default_library') == 'shared'
  library_type = 'shared_library'
  library_args = ['-DZIX_INTERNAL']
else
  library_type = 'static_library'
  library_args = ['-DZIX_INTERNAL', '-DZIX_STATIC']
endif

#
# System Checks and Dependencies
#

# Check for mlock() (used by ZixRing)
mlock_fragment = '''#include <sys/mman.h>
                     int main(void) { return mlock(0, 0); }'''
if cc.compiles(mlock_fragment, name: 'mlock')
  add_project_arguments(['-DHAVE_MLOCK'], language: 'c')
endif

#
# Library
#

sources = [
  'src/allocator.c',
  'src/bitset.c',
  'src/btree.c',
  'src/bump_allocator.c',
  'src/digest.c',
  'src/hash.c',
  'src/ring.c',
  'src/tree.c',
]

# Build shared and/or static library/libraries
libzix = build_target(
  versioned_name,
  sources,
  version: meson.project_version(),
  include_directories: include_directories(['include']),
  c_args: library_args,
  gnu_symbol_visibility: 'hidden',
  install: true,
  target_type: library_type)

zix_dep = declare_dependency(
  link_with: libzix,
  include_directories: include_directories(['include']))

#
# Installation
#

# Generage pkg-config file
pkg.generate(
  libzix,
  name: 'Zix',
  filebase: versioned_name,
  subdirs: [versioned_name],
  version: meson.project_version(),
  description: 'A lightweight portability and data structure C library')

headers = [
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
]

# Install headers to a versioned include directory
install_headers(headers, subdir: versioned_name / 'zix')

#
# Tests
#

tests = [
  'allocator_test',
  'bitset_test',
  'btree_test',
  'digest_test',
  'hash_test',
  'ring_test',
  'sem_test',
  'strerror_test',
  'tree_test',
]

# Build and run tests
if get_option('tests')
  # Check licensing metadata
  reuse = find_program('reuse', required: false)
  if reuse.found()
    test('REUSE', reuse,
         args: ['--root', meson.current_source_dir(), 'lint'],
         suite: 'data')
  endif

  thread_dep = dependency('threads')
  dl_dep = cc.find_library('dl', required: false)

  foreach test : tests
    c_args = []
    sources = files('test/@0@.c'.format(test), 'test/failing_allocator.c')

    test(test,
         executable(test,
                    sources,
                    include_directories: include_directories(['include']),
                    c_args: c_args,
                    dependencies: [dl_dep, zix_dep, thread_dep]),
         timeout: 120)
  endforeach
endif

#
# Benchmarks
#

benchmarks = [
  'dict_bench',
  'tree_bench',
]

build_benchmarks = false
if not get_option('benchmarks').disabled()
  glib_dep = dependency('glib-2.0',
                        version: '>= 2.0.0',
                        required: get_option('benchmarks'))

  build_benchmarks = glib_dep.found()

  if build_benchmarks
    foreach benchmark : benchmarks
      benchmark(benchmark,
                executable(benchmark,
                           'benchmark/@0@.c'.format(benchmark),
                           include_directories: include_directories(['include']),
                           dependencies: [zix_dep, glib_dep]))
    endforeach
  else
    warning('Not building benchmarks because glib was not found')
  endif
endif

if not meson.is_subproject() and meson.version().version_compare('>=0.53.0')
  summary('Benchmarks', build_benchmarks, bool_yn: true)
  summary('Tests', get_option('tests'), bool_yn: true)

  summary('Install prefix', get_option('prefix'))
  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))
endif
