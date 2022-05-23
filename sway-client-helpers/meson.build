#project(
#  'sway_client_helpers',
#  'c',
#  version: '0.1.0',
#  license: 'MIT',
#  default_options: [
#    'c_std=c99',
#    'warning_level=2',
#  ],
#)
#
#cc = meson.get_compiler('c')
#
#add_project_arguments(cc.get_supported_arguments(
#  [
#    '-Wno-unused-parameter',
#  ]),
#  language: 'c',
#)

dependencies = [
  dependency('cairo'),
  dependency('pangocairo'),
  dependency('wayland-client'),
]

sway_client_helpers_inc = include_directories('include')
lib_sway_client_helpers = library(
  meson.project_name(),
  sources: files(
    'log.c',
    'loop.c',
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
