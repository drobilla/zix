#!/usr/bin/env python

import os
import subprocess
import sys

from waflib import Logs, Options
from waflib.extras import autowaf

# Library and package version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
ZIX_VERSION       = '0.0.2'
ZIX_MAJOR_VERSION = '0'

# Mandatory waf variables
APPNAME = 'zix'        # Package name for waf dist
VERSION = ZIX_VERSION  # Package version for waf dist
top     = '.'          # Source directory
out     = 'build'      # Build directory

def options(ctx):
    ctx.load('compiler_c')
    opt = ctx.get_option_group('Configuration options')
    ctx.add_flags(opt, {'bench': 'build benchmarks',
                        'static': 'build static library'})
    opt.add_option('--page-size', type='int', default=4096, dest='page_size',
                   help='Page size for B-tree')

def configure(conf):
    conf.load('compiler_c', cache=True)
    conf.load('autowaf', cache=True)
    autowaf.set_c_lang(conf, 'c99')

    conf.env.update({
        'BUILD_BENCH': Options.options.bench,
        'BUILD_STATIC': Options.options.static})

    if Options.options.ultra_strict:
        autowaf.add_compiler_flags(conf.env, '*', {
            'clang': [
                '-Wno-bad-function-cast',
                '-Wno-cast-align',
                '-Wno-padded',
                '-Wno-reserved-id-macro',
                '-Wno-sign-conversion',
            ],
            'gcc': [
                '-Wno-bad-function-cast',
                '-Wno-cast-align',
                '-Wno-conversion',
                '-Wno-inline',
                '-Wno-null-dereference',
                '-Wno-padded',
                '-Wno-suggest-attribute=const',
                '-Wno-suggest-attribute=pure',
            ],
            'msvc': [
                '/wd4191',  # unsafe function conversion
                '/wd4200',  # zero-sized array in struct/union
                '/wd4365',  # signed/unsigned mismatch
                '/wd4514',  # unreferenced inline function has been removed
                '/wd4706',  # assignment within conditional expression
                '/wd4820',  # padding added after construct
                '/wd5045',  # will insert Spectre mitigation for memory load
            ],
        })

        if 'mingw' in conf.env.CC[0]:
            conf.env.append_unique('CFLAGS', [
                '-Wno-cast-function-type',
                '-Wno-format',
            ])

    # Check for mlock
    conf.check_function('c', 'mlock',
                        header_name='sys/mman.h',
                        return_type='int',
                        arg_types='const void*,size_t',
                        define_name='HAVE_MLOCK',
                        mandatory=False)

    if Options.options.bench:
        conf.check_pkg('glib-2.0 >= 2.0.0',
                       uselib_store='GLIB',
                       system=True,
                       mandatory=False)
        if not conf.is_defined('HAVE_GLIB'):
            conf.fatal('Glib is required to build benchmarks')

    conf.define('ZIX_VERSION', ZIX_VERSION)
    conf.define('ZIX_BTREE_PAGE_SIZE', Options.options.page_size)
    conf.write_config_header('zix-config.h', remove=False)

    autowaf.display_summary(
        conf,
        {'Build unit tests': bool(conf.env.BUILD_TESTS),
         'Build benchmarks': bool(conf.env.BUILD_BENCH),
         'Page size': Options.options.page_size})

sources = [
    'zix/chunk.c',
    'zix/digest.c',
    'zix/hash.c',
    'zix/patree.c',
    'zix/trie.c',
    'zix/ring.c',
    'zix/sorted_array.c',
    'zix/strindex.c',
    'zix/tree.c',
    'zix/btree.c',
    'zix/bitset.c',
    'zix/ampatree.c'
]

tests = [
    'hash_test',
    'inline_test',
    'patree_test',
    'trie_test',
    'ring_test',
    'sem_test',
    'sorted_array_test',
    'strindex_test',
    'tree_test',
    'btree_test',
    'bitset_test',
    'ampatree_test'
]

