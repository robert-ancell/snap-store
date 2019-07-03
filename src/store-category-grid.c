/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-category-grid.h"

#include "store-app-tile.h"

struct _StoreCategoryGrid
{
    GtkBox parent_instance;

    GtkGrid *app_grid;
    GtkLabel *title_label;

    StoreModel *model;
    gchar *name;
};

G_DEFINE_TYPE (StoreCategoryGrid, store_category_grid, GTK_TYPE_BOX)

enum
{
    SIGNAL_APP_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
app_activated_cb (StoreCategoryGrid *self, StoreAppTile *tile)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, store_app_tile_get_app (tile));
}

static void
store_category_grid_dispose (GObject *object)
{
    StoreCategoryGrid *self = STORE_CATEGORY_GRID (object);

    g_clear_object (&self->model);
    g_clear_pointer (&self->name, g_free);

    G_OBJECT_CLASS (store_category_grid_parent_class)->dispose (object);
}

static void
store_category_grid_class_init (StoreCategoryGridClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_category_grid_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-category-grid.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryGrid, app_grid);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryGrid, title_label);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), app_activated_cb);

    signals[SIGNAL_APP_ACTIVATED] = g_signal_new ("app-activated",
                                                  G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                                  G_SIGNAL_RUN_LAST,
                                                  0,
                                                  NULL, NULL,
                                                  NULL,
                                                  G_TYPE_NONE,
                                                  1, store_app_get_type ());
}

static void
store_category_grid_init (StoreCategoryGrid *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreCategoryGrid *
store_category_grid_new (void)
{
    return g_object_new (store_category_grid_get_type (), NULL);
}

void
store_category_grid_set_model (StoreCategoryGrid *self, StoreModel *model)
{
    g_return_if_fail (STORE_IS_CATEGORY_GRID (self));

    g_set_object (&self->model, model);
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->app_grid));
    for (GList *link = children; link != NULL; link = link->next) {
        StoreAppTile *tile = link->data;
        store_app_tile_set_model (tile, model);
    }
}

void
store_category_grid_set_name (StoreCategoryGrid *self, const gchar *name)
{
    g_return_if_fail (STORE_IS_CATEGORY_GRID (self));

    g_free (self->name);
    self->name = g_strdup (name);

    gtk_label_set_label (self->title_label, name);
    gtk_widget_set_visible (GTK_WIDGET (self->title_label), name != NULL);
}

const gchar *
store_category_grid_get_name (StoreCategoryGrid *self)
{
    g_return_val_if_fail (STORE_IS_CATEGORY_GRID (self), NULL);
    return self->name;
}

void
store_category_grid_set_title (StoreCategoryGrid *self, const gchar *title)
{
    g_return_if_fail (STORE_IS_CATEGORY_GRID (self));

    gtk_label_set_label (self->title_label, title);
    gtk_widget_set_visible (GTK_WIDGET (self->title_label), title != NULL);
}

void
store_category_grid_set_apps (StoreCategoryGrid *self, GPtrArray *apps)
{
    g_return_if_fail (STORE_IS_CATEGORY_GRID (self));

    /* Ensure correct number of app tiles */
    // FIXME: Make a new widget that does this
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->app_grid));
    guint n_tiles = g_list_length (children);
    while (n_tiles < apps->len) {
        StoreAppTile *tile = store_app_tile_new ();
        gtk_widget_show (GTK_WIDGET (tile));
        g_signal_connect_object (tile, "activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
        store_app_tile_set_model (tile, self->model);
        gtk_grid_attach (self->app_grid, GTK_WIDGET (tile), n_tiles % 3, n_tiles / 3, 1, 1);
        n_tiles++;
    }
    for (GList *link = children; link != NULL; link = link->next) {
        GtkWidget *child = link->data;
        int left_attach, top_attach;
        gtk_container_child_get (GTK_CONTAINER (self->app_grid), child, "left-attach", &left_attach, "top-attach", &top_attach, NULL);
        guint index = top_attach * 3 + left_attach;
        if (index >= apps->len)
            gtk_container_remove (GTK_CONTAINER (self->app_grid), child);
    }

    for (guint i = 0; i < apps->len; i++) {
        StoreApp *app = g_ptr_array_index (apps, i);
        StoreAppTile *tile = STORE_APP_TILE (gtk_grid_get_child_at (self->app_grid, i % 3, i / 3));
        store_app_tile_set_app (tile, app);
    }
}
