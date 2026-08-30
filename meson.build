project('imsg', 'c', version: '0.1', meson_version: '>=1.6.0', default_options: ['c_std=c11'])

# for IOV_MAX
add_project_arguments('-D_GNU_SOURCE', language: 'c')

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')

lua_only = get_option('lua-only')
is_static = get_option('lua-static')

if build_machine.system() == 'openbsd'
  # Use system libutil (contains imsg)
  libimsg_dep = [cc.find_library('util', static: is_static)]
  imsg_dep = declare_dependency(dependencies: libimsg_dep)
  meson.override_dependency('imsg', imsg_dep)
  libimsg_link = []
  imsg_incdir = []
else
  # Build imsg from source
  imsg_deps = []

  if build_machine.system() in ['freebsd', 'linux']
    # FreeBSD/musl/glibc provide explicit_bzero and strlcpy natively
    add_project_arguments('-Dfreezero(p,s)=(explicit_bzero(p,s),free(p))', language: 'c')
    if build_machine.system() == 'freebsd'
      lua_only = true
    endif
  else
    # Fallback: use libbsd
    imsg_deps += dependency('libbsd-overlay')
  endif

  instlib = not lua_only

  if is_static
    libimsg = static_library('imsg', ['imsg.c', 'imsg-buffer.c'],
      dependencies: imsg_deps, install: instlib)
  else
    libimsg = shared_library('imsg', ['imsg.c', 'imsg-buffer.c'],
      dependencies: imsg_deps, install: instlib, version: '1.0.0')
  endif

  libimsg_dep = []
  libimsg_link = [libimsg]

  if instlib
    install_headers('imsg.h')
    install_man('ibuf_add.3', 'imsg_init.3')
    pkgconfig.generate(libimsg)
  endif

  imsg_incdir = include_directories('.')

  imsg_dep = declare_dependency(
    link_with: libimsg,
    include_directories: imsg_incdir,
    dependencies: imsg_deps,
  )
  meson.override_dependency('imsg', imsg_dep)
endif

ibuf_test = executable('ibuf_test', 'ibuf_test.c',
  link_with: libimsg_link, dependencies: libimsg_dep)
test('ibuf_test', ibuf_test)

which_lua = get_option('lua')
if which_lua != 'false'
  subdir('lua')
endif
