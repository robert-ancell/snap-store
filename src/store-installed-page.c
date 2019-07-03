/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <snapd-glib/snapd-glib.h>

#include "store-app.h"
#include "store-app-installed-tile.h"
#include "store-installed-page.h"

struct _StoreInstalledPage
{
    StorePage parent_instance;

    GtkBox *app_box;
    GtkLabel *summary_label; // FIXME

    GCancellable *cancellable;
    StoreSnapPool *snap_pool;
};

G_DEFINE_TYPE (StoreInstalledPage, store_installed_page, store_page_get_type ())

enum
{
    SIGNAL_APP_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
tile_activated_cb (StoreInstalledPage *self, StoreAppInstalledTile *tile)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, store_app_installed_tile_get_app (tile));
}

static void
get_snaps_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreInstalledPage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_get_snaps_finish (SNAPD_CLIENT (object), result, &error);
    if (snaps == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to get installed snaps: %s", error->message);
        return;
    }

    /* Ensure correct number of app tiles */
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->app_box));
    guint n_tiles = g_list_length (children);
    while (n_tiles < snaps->len) {
        StoreAppInstalledTile *tile = store_app_installed_tile_new ();
        gtk_widget_show (GTK_WIDGET (tile));
        g_signal_connect_object (tile, "activated", G_CALLBACK (tile_activated_cb), self, G_CONNECT_SWAPPED);
        store_app_installed_tile_set_cache (tile, store_page_get_cache (STORE_PAGE (self)));
        gtk_container_add (GTK_CONTAINER (self->app_box), GTK_WIDGET (tile));
        n_tiles++;
        children = g_list_append (children, tile);
    }
    while (n_tiles > snaps->len) {
        for (GList *link = g_list_nth (children, snaps->len); link != NULL; link = link->next) {
            StoreAppInstalledTile *tile = link->data;
            gtk_container_remove (GTK_CONTAINER (self->app_box), GTK_WIDGET (tile));
        }
    }

    for (guint i = 0; i < snaps->len; i++) {
        SnapdSnap *snap = g_ptr_array_index (snaps, i);
        g_autoptr(StoreSnapApp) app = store_snap_pool_get_snap (self->snap_pool, snapd_snap_get_name (snap));
        store_app_set_installed (STORE_APP (app), TRUE);
        store_snap_app_update_from_search (app, snap);
        //FIXMEset_review_counts (self, STORE_APP (app));

        StoreAppInstalledTile *tile = g_list_nth_data (children, i);
        store_app_installed_tile_set_app (tile, STORE_APP (app));
    }
}

static void
store_installed_page_dispose (GObject *object)
{
    StoreInstalledPage *self = STORE_INSTALLED_PAGE (object);

    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_clear_object (&self->snap_pool);

    G_OBJECT_CLASS (store_installed_page_parent_class)->dispose (object);
}

static void
store_installed_page_set_cache (StorePage *page, StoreCache *cache)
{
    // FIXME: Should apply to children

    STORE_PAGE_CLASS (store_installed_page_parent_class)->set_cache (page, cache);
}

static void
store_installed_page_class_init (StoreInstalledPageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_installed_page_dispose;
    STORE_PAGE_CLASS (klass)->set_cache = store_installed_page_set_cache;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-installed-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreInstalledPage, app_box);
    //FIXMEgtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreInstalledPage, summary_label);

    signals[SIGNAL_APP_ACTIVATED] = g_signal_new ("app-activated",
                                                  G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                                  G_SIGNAL_RUN_LAST,
                                                  0,
                                                  NULL, NULL,
                                                  NULL,
                                                  G_TYPE_NONE,
                                                  1, store_app_get_type ());
}

static void
store_installed_page_init (StoreInstalledPage *self)
{
    self->cancellable = g_cancellable_new ();

    store_page_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_installed_page_set_snap_pool (StoreInstalledPage *self, StoreSnapPool *pool)
{
    g_return_if_fail (STORE_IS_INSTALLED_PAGE (self));
    g_set_object (&self->snap_pool, pool);
}

void
store_installed_page_load (StoreInstalledPage *self)
{
    g_return_if_fail (STORE_IS_INSTALLED_PAGE (self));

    g_autoptr(SnapdClient) client = snapd_client_new ();
    snapd_client_get_snaps_async (client, SNAPD_GET_SNAPS_FLAGS_NONE, NULL, self->cancellable, get_snaps_cb, self);
}
