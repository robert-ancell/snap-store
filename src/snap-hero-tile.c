/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "snap-hero-tile.h"

struct _SnapHeroTile
{
    GtkFlowBoxChild parent_instance;

    GtkImage *icon_image;
    GtkLabel *name_label;
    GtkLabel *summary_label;

    gchar *name;
};

G_DEFINE_TYPE (SnapHeroTile, snap_hero_tile, GTK_TYPE_FLOW_BOX_CHILD)

static void
snap_hero_tile_dispose (GObject *object)
{
    SnapHeroTile *self = SNAP_HERO_TILE (object);
    g_clear_pointer (&self->name, g_free);
}

static void
snap_hero_tile_class_init (SnapHeroTileClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = snap_hero_tile_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/snap-hero-tile.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapHeroTile, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapHeroTile, name_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapHeroTile, summary_label);
}

static void
snap_hero_tile_init (SnapHeroTile *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

SnapHeroTile *
snap_hero_tile_new (const gchar *name)
{
    SnapHeroTile *self = g_object_new (snap_hero_tile_get_type (), NULL);
    snap_hero_tile_set_name (self, name);
    return self;
}

void
snap_hero_tile_set_name (SnapHeroTile *self, const gchar *name)
{
    g_return_if_fail (SNAP_IS_HERO_TILE (self));

    g_free (self->name);
    self->name = g_strdup (name);

    gtk_label_set_label (self->name_label, name);
    gtk_label_set_label (self->summary_label, "Lorem Ipsum...");
    gtk_image_set_from_resource (self->icon_image, "/com/ubuntu/SnapStore/default-snap-icon.svg");
}
