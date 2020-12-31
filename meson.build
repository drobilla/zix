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

# Set ultra strict warnings for developers, if requested
if get_option('strict')
  if cc.get_id() == 'clang'
    c_warnings = [
      '-Weverything',
      '-Wno-bad-function-cast',
      '-Wno-conversion',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
    ]
  elif cc.get_id() == 'gcc'
    c_warnings = [
      # '-Waggregate-return',
      '-Walloc-size-larger-than=4096',
      '-Walloc-zero',
      '-Walloca',
      '-Wanalyzer-too-complex',
      '-Warith-conversion',
      '-Warray-bounds=2',
      '-Wattribute-alias=2',
      '-Wcast-align=strict',
      '-Wcast-qual',
      # '-Wconversion',
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
      '-Wframe-larger-than=512',
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
      '-Wstack-usage=512',
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
    c_warnings = [
      '/Wall',    # everything, except...
      '/wd4191',  # unsafe function conversion
      '/wd4200',  # zero-sized array in struct/union
      '/wd4365',  # signed/unsigned mismatch
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4706',  # assignment within conditional expression
      '/wd4710',  # function not inlined
      '/wd4711',  # function selected for automatic inline expansion
      '/wd4777',  # format string and argument mismatch
      '/wd4820',  # padding added after construct
      '/wd5045',  # will insert Spectre mitigation for memory load
    ]
  else
    c_warnings = []
  endif

  if cc.get_id() == 'gcc' and host_machine.system() == 'windows'
    c_warnings += [
      '-Wno-cast-function-type',
      '-Wno-format',
    ]
  endif

  add_project_arguments(cc.get_supported_arguments(c_warnings), language: ['c'])
endif

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
  'src/bitset.c',
  'src/btree.c',
  'src/digest.c',
  'src/hash.c',
  'src/ring.c',
  'src/sorted_array.c',
  'src/strindex.c',
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

libzix_dep = declare_dependency(link_with: libzix)

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
  'include/zix/bitset.h',
  'include/zix/btree.h',
  'include/zix/common.h',
  'include/zix/digest.h',
  'include/zix/hash.h',
  'include/zix/ring.h',
  'include/zix/sem.h',
  'include/zix/sorted_array.h',
  'include/zix/strindex.h',
  'include/zix/thread.h',
  'include/zix/tree.h',
]

# Install headers to a versioned include directory
install_headers(headers, subdir: versioned_name / 'zix')

#
# Tests
#

tests = [
  'bitset_test',
  'btree_test',
  'digest_test',
  'hash_test',
  'ring_test',
  'sem_test',
  'sorted_array_test',
  'strindex_test',
  'tree_test',
]

# Build and run tests
if get_option('tests')
  thread_dep = dependency('threads')
  dl_dep = cc.find_library('dl', required: false)

  foreach test : tests
    c_args = []
    sources = ['test/@0@.c'.format(test)]

    if get_option('test_malloc') and host_machine.system() != 'windows'
      sources += ['test/test_malloc.c']
      c_args += ['-DZIX_WITH_TEST_MALLOC']
    endif

    test(test,
         executable(test,
                    sources,
                    include_directories: include_directories(['include']),
                    c_args: c_args,
                    dependencies: [dl_dep, libzix_dep, thread_dep]),
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

# Build benchmarks
if not get_option('benchmarks').disabled()
  glib_dep = dependency('glib-2.0',
                        version: '>= 2.0.0',
                        required: get_option('benchmarks'))

  if glib_dep.found()
    foreach benchmark : benchmarks
      benchmark(benchmark,
                executable(benchmark,
                           'benchmark/@0@.c'.format(benchmark),
                           include_directories: include_directories(['include']),
                           dependencies: [libzix_dep, glib_dep]))
    endforeach
  else
    warning('Not building benchmarks because glib was not found')
  endif
endif