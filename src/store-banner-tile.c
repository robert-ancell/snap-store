/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-banner-tile.h"

struct _StoreBannerTile
{
    GtkEventBox parent_instance;

    StoreImage *image;

    StoreApp *app;
};

G_DEFINE_TYPE (StoreBannerTile, store_banner_tile, GTK_TYPE_EVENT_BOX)

enum
{
    SIGNAL_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

static gboolean
button_release_event_cb (StoreBannerTile *self)
{
    g_signal_emit (self, signals[SIGNAL_ACTIVATED], 0);
    return TRUE;
}

static void
store_banner_tile_dispose (GObject *object)
{
    StoreBannerTile *self = STORE_BANNER_TILE (object);

    g_clear_object (&self->app);

    G_OBJECT_CLASS (store_banner_tile_parent_class)->dispose (object);
}

static void
store_banner_tile_class_init (StoreBannerTileClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_banner_tile_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-banner-tile.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreBannerTile, image);

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
store_banner_tile_init (StoreBannerTile *self)
{
    store_image_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_banner_tile_set_cache (StoreBannerTile *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_BANNER_TILE (self));
    store_image_set_cache (self->image, cache);
}

void
store_banner_tile_set_app (StoreBannerTile *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_BANNER_TILE (self));

    if (self->app == app)
        return;

    g_clear_object (&self->app);
    if (app != NULL)
        self->app = g_object_ref (app);

    g_object_bind_property (app, "banner", self->image, "media", G_BINDING_SYNC_CREATE);
}

StoreApp *
store_banner_tile_get_app (StoreBannerTile *self)
{
    g_return_val_if_fail (STORE_IS_BANNER_TILE (self), NULL);
    return self->app;
}
