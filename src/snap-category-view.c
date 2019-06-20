/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "snap-category-view.h"

#include "snap-app-tile.h"
#include "snap-hero-tile.h"

struct _SnapCategoryView
{
    GtkFlowBoxChild parent_instance;

    GtkFlowBox *app_flow_box;
    SnapHeroTile *hero_tile;
    GtkLabel *title_label;

    gchar *name;
};

G_DEFINE_TYPE (SnapCategoryView, snap_category_view, GTK_TYPE_FLOW_BOX_CHILD)

static void
snap_category_view_dispose (GObject *object)
{
    SnapCategoryView *self = SNAP_CATEGORY_VIEW (object);
    g_clear_pointer (&self->name, g_free);
}

static void
snap_category_view_class_init (SnapCategoryViewClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = snap_category_view_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/snap-category-view.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapCategoryView, app_flow_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapCategoryView, hero_tile);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapCategoryView, title_label);
}

static void
snap_category_view_init (SnapCategoryView *self)
{
    snap_hero_tile_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

SnapCategoryView *
snap_category_view_new (const gchar *name)
{
    SnapCategoryView *self = g_object_new (snap_category_view_get_type (), NULL);
    snap_category_view_set_name (self, name);
    return self;
}

void
snap_category_view_set_name (SnapCategoryView *self, const gchar *name)
{
    g_return_if_fail (SNAP_IS_CATEGORY_VIEW (self));

    g_free (self->name);
    self->name = g_strdup (name);

    gtk_label_set_label (self->title_label, name);
}

void
snap_category_view_set_hero (SnapCategoryView *self, const gchar *name)
{
    g_return_if_fail (SNAP_IS_CATEGORY_VIEW (self));

    snap_hero_tile_set_name (self->hero_tile, name);
}

void
snap_category_view_set_apps (SnapCategoryView *self, GStrv names)
{
    g_return_if_fail (SNAP_IS_CATEGORY_VIEW (self));

    for (int i = 0; names[i] != NULL; i++) {
        SnapAppTile *tile = snap_app_tile_new (names[i]);
        gtk_widget_show (GTK_WIDGET (tile));
        gtk_container_add (GTK_CONTAINER (self->app_flow_box), GTK_WIDGET (tile));
    }
}
