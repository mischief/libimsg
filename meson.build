project('imsg', 'c')

pkgconfig = import('pkgconfig')

libbsd_overlay_dep = dependency('libbsd-overlay')

libimsg = library('imsg', ['imsg.c', 'imsg-buffer.c'], dependencies: [libbsd_overlay_dep], install: true)

ibuf_test = executable('ibuf_test', 'ibuf_test.c', link_with: [libimsg])

test('ibuf_test', ibuf_test)

install_headers('imsg.h')
install_man('imsg_init.3')

pkgconfig.generate(libimsg)