def build(bld):
    # C Headers
    bld.install_files('${INCLUDEDIR}/zix', bld.path.ant_glob('zix/*.h'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'ZIX', ZIX_VERSION, ZIX_MAJOR_VERSION, [],
                     {'ZIX_MAJOR_VERSION': ZIX_MAJOR_VERSION})

    framework = ''
    if sys.platform == 'darwin':
        framework = ['CoreServices']

    libflags = ['-fvisibility=hidden']
    if bld.env.MSVC_COMPILER:
        libflags = []

    # Library
    bld(features        = 'c cshlib',
        export_includes = ['.'],
        source          = sources,
        includes        = ['.'],
        name            = 'libzix',
        target          = 'zix',
        vnum            = ZIX_VERSION,
        install_path    = '${LIBDIR}',
        framework       = framework,
        cflags          = libflags + ['-DZIX_SHARED', '-DZIX_INTERNAL'])

    if bld.env.BUILD_STATIC or bld.env.BUILD_BENCH:
        bld(features        = 'c cstlib',
            export_includes = ['.'],
            source          = sources,
            includes        = ['.'],
            name            = 'libzix_static',
            target          = 'zix',
            vnum            = ZIX_VERSION,
            install_path    = None,
            framework       = framework,
            cflags          = libflags + ['-DZIX_SHARED', '-DZIX_INTERNAL'])

    if bld.env.BUILD_TESTS:
        test_cflags    = []
        test_linkflags = []
        test_libs      = ['pthread', 'dl']
        if bld.env.MSVC_COMPILER:
            test_libs = []
        elif bld.env.DEST_OS == 'win32':
            test_libs = 'pthread'
        if bld.is_defined('HAVE_GCOV'):
            test_cflags    += ['--coverage']
            test_linkflags += ['--coverage']

        # Profiled static library (for unit test code coverage)
        bld(features     = 'c cstlib',
            source       = sources,
            includes     = ['.'],
            lib          = test_libs,
            name         = 'libzix_profiled',
            target       = 'zix_profiled',
            install_path = '',
            framework    = framework,
            cflags       = test_cflags + ['-DZIX_INTERNAL'],
            linkflags    = test_linkflags)

        if bld.env.DEST_OS == 'win32':
            test_malloc = []
        else:
            test_malloc = ['test/test_malloc.c']

        # Unit test programs
        for i in tests:
            bld(features     = 'c cprogram',
                source       = ['test/%s.c' % i] + test_malloc,
                includes     = ['.'],
                use          = 'libzix_profiled',
                lib          = test_libs,
                target       = 'test/%s' % i,
                install_path = None,
                framework    = framework,
                cflags       = test_cflags,
                linkflags    = test_linkflags)

    if bld.env.BUILD_BENCH:
        # Benchmark programs
        for i in ['tree_bench', 'dict_bench']:
            bld(features     = 'c cprogram',
                source       = 'test/%s.c' % i,
                includes     = ['.'],
                use          = 'libzix_static',
                uselib       = 'GLIB',
                lib          = ['rt'],
                target       = 'test/%s' % i,
                framework    = framework,
                install_path = '')

    # Documentation
    autowaf.build_dox(bld, 'ZIX', ZIX_VERSION, top, out)

    bld.add_post_fun(autowaf.run_ldconfig)

def lint(ctx):
    "checks code for style issues"
    import subprocess
    import glob

    subprocess.call(
        ['flake8', '--ignore', 'E221,W504,E302,E305,E251', 'wscript'])

    files = ['../%s' % x for x in glob.glob('**/*.c')]
    checks = ['*',
              '-clang-analyzer-valist.Uninitialized',
              '-llvm-header-guard',
              '-misc-unused-parameters',
              '-readability-else-after-return']
    subprocess.call(['clang-tidy', '-p=.', '-header-filter=.*',
                     '-checks="%s"' % ','.join(checks)] + files,
                    cwd='build')

def build_dir(ctx, subdir):
    if autowaf.is_child():
        return os.path.join('build', APPNAME, subdir)
    else:
        return os.path.join('build', subdir)

def upload_docs(ctx):
    os.system('rsync -avz --delete -e ssh build/doc/html/*'
              ' drobilla@drobilla.net:~/drobilla.net/docs/zix')

def test(tst):
    with tst.group('unit') as check:
        for test in tests:
            check([os.path.join('test', test)])

def bench(ctx):
    os.chdir('build')

    # Benchmark trees

    subprocess.call(['test/tree_bench', '400000', '6400000'])
    subprocess.call(['../plot.py', 'tree_bench.svg',
                     'tree_insert.txt',
                     'tree_search.txt',
                     'tree_iterate.txt',
                     'tree_delete.txt'])

    # Benchmark dictionaries

    filename = 'gibberish.txt'
    if not os.path.exists(filename):
        Logs.info('Generating random text %s' % filename)
        import random
        out = open(filename, 'w')
        for i in range(1 << 20):
            wordlen = random.randrange(1, 64)
            word    = ''
            for j in range(wordlen):
                word += chr(random.randrange(ord('A'),
                                             min(ord('Z'), ord('A') + j + 1)))
            out.write(word + ' ')
        out.close()

    subprocess.call(['test/dict_bench', 'gibberish.txt'])
    subprocess.call(['../plot.py', 'dict_bench.svg',
                     'dict_insert.txt', 'dict_search.txt'])
