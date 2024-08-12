project(
  'trappist',
  'c',
  license: 'GPL-2.0-only',
  default_options: [
    'c_std=c11',
    'warning_level=2',
  ],
)

add_project_arguments(
  [
    '-Wno-unused-parameter',
    '-D_XOPEN_SOURCE=700',
  ],
  language: 'c',
)

cc = meson.get_compiler('c')

add_project_arguments(cc.get_supported_arguments(
  [
    '-Wno-unused-parameter',
  ]),
  language: 'c',
)

# Need '>=1.20.0' for wl_output version 4 which gives output name
wayland_client = dependency('wayland-client', version: '>=1.20.0')
wayland_cursor = dependency('wayland-cursor')
wayland_protos = dependency('wayland-protocols')
xkbcommon = dependency('xkbcommon')
cairo = dependency('cairo')
pangocairo = dependency('pangocairo')
xml2 = dependency('libxml-2.0')
svg = dependency('librsvg-2.0', version: '>=2.46', required: false)
inih = dependency('inih')
talloc = dependency('talloc')


subdir('sway-client-helpers')

wayland_scanner = find_program('wayland-scanner')
wl_protocol_dir = wayland_protos.get_variable('pkgdatadir')
wayland_scanner_code = generator(
  wayland_scanner,
  output: '@BASENAME@-protocol.c',
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
  wayland_scanner,
  output: '@BASENAME@-client-protocol.h',
  arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

protos_src = []

client_protocols = [
  wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml',
  'protocols/wlr-layer-shell-unstable-v1.xml',
]

foreach xml : client_protocols
  protos_src += wayland_scanner_code.process(xml)
  protos_src += wayland_scanner_client.process(xml)
endforeach

sfdo_icon = dependency(
  'libsfdo-icon',
  version: '>=0.1.0',
  fallback: 'libsfdo',
  default_options: [
    'default_library=static',
    'examples=false',
  ],
)

dependencies = [
  cairo,
  pangocairo,
  xkbcommon,
  wayland_client,
  wayland_cursor,
  sway_client_helpers,
  svg,
  xml2,
  sfdo_icon,
  inih,
  talloc,
]

sources = files(
  'src/conf.c',
  'src/globals.c',
  'src/icon.c',
  'src/main.c',
  'src/menu.c',
  'src/output.c',
  'src/pixmap.c',
  'src/render.c',
  'src/search.c',
  'src/seat.c',
  'src/surface.c',
  'ccan/ccan/opt/helpers.c',
  'ccan/ccan/opt/opt.c',
  'ccan/ccan/opt/parse.c',
  'ccan/ccan/opt/usage.c',
)

trappist_inc = include_directories(
  'include',
  'ccan',
)

executable(
  meson.project_name(),
  sources + protos_src,
  include_directories: trappist_inc,
  dependencies: dependencies,
)

