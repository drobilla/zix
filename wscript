#!/usr/bin/env python

import os
import subprocess
import sys

from waflib import Build, Logs, Options
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
    opt.add_option('--no-test-malloc', action='store_true',
                   dest='no_test_malloc',
                   help='Do not use test malloc implementation')


def configure(conf):
    conf.load('compiler_c', cache=True)
    conf.load('autowaf', cache=True)
    autowaf.set_c_lang(conf, 'c99')

    conf.env.update({
        'BUILD_BENCH': Options.options.bench,
        'BUILD_STATIC': Options.options.static})

    if Options.options.strict:
        # Check for programs used by lint target
        conf.find_program("flake8", var="FLAKE8", mandatory=False)
        conf.find_program("clang-tidy", var="CLANG_TIDY", mandatory=False)
        conf.find_program("iwyu_tool", var="IWYU_TOOL", mandatory=False)

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
                '-Wno-padded',
            ],
            'msvc': [
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

    if not (conf.env.DEST_OS == 'win32' or Options.options.no_test_malloc):
        conf.env['ZIX_WITH_TEST_MALLOC'] = True
    else:
        conf.env['ZIX_WITH_TEST_MALLOC'] = False

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
    'digest_test',
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

        if bld.env.ZIX_WITH_TEST_MALLOC:
            test_malloc = ['test/test_malloc.c']
            test_cflags += ['-DZIX_WITH_TEST_MALLOC']
        else:
            test_malloc = []

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
                source       = 'benchmark/%s.c' % i,
                includes     = ['.'],
                use          = 'libzix_static',
                uselib       = 'GLIB',
                lib          = ['rt'],
                target       = 'benchmark/%s' % i,
                framework    = framework,
                install_path = '')

    # Documentation
    autowaf.build_dox(bld, 'ZIX', ZIX_VERSION, top, out)

    bld.add_post_fun(autowaf.run_ldconfig)


class LintContext(Build.BuildContext):
    fun = cmd = 'lint'


def lint(ctx):
    "checks code for style issues"

    import glob
    import subprocess

    st = 0

    if "FLAKE8" in ctx.env:
        Logs.info("Running flake8")
        st = subprocess.call([ctx.env.FLAKE8[0],
                              "wscript",
                              "--ignore",
                              "E101,E129,W191,E221,W504,E251,E241,E741"])
    else:
        Logs.warn("Not running flake8")

    if "IWYU_TOOL" in ctx.env:
        Logs.info("Running include-what-you-use")
        cmd = [ctx.env.IWYU_TOOL[0], "-o", "clang", "-p", "build"]
        output = subprocess.check_output(cmd).decode('utf-8')
        if 'error: ' in output:
            sys.stdout.write(output)
            st += 1
    else:
        Logs.warn("Not running include-what-you-use")

    if "CLANG_TIDY" in ctx.env and "clang" in ctx.env.CC[0]:
        Logs.info("Running clang-tidy")
        sources = glob.glob('zix/*.c') + glob.glob('test/*.c*')
        sources = list(map(os.path.abspath, sources))
        procs = []
        for source in sources:
            cmd = [ctx.env.CLANG_TIDY[0], "--quiet", "-p=.", source]
            procs += [subprocess.Popen(cmd, cwd="build")]

        for proc in procs:
            stdout, stderr = proc.communicate()
            st += proc.returncode
    else:
        Logs.warn("Not running clang-tidy")

    if st != 0:
        sys.exit(st)


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

    subprocess.call(['benchmark/tree_bench', '400000', '6400000'])
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

    subprocess.call(['benchmark/dict_bench', 'gibberish.txt'])
    subprocess.call(['../plot.py', 'dict_bench.svg',
                     'dict_insert.txt', 'dict_search.txt'])
