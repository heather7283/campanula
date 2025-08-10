project('campanula', 'c',
        license: 'GPL-3.0-or-later',
        version: '0.1.0',
        meson_version: '>= 1.3.0',
        default_options: ['warning_level=3', 'c_std=c23'])

add_project_arguments([
  '-D_GNU_SOURCE',
  '-Wno-pedantic',
  '-Wno-unused-parameter',
], language: 'c')

libmpv_dep = dependency('mpv')
libcurl_dep = dependency('libcurl')
libopenssl_dep = dependency('OpenSSL') # needed for md5, curl links against it anyway
libjsonc_dep = dependency('json-c')
libsqlite_dep = dependency('sqlite3')
libncursesw_dep = dependency('ncursesw') # I will regret this later.
pollen_dep = dependency('pollen', version: '>=2.1.0')

dependencies = [
  libmpv_dep,
  libcurl_dep,
  libopenssl_dep,
  libjsonc_dep,
  libsqlite_dep,
  libncursesw_dep,
  pollen_dep,
]

include_dirs = include_directories([
  'src',
])

sources = files([
  'src/campanula.c',
  'src/auth.c',
  'src/config.c',
  'src/log.c',
  'src/xmalloc.c',
  'src/eventloop.c',
  'src/signals.c',

  'src/types/song.c',
  'src/types/album.c',
  'src/types/artist.c',

  'src/api/network.c',
  'src/api/requests.c',
  'src/api/json.c',
  'src/api/types.c',

  'src/player/internal.c',
  'src/player/control.c',
  'src/player/playlist.c',
  'src/player/events.c',
  'src/player/stream.c',
  'src/player/utils.c',

  'src/commands/parse.c',

  'src/db/internal.c',
  'src/db/populate.c',
  'src/db/query.c',

  'src/tui/internal.c',
  'src/tui/init.c',
  'src/tui/utils.c',
  'src/tui/events.c',
  'src/tui/draw.c',

  'src/collections/string.c',
  'src/collections/array.c',
])

executable('campanula', sources,
           include_directories: include_dirs,
           dependencies: dependencies,
           install: true)

subdir('test')

