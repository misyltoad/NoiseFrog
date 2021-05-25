rnnoise_src = [
  'rnnoise/src/denoise.c',
  'rnnoise/src/rnn.c',
  'rnnoise/src/rnn_data.c',
  'rnnoise/src/rnn_reader.c',
  'rnnoise/src/pitch.c',
  'rnnoise/src/kiss_fft.c',
  'rnnoise/src/celt_lpc.c',
]

rnnoise_include = [
  'rnnoise/include',
]

m_lib = noisefrog_compiler.find_library('m')

rnnoise_lib = static_library('rnnoise', rnnoise_src,
  dependencies        : [ m_lib ],
  include_directories : rnnoise_include)

rnnoise_dep = declare_dependency(
  link_with           : rnnoise_lib,
  include_directories : rnnoise_include)
