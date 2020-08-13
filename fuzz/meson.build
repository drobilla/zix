# Copyright 2020-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

fuzzers = [
  'btree_fuzz',
  'btree_fuzz_gen',
]

fuzzer_c_args = extra_c_args

fuzzer_c_suppressions = []
if cc.get_id() in ['clang', 'emscripten']
  fuzzer_c_suppressions = [
    '-Wno-bad-function-cast',
    '-Wno-c11-extensions', # Glib
    '-Wno-reserved-identifier',
  ]
elif cc.get_id() == 'gcc'
  fuzzer_c_suppressions = ['-Wno-bad-function-cast']
endif
fuzzer_c_args += cc.get_supported_arguments(fuzzer_c_suppressions)

foreach fuzzer : fuzzers
  executable(
    fuzzer,
    files('@0@.c'.format(fuzzer)),
    c_args: c_suppressions + fuzzer_c_args,
    dependencies: zix_dep,
    implicit_include_directories: false,
    include_directories: include_dirs,
  )
endforeach
