#!/usr/bin/env python
import filecmp
import glob
import os
import shutil
import subprocess

from waflib.extras import autowaf as autowaf
import waflib.Logs as Logs, waflib.Options as Options

# Version of this package (even if built as a child)
ZIX_VERSION = '0.0.2'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Zix uses the same version number for both library and package
ZIX_LIB_VERSION = ZIX_VERSION

# Variables for 'waf dist'
APPNAME = 'zix'
VERSION = ZIX_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    autowaf.set_options(opt)
    opt.load('compiler_c')
    opt.add_option('--test', action='store_true', default=False, dest='build_tests',
                   help="Build unit tests")
    opt.add_option('--bench', action='store_true', default=False, dest='build_bench',
                   help="Build benchmarks")

def configure(conf):
    conf.line_just = max(conf.line_just, 59)
    autowaf.configure(conf)
    autowaf.display_header('Zix Configuration')

    conf.load('compiler_c')
    conf.env.append_value('CFLAGS', '-std=c99')

    autowaf.check_pkg(conf, 'glib-2.0', uselib_store='GLIB',
                      atleast_version='2.0.0', mandatory=False)

    conf.env['BUILD_TESTS'] = Options.options.build_tests
    if conf.is_defined('HAVE_GLIB'):
        conf.env['BUILD_BENCH'] = Options.options.build_bench

    autowaf.define(conf, 'ZIX_VERSION', ZIX_VERSION)
    conf.write_config_header('zix-config.h', remove=False)

    autowaf.display_msg(conf, "Unit tests", str(conf.env['BUILD_TESTS']))
    autowaf.display_msg(conf, "Benchmarks", str(conf.env['BUILD_BENCHx']))
    print('')

def build(bld):
    # C Headers
    bld.install_files('${INCLUDEDIR}/zix', bld.path.ant_glob('zix/*.h'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'ZIX', ZIX_VERSION, [])

    lib_source = '''
        src/tree.c
        src/sorted_array.c
    '''

    # Library
    obj = bld(features = 'c cshlib')
    obj.export_includes = ['.']
    obj.source          = lib_source
    obj.includes        = ['.', './src']
    obj.name            = 'libzix'
    obj.target          = 'zix'
    obj.vnum            = ZIX_LIB_VERSION
    obj.install_path    = '${LIBDIR}'
    obj.cflags          = [ '-fvisibility=hidden', '-DZIX_SHARED', '-DZIX_INTERNAL' ]

    if bld.env['BUILD_TESTS']:
        # Static library (for unit test code coverage)
        obj = bld(features = 'c cstlib')
        obj.source       = lib_source
        obj.includes     = ['.', './src']
        obj.name         = 'libzix_static'
        obj.target       = 'zix_static'
        obj.install_path = ''
        obj.cflags       = [ '-fprofile-arcs',  '-ftest-coverage' ]

        # Unit test programs
        for i in ['tree_test', 'sorted_array_test']:
            obj = bld(features = 'c cprogram')
            obj.source       = 'test/%s.c' % i
            obj.includes     = ['.']
            obj.use          = 'libzix_static'
            obj.linkflags    = '-lgcov'
            obj.target       = 'test/%s' % i
            obj.install_path = ''
            obj.cflags       = [ '-fprofile-arcs',  '-ftest-coverage' ]

    if bld.env['BUILD_BENCH']:
        # Benchmark programs
        for i in ['tree_bench']:
            obj = bld(features = 'c cprogram')
            obj.source       = 'test/%s.c' % i
            obj.includes     = ['.']
            obj.use          = 'libzix'
            obj.uselib       = 'GLIB'
            obj.linkflags    = '-lrt'
            obj.target       = 'test/%s' % i
            obj.install_path = ''

    # Documentation
    autowaf.build_dox(bld, 'ZIX', ZIX_VERSION, top, out)

    bld.add_post_fun(autowaf.run_ldconfig)
    if bld.env['DOCS']:
        bld.add_post_fun(fix_docs)

def lint(ctx):
    subprocess.call('cpplint.py --filter=-whitespace/tab,-whitespace/braces,-build/header_guard,-readability/casting,-readability/todo src/* zix/*', shell=True)

def build_dir(ctx, subdir):
    if autowaf.is_child():
        return os.path.join('build', APPNAME, subdir)
    else:
        return os.path.join('build', subdir)

def fix_docs(ctx):
    try:
        os.chdir('build/doc/html')
        os.system("sed -i 's/ZIX_API //' group__zix.html")
        os.system("sed -i 's/ZIX_DEPRECATED //' group__zix.html")
        os.remove('index.html')
        os.symlink('group__zix.html',
                   'index.html')
    except:
        Logs.error("Failed to fix up %s documentation" % APPNAME)

def fix_docs(ctx):
    try:
        top = os.getcwd()
        os.chdir(build_dir(ctx, 'doc/html'))
        os.system("sed -i 's/ZIX_API //' group__zix.html")
        os.remove('index.html')
        os.symlink('group__zix.html',
                   'index.html')
        os.chdir(top)
    except Exception as e:
        Logs.error("Failed to fix up %s documentation (%s)" % (APPNAME, e))

def upload_docs(ctx):
    os.system("rsync -avz --delete -e ssh build/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/zix")

def test(ctx):
    autowaf.pre_test(ctx, APPNAME)
    for i in ['tree_test', 'sorted_array_test']:
        autowaf.run_tests(ctx, APPNAME, ['test/%s' % i], dirs=['./src','./test'])
    autowaf.post_test(ctx, APPNAME)
