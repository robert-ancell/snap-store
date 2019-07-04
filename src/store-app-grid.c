/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-app-grid.h"

#include "store-app-tile.h"

struct _StoreAppGrid
{
    GtkGrid parent_instance;

    GtkGrid *app_grid;

    StoreModel *model;
};

G_DEFINE_TYPE (StoreAppGrid, store_app_grid, GTK_TYPE_GRID)

enum
{
    SIGNAL_APP_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
app_activated_cb (StoreAppGrid *self, StoreAppTile *tile)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, store_app_tile_get_app (tile));
}

static void
store_app_grid_dispose (GObject *object)
{
    StoreAppGrid *self = STORE_APP_GRID (object);

    g_clear_object (&self->model);

    G_OBJECT_CLASS (store_app_grid_parent_class)->dispose (object);
}

static void
store_app_grid_class_init (StoreAppGridClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_grid_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-app-grid.ui");

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
store_app_grid_init (StoreAppGrid *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreAppGrid *
store_app_grid_new (void)
{
    return g_object_new (store_app_grid_get_type (), NULL);
}

void
store_app_grid_set_apps (StoreAppGrid *self, GPtrArray *apps)
{
    g_return_if_fail (STORE_IS_APP_GRID (self));

    /* Ensure correct number of app tiles */
    // FIXME: Make a new widget that does this
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self));
    guint n_tiles = g_list_length (children);
    while (n_tiles < apps->len) {
        StoreAppTile *tile = store_app_tile_new ();
        gtk_widget_show (GTK_WIDGET (tile));
        g_signal_connect_object (tile, "activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
        store_app_tile_set_model (tile, self->model);
        gtk_grid_attach (self, GTK_WIDGET (tile), n_tiles % 3, n_tiles / 3, 1, 1);
        n_tiles++;
    }
    for (GList *link = children; link != NULL; link = link->next) {
        GtkWidget *child = link->data;
        int left_attach, top_attach;
        gtk_container_child_get (GTK_CONTAINER (self), child, "left-attach", &left_attach, "top-attach", &top_attach, NULL);
        guint index = top_attach * 3 + left_attach;
        if (index >= apps->len)
            gtk_container_remove (GTK_CONTAINER (self), child);
    }

    for (guint i = 0; i < apps->len; i++) {
        StoreApp *app = g_ptr_array_index (apps, i);
        StoreAppTile *tile = STORE_APP_TILE (gtk_grid_get_child_at (self, i % 3, i / 3));
        store_app_tile_set_app (tile, app);
    }
}

void
store_app_grid_set_model (StoreAppGrid *self, StoreModel *model)
{
    g_return_if_fail (STORE_IS_APP_GRID (self));

    g_set_object (&self->model, model);
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self));
    for (GList *link = children; link != NULL; link = link->next) {
        StoreAppTile *tile = link->data;
        store_app_tile_set_model (tile, model);
    }
}
