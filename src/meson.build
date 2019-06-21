resources_src = gnome.compile_resources(
  'store-resources',
  'snap-store.gresource.xml',
  source_dir : '.',
  c_name : 'snap'
)

exe = executable('snap-store',
                 resources_src,
                 sources : [
                   'snap-store.c',
                   'store-application.c',
                   'store-application-window.c',
                   'store-category-view.c',
                   'store-app-tile.c',
                   'store-hero-tile.c',
                   'store-home-page.c'
                 ],
                 dependencies : [ gtk_dep, snapd_glib_dep ],
                 install : true)
