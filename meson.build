project('campanula', 'c',
        meson_version: '>= 1.3.0',
        version: '0.1.0',
        default_options: ['warning_level=3', 'c_std=c23'])

add_project_arguments([
  '-D_GNU_SOURCE',
  '-Wno-pedantic',
  '-Wno-unused-parameter',
], language: 'c')

libmpv_dep = dependency('mpv')
libcurl_dep = dependency('libcurl')
libopenssl_dep = dependency('OpenSSL') # needed for md5, curl links against it anyway
pollen_dep = dependency('pollen', version: '>=2.0.0')

dependencies = [
  libmpv_dep,
  libcurl_dep,
  libopenssl_dep,
  pollen_dep,
]

include_dirs = include_directories([
  'src',
])

sources = [
  'src/campanula.c',
  'src/xmalloc.c',
  'src/collections.c',
  'src/pollen_impl.c',
]

executable('campanula', sources,
           include_directories: include_dirs,
           dependencies: dependencies,
           install: true)

subdir('test')

