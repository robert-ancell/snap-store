project('snap-store', 'c',
        version : '0.1',
        default_options : ['warning_level=3', 'c_std=c11'])

gnome = import('gnome')
i18n = import('i18n')

conf = configuration_data()
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(output: 'config.h', configuration: conf)

top_inc = include_directories('.')

gtk_dep = dependency('gtk+-3.0')
snapd_glib_dep = dependency('snapd-glib')
soup_dep = dependency('libsoup-2.4')

install_data('io.snapcraft.Store.desktop', install_dir: join_paths(get_option('datadir'), 'applications'))

subdir('po')
subdir('src')
