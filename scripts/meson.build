# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

if get_option('strict') and not meson.is_subproject()
  flake8 = find_program('flake8', required: get_option('tests'))
  pylint = find_program('pylint', required: get_option('tests'))
  black = find_program('black', required: get_option('tests'))

  # Scripts that pass with everything including pylint
  scripts = files(
    'benchmark.py',
    'plot.py',
  )

  if is_variable('black') and black.found()
    black_opts = ['-l', '79', '-q', '--check']
    test('black', black, args: black_opts + scripts, suite: 'scripts')
  endif

  if is_variable('flake8') and flake8.found()
    test('flake8', flake8, args: scripts, suite: 'scripts')
  endif

  if is_variable('pylint') and pylint.found()
    test('pylint', pylint, args: scripts, suite: 'scripts')
  endif
endif
