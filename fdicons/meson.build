so_version = ['0', '0', '0']
cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments([
	'-Wundef',
	'-Wlogical-op',
	'-Wmissing-include-dirs',
	'-Wold-style-definition',
	'-Wpointer-arith',
	'-Winit-self',
	'-Wstrict-prototypes',
	'-Wimplicit-fallthrough=2',
	'-Wendif-labels',
	'-Wstrict-aliasing=2',
	'-Woverflow',
	'-Wmissing-prototypes',

	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-unused-parameter',

	'-D_XOPEN_SOURCE=700',
	'-DDATADIR="@0@/@1@"'.format(
		get_option('prefix'), get_option('datadir')),
]), language: 'c')

fdi_inc = include_directories('.', 'include')

subdir('include')

symbols_file = 'fdicons.syms'
symbols_flag = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), symbols_file)

libfdicons = library(
	'fdicons',
	sources: files(
		'src/database.c',
		'src/ini.c',
		'src/lookup.c',
	),
	version: '.'.join(so_version),
	include_directories: fdi_inc,
	install: true,
	link_args: symbols_flag,
)

pkgconfig = import('pkgconfig')
pkgconfig.generate(libfdicons,
	version: meson.project_version(),
	filebase: meson.project_name(),
	name: meson.project_name(),
	description: 'FreeDesktop Icon Theme database query library',
)

