/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-app-tile.h"

struct _StoreAppTile
{
    GtkFlowBoxChild parent_instance;

    GtkImage *icon_image;
    GtkLabel *name_label;
    GtkLabel *publisher_label;
    GtkImage *publisher_validated_image; // FIXME
    GtkLabel *summary_label;

    gchar *name;
};

G_DEFINE_TYPE (StoreAppTile, store_app_tile, GTK_TYPE_FLOW_BOX_CHILD)

static void
store_app_tile_dispose (GObject *object)
{
    StoreAppTile *self = STORE_APP_TILE (object);
    g_clear_pointer (&self->name, g_free);
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
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreAppTile *
store_app_tile_new (const gchar *name)
{
    StoreAppTile *self = g_object_new (store_app_tile_get_type (), NULL);
    store_app_tile_set_name (self, name);
    return self;
}


void
store_app_tile_set_name (StoreAppTile *self, const gchar *name)
{
    g_return_if_fail (STORE_IS_APP_TILE (self));

    g_free (self->name);
    self->name = g_strdup (name);

    gtk_label_set_label (self->name_label, name);
    gtk_label_set_label (self->publisher_label, "Publisher");
    gtk_label_set_label (self->summary_label, "Lorem Ipsum...");
    gtk_image_set_from_resource (self->icon_image, "/com/ubuntu/SnapStore/default-snap-icon.svg");
}
