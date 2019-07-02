/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-app-installed-tile.h"

#include "store-image.h"
#include "store-rating-label.h"

struct _StoreAppInstalledTile
{
    GtkEventBox parent_instance;

    StoreImage *icon_image;
    GtkLabel *publisher_label;
    GtkImage *publisher_validated_image;
    StoreRatingLabel *rating_label;
    GtkLabel *size_label;
    GtkLabel *summary_label;
    GtkLabel *title_label;

    StoreApp *app;
};

G_DEFINE_TYPE (StoreAppInstalledTile, store_app_installed_tile, GTK_TYPE_EVENT_BOX)

enum
{
    SIGNAL_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

static gboolean
button_release_event_cb (StoreAppInstalledTile *self)
{
    g_signal_emit (self, signals[SIGNAL_ACTIVATED], 0);
    return TRUE;
}

static void
store_app_installed_tile_dispose (GObject *object)
{
    StoreAppInstalledTile *self = STORE_APP_INSTALLED_TILE (object);

    g_clear_object (&self->app);

    G_OBJECT_CLASS (store_app_installed_tile_parent_class)->dispose (object);
}

static void
store_app_installed_tile_class_init (StoreAppInstalledTileClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_installed_tile_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-app-installed-tile.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppInstalledTile, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppInstalledTile, publisher_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppInstalledTile, publisher_validated_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppInstalledTile, rating_label);
    // FIXMEgtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppInstalledTile, size_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppInstalledTile, summary_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppInstalledTile, title_label);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), button_release_event_cb);

    signals[SIGNAL_ACTIVATED] = g_signal_new ("activated",
                                              G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                              G_SIGNAL_RUN_LAST,
                                              0,
                                              NULL, NULL,
                                              NULL,
                                              G_TYPE_NONE,
                                              0);
}

static void
store_app_installed_tile_init (StoreAppInstalledTile *self)
{
    store_image_get_type ();
    store_rating_label_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreAppInstalledTile *
store_app_installed_tile_new (void)
{
    return g_object_new (store_app_installed_tile_get_type (), NULL);
}

void
store_app_installed_tile_set_cache (StoreAppInstalledTile *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_APP_INSTALLED_TILE (self));
    store_image_set_cache (self->icon_image, cache);
}

void
store_app_installed_tile_set_app (StoreAppInstalledTile *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_APP_INSTALLED_TILE (self));

    if (self->app == app)
        return;

    g_clear_object (&self->app);
    if (app != NULL)
        self->app = g_object_ref (app);

    g_object_bind_property (app, "icon", self->icon_image, "media", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "publisher", self->publisher_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "publisher-validated", self->publisher_validated_image, "visible", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "review-average", self->rating_label, "rating", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "summary", self->summary_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "title", self->title_label, "label", G_BINDING_SYNC_CREATE);
    // FIXME: Size
}

StoreApp *
store_app_installed_tile_get_app (StoreAppInstalledTile *self)
{
    g_return_val_if_fail (STORE_IS_APP_INSTALLED_TILE (self), NULL);
    return self->app;
}
