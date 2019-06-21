/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-hero-tile.h"

struct _StoreHeroTile
{
    GtkFlowBoxChild parent_instance;

    GtkImage *icon_image;
    GtkLabel *name_label;
    GtkLabel *summary_label;

    StoreApp *app;
};

G_DEFINE_TYPE (StoreHeroTile, store_hero_tile, GTK_TYPE_FLOW_BOX_CHILD)

static void
store_hero_tile_dispose (GObject *object)
{
    StoreHeroTile *self = STORE_HERO_TILE (object);
    g_clear_object (&self->app);
}

static void
store_hero_tile_class_init (StoreHeroTileClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_hero_tile_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/store-hero-tile.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHeroTile, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHeroTile, name_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHeroTile, summary_label);
}

static void
store_hero_tile_init (StoreHeroTile *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreHeroTile *
store_hero_tile_new (void)
{
    return g_object_new (store_hero_tile_get_type (), NULL);
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

    gtk_label_set_label (self->name_label, store_app_get_title (app));
    gtk_label_set_label (self->summary_label, store_app_get_summary (app));
    gtk_image_set_from_resource (self->icon_image, "/com/ubuntu/SnapStore/default-snap-icon.svg");
}

StoreApp *
store_hero_tile_get_app (StoreHeroTile *self)
{
    g_return_val_if_fail (STORE_IS_HERO_TILE (self), NULL);
    return self->app;
}
