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

client_protos_src = []
client_protos_headers = []

client_protocols = [
  [wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
  ['protocols', 'wlr-layer-shell-unstable-v1.xml'],
  ['protocols', 'wlr-input-inhibitor-unstable-v1.xml'],
]

foreach p : client_protocols
  xml = join_paths(p)
  client_protos_src += wayland_scanner_code.process(xml)
  client_protos_headers += wayland_scanner_client.process(xml)
endforeach

lib_client_protos = static_library(
  'client_protos',
  client_protos_src + client_protos_headers,
  dependencies: [wayland_client]
)

client_protos = declare_dependency(
  link_with: lib_client_protos,
  sources: client_protos_headers,
)

dependencies = [
  cairo,
  client_protos,
  pangocairo,
  xkbcommon,
  wayland_client,
  wayland_cursor,
  sway_client_helpers,
  xml2,
]

sources = files(
  'globals.c',
  'main.c',
  'menu.c',
  'output.c',
  'pixmap.c',
  'render.c',
  'search.c',
  'seat.c',
  'surface.c',
)

executable(
  meson.project_name(),
  sources,
  dependencies: dependencies,
)

