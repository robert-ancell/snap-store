/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-app.h"
#include "store-app-tile.h"
#include "store-category-page.h"

struct _StoreCategoryPage
{
    StorePage parent_instance;

    GtkGrid *app_grid;
    GtkLabel *summary_label;
    GtkLabel *title_label;
};

G_DEFINE_TYPE (StoreCategoryPage, store_category_page, store_page_get_type ())

enum
{
    SIGNAL_APP_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
app_activated_cb (StoreCategoryPage *self, StoreAppTile *tile)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, store_app_tile_get_app (tile));
}

static void
store_category_page_set_model (StorePage *page, StoreModel *model)
{
    // FIXME: Should apply to children

    STORE_PAGE_CLASS (store_category_page_parent_class)->set_model (page, model);
}

static void
store_category_page_class_init (StoreCategoryPageClass *klass)
{
    STORE_PAGE_CLASS (klass)->set_model = store_category_page_set_model;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-category-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryPage, app_grid);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryPage, summary_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryPage, title_label);

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
store_category_page_init (StoreCategoryPage *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_category_page_set_category (StoreCategoryPage *self, StoreCategory *category)
{
    g_return_if_fail (STORE_IS_CATEGORY_PAGE (self));

    g_object_bind_property (category, "summary", self->summary_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property (category, "title", self->title_label, "label", G_BINDING_SYNC_CREATE);

    GPtrArray *apps = store_category_get_apps (category);

    /* Ensure correct number of app tiles */
    // FIXME: Make a new widget that does this
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->app_grid));
    guint n_tiles = g_list_length (children);
    while (n_tiles < apps->len) {
        StoreAppTile *tile = store_app_tile_new ();
        gtk_widget_show (GTK_WIDGET (tile));
        store_app_tile_set_model (tile, store_page_get_model (STORE_PAGE (self)));
        g_signal_connect_object (tile, "activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
        gtk_grid_attach (self->app_grid, GTK_WIDGET (tile), n_tiles % 3, n_tiles / 3, 1, 1);
        n_tiles++;
        children = g_list_append (children, tile);
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
