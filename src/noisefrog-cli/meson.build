noisefrog_cli_src = [
  'main.cpp',
]

pulse_lib = noisefrog_compiler.find_library('pulse')

executable('noisefrog-cli', noisefrog_cli_src,
  dependencies        : [ pulse_lib ],
  override_options    : [ 'cpp_std=c++20' ])
