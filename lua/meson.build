if which_lua == 'luajit'
  install_cmod = get_option('prefix') / 'lib/lua/5.1'
  install_mod = get_option('prefix') / 'share/lua/5.1'
else
  install_cmod = get_option('prefix') / 'lib/lua/' + which_lua.replace('lua', '')
  install_mod = get_option('prefix') / 'share/lua/' + which_lua.replace('lua', '')
endif

cmod_override = get_option('lua-module-directory')
if cmod_override != ''
  install_cmod = cmod_override
endif

dep_name = which_lua
bin_name = which_lua
if build_machine.system() == 'openbsd'
  dep_name = dep_name.replace('.', '')
  bin_name = dep_name
endif

if build_machine.system() == 'freebsd'
  bin_name = dep_name.replace('.', '')
  dep_name = dep_name.replace('lua', 'lua-')
endif

lua_dep = dependency(dep_name)

lua_libimsg = shared_library('imsg',
  'imsg_lua.c',
  name_prefix: '',
  include_directories: [imsg_incdir],
  dependencies: [lua_dep, libimsg_dep],
  link_with: libimsg_link,
  install: true,
  install_dir: install_cmod,
)

lua_bin = find_program(bin_name)
test('imsg_lua_test', lua_bin, depends: lua_libimsg, args: files('imsg_test.lua'), workdir: meson.current_build_dir())

ldoc_bin = find_program('ldoc', required: false)
if ldoc_bin.found()
  documentation = custom_target('ldoc',
    input: [files('imsg_lua.c')],
    output: ['index.html', 'ldoc.css'],
    command: [ldoc_bin, '-d', '@OUTDIR@', '@INPUT@'],
  )
endif

