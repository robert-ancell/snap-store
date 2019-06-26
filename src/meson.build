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
                   'store-cache.c',
                   'store-category-view.c',
                   'store-channel.c',
                   'store-channel-combo.c',
                   'store-hero-tile.c',
                   'store-home-page.c',
                   'store-image.c',
                   'store-install-button.c',
                   'store-media.c',
                   'store-odrs-client.c',
                   'store-odrs-review.c',
                   'store-rating-label.c',
                   'store-review-view.c',
                   'store-snap-app.c',
                   'store-window.c'
                 ],
                 dependencies : [ gtk_dep, snapd_glib_dep ],
                 include_directories : [ top_inc ],
                 install : true)
