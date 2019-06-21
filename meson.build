project('snap-store', 'c',
        version : '0.1',
        default_options : ['warning_level=3', 'c_std=c11'])

gnome = import('gnome')

gtk_dep = dependency('gtk+-3.0')
snapd_glib_dep = dependency('snapd-glib')
soup_dep = dependency('libsoup-2.4')

install_data('snap-store.desktop', install_dir: join_paths(get_option('datadir'), 'applications'))

subdir('src')
