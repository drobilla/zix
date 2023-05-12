# Copyright 2020-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

benchmarks = [
  'dict_bench',
  'tree_bench',
]

glib_dep = dependency(
  'glib-2.0',
  include_type: 'system',
  required: get_option('benchmarks'),
  version: '>= 2.0.0',
)

if glib_dep.found()
  build_benchmarks = true
  benchmark_c_args = platform_c_args

  if cc.get_id() in ['clang', 'emscripten']
    benchmark_c_suppressions = [
      '-Wno-reserved-identifier',
    ]

    benchmark_c_args += cc.get_supported_arguments(benchmark_c_suppressions)
  endif

  foreach benchmark : benchmarks
    benchmark(
      benchmark,
      executable(
        benchmark,
        files('@0@.c'.format(benchmark)),
        c_args: c_suppressions + benchmark_c_args,
        dependencies: [zix_dep, glib_dep],
        include_directories: include_dirs,
      ),
    )
  endforeach
endif
