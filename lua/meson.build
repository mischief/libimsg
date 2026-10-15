if which_lua == 'rock'
  # luarocks gives the header and module directories; the interpreter
  # resolves the Lua symbols when it loads the module.
  install_cmod = get_option('prefix') / 'lib'
  install_mod = get_option('prefix') / 'share'
elif which_lua == 'luajit'
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

if which_lua == 'rock'
  lua_incdir = get_option('lua-incdir')
  if lua_incdir == ''
    error('-Dlua=rock needs -Dlua-incdir')
  endif
  lua_link = []
  if host_machine.system() == 'darwin'
    lua_link = ['-undefined', 'dynamic_lookup']
  endif
  lua_dep = declare_dependency(compile_args: ['-I' + lua_incdir], link_args: lua_link)
else
  lua_dep = dependency(dep_name)
endif

lua_libimsg = shared_library('imsg',
  'imsg_lua.c',
  name_prefix: '',
  include_directories: [imsg_incdir],
  dependencies: [lua_dep, libimsg_dep],
  link_with: libimsg_link,
  install: true,
  install_dir: install_cmod,
  # A rock build does not link liblua, so Lua symbols stay undefined.
  override_options: which_lua == 'rock' ? ['b_lundef=false'] : [],
)

# A rock build does not know the interpreter; luarocks test runs the tests.
lua_bin = disabler()
if which_lua != 'rock'
  lua_bin = find_program(bin_name, required: false)
endif
if lua_bin.found()
  # The built module comes first, before any installed one.
  test('imsg_lua_test', lua_bin, depends: lua_libimsg, args: files('imsg_test.lua'),
    workdir: meson.current_build_dir(),
    env: {'LUA_CPATH': meson.current_build_dir() / '?.so;;'})
else
  warning('Lua interpreter "' + bin_name + '" not found, skipping tests')
endif

ldoc_bin = find_program('ldoc', required: get_option('docs'))
if ldoc_bin.found()
  documentation = custom_target('ldoc',
    input: [files('imsg_lua.c')],
    output: ['index.html', 'ldoc.css'],
    command: [ldoc_bin, '-c', files('config.ld'), '-d', '@OUTDIR@', '@INPUT@'],
    depend_files: files('config.ld'),
    build_by_default: get_option('docs').enabled(),
    install: get_option('docs').enabled(),
    install_dir: get_option('datadir') / 'doc/imsg',
  )
  alias_target('docs', documentation)
endif

