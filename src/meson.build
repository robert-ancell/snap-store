resources_src = gnome.compile_resources(
  'snap-resources',
  'snap-store.gresource.xml',
  source_dir : '.',
  c_name : 'snap'
)

exe = executable('snap-store',
                 resources_src,
                 sources : [
                   'snap-application.c',
                   'snap-application-window.c',
                   'snap-store.c',
                 ],
                 dependencies : [ gtk_dep, snapd_glib_dep ],
                 install : true)
