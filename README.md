# imsg

This is a vendored copy and lua binding to OpenBSD's
[imsg](https://man.openbsd.org/imsg_init), and is distributed under the ISC
license.

On OpenBSD, the build uses imsg from the system libutil. On other systems, it
builds the vendored copy. Linux and FreeBSD need no extra libraries. Other
systems need libbsd.

## Installation

The build needs meson 1.6 or later and ninja.

### Luarocks

The Lua module is on luarocks.org. It supports Lua 5.3, 5.4 and 5.5.

    luarocks install imsg

The rock runs meson and ninja, so install them first. The rock links the
vendored imsg statically, or libutil on OpenBSD. It installs no C library.

To build the rock from a checkout:

    git clone https://github.com/mischief/libimsg.git
    cd libimsg
    luarocks make --local imsg-scm-0.rockspec

### Linux

    sudo apt-get install meson ninja-build
    sudo apt-get install liblua5.4-dev # or libluajit-5.1-dev, for the Lua module

    git clone https://github.com/mischief/libimsg.git
    cd libimsg
    meson setup -Dlua=lua5.4 build
    sudo ninja -C build install

### OpenBSD

    pkg_add meson lua%5.4

    git clone https://github.com/mischief/libimsg.git
    cd libimsg
    meson setup -Dlua=lua5.4 build
    doas ninja -C build install

On OpenBSD, only the Lua module is installed. The C library is libutil.

### FreeBSD

The build installs only the Lua module on FreeBSD.

### Build options

    -Dlua=false|luajit|lua5.3|lua5.4|lua5.5   build the Lua module (default false)
    -Dlua-only=true                           install the Lua module only
    -Dlua-static=true                         link imsg statically into the module
    -Dlua-module-directory=DIR                install the module into DIR
    -Ddocs=enabled|disabled|auto              build the LDoc documentation

### Meson subproject

The build declares the `imsg` dependency with `meson.override_dependency`.
A project that includes libimsg as a subproject gets it with
`dependency('imsg')`.

## Documentation

Manuals for imsg are online at https://man.openbsd.org/imsg_init, or they can
be viewed locally with `man ./imsg_init.3` prior to installation, or on Linux, `man
imsg_init` after installation.

On OpenBSD, the system manuals can be viewed locally with `man imsg_init`.

[LDoc](https://github.com/lunarmodules/ldoc) can build documentation for the Lua module.

    sudo apt-get install lua-ldoc # or doas pkg_add lualdoc
    meson configure -Ddocs=enabled build
    ninja -C build docs

The output is in `build/lua/index.html`, and is installed to
`$prefix/share/doc/imsg` when `-Ddocs=enabled` is set.

A plaintext version can be viewed with `ldoc --dump lua/imsg_lua.c`.

## Tests

The Lua binding tests need luaposix.

    sudo apt-get install lua-posix # or doas pkg_add luaposix
    meson test -C build
