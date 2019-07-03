/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-app.h"
#include "store-app-installed-tile.h"
#include "store-installed-page.h"

struct _StoreInstalledPage
{
    StorePage parent_instance;

    GtkBox *app_box;
    GtkLabel *summary_label; // FIXME

    GCancellable *cancellable;
};

enum
{
    PROP_0,
    PROP_APPS,
    PROP_LAST
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
store_installed_page_dispose (GObject *object)
{
    StoreInstalledPage *self = STORE_INSTALLED_PAGE (object);

    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);

    G_OBJECT_CLASS (store_installed_page_parent_class)->dispose (object);
}

static void
store_installed_page_get_property (GObject *object, guint prop_id, GValue *value G_GNUC_UNUSED, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
store_installed_page_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreInstalledPage *self = STORE_INSTALLED_PAGE (object);

    switch (prop_id)
    {
    case PROP_APPS:
        store_installed_page_set_apps (self, g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_installed_page_set_model (StorePage *page, StoreModel *model)
{
    StoreInstalledPage *self = STORE_INSTALLED_PAGE (page);

    // FIXME: Should apply to children

    g_object_bind_property (model, "installed", self, "apps", G_BINDING_SYNC_CREATE);

    STORE_PAGE_CLASS (store_installed_page_parent_class)->set_model (page, model);
}

static void
store_installed_page_class_init (StoreInstalledPageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_installed_page_dispose;
    G_OBJECT_CLASS (klass)->get_property = store_installed_page_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_installed_page_set_property;
    STORE_PAGE_CLASS (klass)->set_model = store_installed_page_set_model;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_APPS,
                                     g_param_spec_boxed ("apps", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_WRITABLE));

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
store_installed_page_load (StoreInstalledPage *self)
{
    g_return_if_fail (STORE_IS_INSTALLED_PAGE (self));

    store_model_update_installed_async (store_page_get_model (STORE_PAGE (self)), NULL, NULL, NULL);
}

void
store_installed_page_set_apps (StoreInstalledPage *self, GPtrArray *apps)
{
    g_return_if_fail (STORE_IS_INSTALLED_PAGE (self));

    /* Ensure correct number of app tiles */
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->app_box));
    guint n_tiles = g_list_length (children);
    while (n_tiles < apps->len) {
        StoreAppInstalledTile *tile = store_app_installed_tile_new ();
        gtk_widget_show (GTK_WIDGET (tile));
        g_signal_connect_object (tile, "activated", G_CALLBACK (tile_activated_cb), self, G_CONNECT_SWAPPED);
        store_app_installed_tile_set_model (tile, store_page_get_model (STORE_PAGE (self)));
        gtk_container_add (GTK_CONTAINER (self->app_box), GTK_WIDGET (tile));
        n_tiles++;
        children = g_list_append (children, tile);
    }
    while (n_tiles > apps->len) {
        for (GList *link = g_list_nth (children, apps->len); link != NULL; link = link->next) {
            StoreAppInstalledTile *tile = link->data;
            gtk_container_remove (GTK_CONTAINER (self->app_box), GTK_WIDGET (tile));
        }
    }

    for (guint i = 0; i < apps->len; i++) {
        StoreSnapApp *app = g_ptr_array_index (apps, i);
        StoreAppInstalledTile *tile = g_list_nth_data (children, i);
        store_app_installed_tile_set_app (tile, STORE_APP (app));
    }
}
