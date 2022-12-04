# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

docdir = get_option('datadir') / 'doc'

doxygen = find_program('doxygen', required: get_option('docs'))
sphinxygen = find_program('sphinxygen', required: false)
sphinx_build = find_program('sphinx-build', required: get_option('docs'))

if not sphinxygen.found()
  subproject('sphinxygen')
  sphinxygen = find_program('sphinxygen', required: get_option('docs'))
endif

build_docs = doxygen.found() and sphinxygen.found() and sphinx_build.found()
if build_docs

  # Documentation Code

  if not get_option('tests').disabled()
    test(
      'overview_code',
      executable(
        'overview_code',
        files('overview_code.c'),
        dependencies: [zix_dep],
        c_args: c_suppressions,
      ),
      suite: 'doc',
    )
  endif

  # Generated API Reference

  c_rst_files = files(
    'allocation.rst',
    'error_handling.rst',
    'index.rst',
    'string_views.rst',
    'summary.rst',
    'using_zix.rst',
  )

  c_doc_files = c_rst_files + files(
    'overview_code.c',
  )

  foreach f : c_doc_files
    configure_file(copy: true, input: f, output: '@PLAINNAME@')
  endforeach

  config = configuration_data()
  config.set('ZIX_VERSION', meson.project_version())
  config.set('ZIX_SRCDIR', zix_src_root)
  config.set('ZIX_TITLE', get_option('title'))

  conf_py = configure_file(
    configuration: config,
    input: files('conf.py.in'),
    output: 'conf.py',
  )

  subdir('xml')
  subdir('api')

  sphinx_options = [
    '-D', 'release=@0@'.format(meson.project_version()),
    '-E',
    '-W',
    '-a',
    '-q',
  ]

  doc_inputs = c_rst_files + [conf_py, c_zix_rst, c_index_xml]

  custom_target(
    'singlehtml',
    build_by_default: true,
    command: [sphinx_build, '-M', 'singlehtml', '@OUTDIR@', '@OUTDIR@',
              '-t', 'singlehtml'] + sphinx_options,
    input: doc_inputs,
    install: true,
    install_dir: docdir / versioned_name,
    output: 'singlehtml',
  )

  custom_target(
    'html',
    build_by_default: true,
    command: [sphinx_build, '-M', 'html', '@OUTDIR@', '@OUTDIR@',
              '-t', 'html'] + sphinx_options,
    input: doc_inputs,
    install: true,
    install_dir: docdir / versioned_name,
    output: 'html',
  )

endif
