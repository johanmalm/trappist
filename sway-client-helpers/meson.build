dependencies = [
  dependency('cairo'),
  dependency('pangocairo'),
  dependency('wayland-client'),
]

sway_client_helpers_inc = include_directories('include')
lib_sway_client_helpers = library(
  meson.project_name(),
  sources: files(
    'cairo.c',
    'log.c',
    'loop.c',
    'pango.c',
    'pool-buffer.c',
    'unicode.c',
    'util.c',
  ),
  include_directories: sway_client_helpers_inc,
  dependencies: dependencies,
  install: true,
)

sway_client_helpers = declare_dependency(
  link_with: lib_sway_client_helpers,
  dependencies: dependencies,
  include_directories: sway_client_helpers_inc,
)
