project('imsg', 'c', meson_version: '>=1.6.0')

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')

lua_only = get_option('lua-only')

is_static = get_option('lua-static')

if build_machine.system() == 'openbsd'
  libimsg_dep = [cc.find_library('util', static: is_static)]
  libimsg_link = []
  imsg_incdir = []
else
  libbsd_overlay_dep = dependency('libbsd-overlay')

  instlib = true
  if lua_only
    instlib = false
  endif

  if is_static
    libimsg = static_library('imsg', ['imsg.c', 'imsg-buffer.c'], dependencies: [libbsd_overlay_dep], install: instlib)
  else
    libimsg = shared_library('imsg', ['imsg.c', 'imsg-buffer.c'], dependencies: [libbsd_overlay_dep], install: instlib, version: '1.0.0')
  endif
  libimsg_dep = []
  libimsg_link = [libimsg]

  if not lua_only
    install_headers('imsg.h')
    install_man('ibuf_add.3', 'imsg_init.3')

    pkgconfig.generate(libimsg)
  endif

  imsg_incdir = include_directories('.')
endif

ibuf_test = executable('ibuf_test', 'ibuf_test.c', link_with: libimsg_link, dependencies: libimsg_dep)
test('ibuf_test', ibuf_test)

which_lua = get_option('lua')
if which_lua != 'false'
  subdir('lua')
endif

