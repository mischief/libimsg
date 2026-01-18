# imsg

This is a vendored copy and lua binding to OpenBSD's
[imsg](https://man.openbsd.org/imsg_init), and is distributed under the ISC
license.

## Installation

### Linux

On linux, libbsd is needed.

    sudo apt-get install libbsd-dev

Lua bindings need lua5.4 or luajit.

    sudo apt-get install liblua5.4-dev # or libluajit-5.1-dev

Finally,

    git clone https://github.com/mischief/libimsg.git
    cd libimsg
    meson setup -Dlua=5.4 build
    sudo ninja -C build install

### OpenBSD

    pkg_add lua%5.4

    git clone https://github.com/mischief/libimsg.git
    cd libimsg
    meson setup -Dlua=5.4 build
    doas ninja -C build install

### Luarocks

    git clone https://github.com/mischief/libimsg.git
    cd libimsg
    luarocks make --local imsg-scm-0.rockspec

or

    luarocks install https://github.com/mischief/libimsg/raw/refs/heads/main/imsg-scm-0.rockspec

## Documentation

Manuals for imsg are online at https://man.openbsd.org/imsg_init, or they can
be viewed locally with `man ./imsg_init.3` prior to installation, or on Linux, `man
imsg_init` after installation.

On OpenBSD, the system manuals can be viewed locally with `man imsg_init`.

[LDoc](https://github.com/lunarmodules/ldoc) can build documentation for the Lua module.

    sudo apt-get install lua-ldoc # or doas pkg_add lualdoc
    ninja -C build lua/index.html

The output is in `build/lua/index.html`

A plaintext version can be viewed with `ldoc --dump lua/imsg_lua.c`.

