/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "snap-app-tile.h"

struct _SnapAppTile
{
    GtkFlowBoxChild parent_instance;

    GtkImage *icon_image;
    GtkLabel *name_label;
    GtkLabel *publisher_label;
    GtkImage *publisher_validated_image; // FIXME
    GtkLabel *summary_label;

    gchar *name;
};

G_DEFINE_TYPE (SnapAppTile, snap_app_tile, GTK_TYPE_FLOW_BOX_CHILD)

static void
snap_app_tile_dispose (GObject *object)
{
    SnapAppTile *self = SNAP_APP_TILE (object);
    g_clear_pointer (&self->name, g_free);
}

static void
snap_app_tile_class_init (SnapAppTileClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = snap_app_tile_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/snap-app-tile.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapAppTile, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapAppTile, name_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapAppTile, publisher_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapAppTile, summary_label);
}

static void
snap_app_tile_init (SnapAppTile *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

SnapAppTile *
snap_app_tile_new (const gchar *name)
{
    SnapAppTile *self = g_object_new (snap_app_tile_get_type (), NULL);
    snap_app_tile_set_name (self, name);
    return self;
}


void
snap_app_tile_set_name (SnapAppTile *self, const gchar *name)
{
    g_return_if_fail (SNAP_IS_APP_TILE (self));

    g_free (self->name);
    self->name = g_strdup (name);

    gtk_label_set_label (self->name_label, name);
    gtk_label_set_label (self->publisher_label, "Publisher");
    gtk_label_set_label (self->summary_label, "Lorem Ipsum...");
}
