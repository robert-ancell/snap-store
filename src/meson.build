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
                   'store-app.c',
                   'store-app-page.c',
                   'store-app-tile.c',
                   'store-category-view.c',
                   'store-hero-tile.c',
                   'store-home-page.c',
                   'store-image.c',
                   'store-odrs-client.c',
                   'store-window.c'
                 ],
                 dependencies : [ gtk_dep, snapd_glib_dep ],
                 install : true)
