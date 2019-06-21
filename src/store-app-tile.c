/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-app-tile.h"

#include "store-image.h"

struct _StoreAppTile
{
    GtkFlowBoxChild parent_instance;

    StoreImage *icon_image;
    GtkLabel *name_label;
    GtkLabel *publisher_label;
    GtkImage *publisher_validated_image; // FIXME
    GtkLabel *summary_label;

    StoreApp *app;
};

G_DEFINE_TYPE (StoreAppTile, store_app_tile, GTK_TYPE_FLOW_BOX_CHILD)

static void
store_app_tile_dispose (GObject *object)
{
    StoreAppTile *self = STORE_APP_TILE (object);
    g_clear_object (&self->app);
}

static void
store_app_tile_class_init (StoreAppTileClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_tile_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/store-app-tile.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppTile, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppTile, name_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppTile, publisher_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppTile, summary_label);
}

static void
store_app_tile_init (StoreAppTile *self)
{
    store_image_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreAppTile *
store_app_tile_new (void)
{
    return g_object_new (store_app_tile_get_type (), NULL);
}

void
store_app_tile_set_app (StoreAppTile *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_APP_TILE (self));

    if (self->app == app)
        return;

    g_clear_object (&self->app);
    if (app != NULL)
        self->app = g_object_ref (app);

    gtk_label_set_label (self->name_label, store_app_get_title (app));
    gtk_label_set_label (self->publisher_label, store_app_get_publisher (app));
    gtk_label_set_label (self->summary_label, store_app_get_summary (app));
    store_image_set_url (self->icon_image, store_app_get_icon (app));
}

StoreApp *
store_app_tile_get_app (StoreAppTile *self)
{
    g_return_val_if_fail (STORE_IS_APP_TILE (self), NULL);
    return self->app;
}
