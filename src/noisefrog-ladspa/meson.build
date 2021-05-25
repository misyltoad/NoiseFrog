noisefrog_ladspa_src = [
  'main.cpp',
]

shared_library('noisefrog-ladspa', noisefrog_ladspa_src,
  dependencies        : [ rnnoise_dep ],
  include_directories : [ '../../thirdparty/ladspa-sdk' ],
  override_options    : [ 'cpp_std=c++20' ])
