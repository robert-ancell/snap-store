project('snap-store', 'c',
        version : '0.1',
        default_options : ['warning_level=3', 'c_std=c11'])

# GResource generates longer strings than the spec allows but GCC and clang support these fine
add_project_arguments('-Wno-overlength-strings', language : 'c')

gnome = import('gnome')
i18n = import('i18n')

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(output: 'config.h', configuration: conf)

top_inc = include_directories('.')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
gtk_dep = dependency('gtk+-3.0')
json_glib_dep = dependency('json-glib-1.0')
snapd_glib_dep = dependency('snapd-glib')
soup_dep = dependency('libsoup-2.4')

install_data('io.snapcraft.Store.desktop', install_dir: join_paths(get_option('datadir'), 'applications'))

subdir('po')
subdir('src')
subdir('tests')
