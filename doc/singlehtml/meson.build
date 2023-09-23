# Copyright 2021-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

singlehtml_dir = docdir / versioned_name / 'singlehtml'

custom_target(
  'singlehtml',
  build_by_default: true,
  command: [
    sphinx_build,
    '-M', 'singlehtml',
    '@OUTDIR@' / '..',
    '@OUTDIR@' / '..',
    '-t', 'singlehtml',
  ] + sphinx_flags,
  input: [api_zix_rst, conf_py, sphinx_input],
  install: true,
  install_dir: singlehtml_dir,
  output: [
    'index.html',

    '_static',
  ],
)

if not meson.is_subproject()
  summary(
    'Unified HTML',
    get_option('prefix') / singlehtml_dir,
    section: 'Directories',
  )
endif
