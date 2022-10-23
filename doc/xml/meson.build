# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('ZIX_SRCDIR', zix_src_root)
config.set('DOX_OUTPUT', meson.current_build_dir() / '..')

c_doxyfile = configure_file(
  configuration: config,
  input: files('../Doxyfile.in'),
  output: 'Doxyfile',
)

c_index_xml = custom_target(
  'index.xml',
  command: [doxygen, '@INPUT0@'],
  input: [c_doxyfile] + c_headers,
  output: 'index.xml',
)
