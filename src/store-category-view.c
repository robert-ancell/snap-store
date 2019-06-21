/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-category-view.h"

#include "store-app-tile.h"
#include "store-hero-tile.h"

struct _StoreCategoryView
{
    GtkFlowBoxChild parent_instance;

    GtkFlowBox *app_flow_box;
    StoreHeroTile *hero_tile;
    GtkLabel *title_label;

    gchar *name;
};

G_DEFINE_TYPE (StoreCategoryView, store_category_view, GTK_TYPE_FLOW_BOX_CHILD)

static void
store_category_view_dispose (GObject *object)
{
    StoreCategoryView *self = STORE_CATEGORY_VIEW (object);
    g_clear_pointer (&self->name, g_free);
}

static void
store_category_view_class_init (StoreCategoryViewClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_category_view_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/store-category-view.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryView, app_flow_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryView, hero_tile);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryView, title_label);
}

static void
store_category_view_init (StoreCategoryView *self)
{
    store_hero_tile_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreCategoryView *
store_category_view_new (const gchar *name)
{
    StoreCategoryView *self = g_object_new (store_category_view_get_type (), NULL);
    store_category_view_set_name (self, name);
    return self;
}

void
store_category_view_set_name (StoreCategoryView *self, const gchar *name)
{
    g_return_if_fail (STORE_IS_CATEGORY_VIEW (self));

    g_free (self->name);
    self->name = g_strdup (name);

    gtk_label_set_label (self->title_label, name);
}

void
store_category_view_set_hero (StoreCategoryView *self, const gchar *name)
{
    g_return_if_fail (STORE_IS_CATEGORY_VIEW (self));

    store_hero_tile_set_name (self->hero_tile, name);
}

void
store_category_view_set_apps (StoreCategoryView *self, GStrv names)
{
    g_return_if_fail (STORE_IS_CATEGORY_VIEW (self));

    for (int i = 0; names[i] != NULL; i++) {
        StoreAppTile *tile = store_app_tile_new (names[i]);
        gtk_widget_show (GTK_WIDGET (tile));
        gtk_container_add (GTK_CONTAINER (self->app_flow_box), GTK_WIDGET (tile));
    }
}
