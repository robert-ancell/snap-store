/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-hero-tile.h"

#include "store-image.h"

struct _StoreHeroTile
{
    GtkEventBox parent_instance;

    StoreImage *icon_image;
    GtkLabel *summary_label;
    GtkLabel *title_label;

    StoreApp *app;
};

G_DEFINE_TYPE (StoreHeroTile, store_hero_tile, GTK_TYPE_EVENT_BOX)

enum
{
    SIGNAL_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static gboolean
button_release_event_cb (StoreHeroTile *self)
{
    g_signal_emit (self, signals[SIGNAL_ACTIVATED], 0);
    return TRUE;
}

static void
store_hero_tile_dispose (GObject *object)
{
    StoreHeroTile *self = STORE_HERO_TILE (object);
    g_clear_object (&self->app);

    G_OBJECT_CLASS (store_hero_tile_parent_class)->dispose (object);
}

static void
store_hero_tile_class_init (StoreHeroTileClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_hero_tile_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-hero-tile.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHeroTile, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHeroTile, summary_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHeroTile, title_label);

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
store_hero_tile_init (StoreHeroTile *self)
{
    store_image_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));

    store_image_set_size (self->icon_image, 128, 128); // FIXME: Move into .ui
}

StoreHeroTile *
store_hero_tile_new (void)
{
    return g_object_new (store_hero_tile_get_type (), NULL);
}

void
store_hero_tile_set_cache (StoreHeroTile *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_HERO_TILE (self));
    store_image_set_cache (self->icon_image, cache);
}

void
store_hero_tile_set_app (StoreHeroTile *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_HERO_TILE (self));

    if (self->app == app)
        return;

    g_clear_object (&self->app);
    if (app != NULL)
        self->app = g_object_ref (app);

    g_autofree gchar *title = g_utf8_strup (store_app_get_title (app), -1);
    gtk_label_set_label (self->title_label, title);
    gtk_label_set_label (self->summary_label, store_app_get_summary (app));
    store_image_set_url (self->icon_image, NULL); // FIXME: Hack to reset icon
    if (store_app_get_icon (app) != NULL)
        store_image_set_url (self->icon_image, store_media_get_url (store_app_get_icon (app)));
}

StoreApp *
store_hero_tile_get_app (StoreHeroTile *self)
{
    g_return_val_if_fail (STORE_IS_HERO_TILE (self), NULL);
    return self->app;
}
