project('NoiseFrog', ['c', 'cpp'], version : '0.1.0', meson_version : '>= 0.56', default_options : [
  'warning_level=3',
])

noisefrog_compiler = meson.get_compiler('cpp')

add_project_arguments(noisefrog_compiler.get_supported_arguments([
  '-msse2',
  '-mfpmath=sse',
  '-ffast-math',
  '-Wno-missing-field-initializers',
  ]),
  language : 'cpp')

subdir('thirdparty')
subdir('src')
