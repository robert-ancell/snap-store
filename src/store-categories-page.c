/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <glib/gi18n.h>

#include "store-app.h"
#include "store-categories-page.h"
#include "store-category.h"
#include "store-category-tile.h"

struct _StoreCategoriesPage
{
    GtkBox parent_instance;

    GtkGrid *category_grid;
};

G_DEFINE_TYPE (StoreCategoriesPage, store_categories_page, GTK_TYPE_BOX)

enum
{
    SIGNAL_CATEGORY_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
category_activated_cb (StoreCategoriesPage *self, StoreCategoryTile *tile)
{
    g_signal_emit (self, signals[SIGNAL_CATEGORY_ACTIVATED], 0, store_category_tile_get_category (tile));
}

static void
store_categories_page_class_init (StoreCategoriesPageClass *klass)
{
    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-categories-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoriesPage, category_grid);

    signals[SIGNAL_CATEGORY_ACTIVATED] = g_signal_new ("category-activated",
                                                       G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                                       G_SIGNAL_RUN_LAST,
                                                       0,
                                                       NULL, NULL,
                                                       NULL,
                                                       G_TYPE_NONE,
                                                       1, store_category_get_type ());
}

static void
store_categories_page_init (StoreCategoriesPage *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_categories_page_set_categories (StoreCategoriesPage *self, GPtrArray *categories)
{
    g_return_if_fail (STORE_IS_CATEGORIES_PAGE (self));

    /* Ensure correct number of category tiles */
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->category_grid));
    guint n_tiles = g_list_length (children);
    while (n_tiles < categories->len) {
        StoreCategoryTile *tile = store_category_tile_new ();
        gtk_widget_show (GTK_WIDGET (tile));
        g_signal_connect_object (tile, "activated", G_CALLBACK (category_activated_cb), self, G_CONNECT_SWAPPED);
        gtk_grid_attach (self->category_grid, GTK_WIDGET (tile), n_tiles % 3, n_tiles / 3, 1, 1);
        n_tiles++;
        children = g_list_append (children, tile);
    }
    while (n_tiles > categories->len) {
        for (GList *link = g_list_nth (children, categories->len); link != NULL; link = link->next) {
            StoreCategoryTile *tile = link->data;
            gtk_container_remove (GTK_CONTAINER (self->category_grid), GTK_WIDGET (tile));
        }
    }
    for (guint i = 0; i < categories->len; i++) {
        StoreCategory *category = g_ptr_array_index (categories, i);
        StoreCategoryTile *tile = g_list_nth_data (children, i);
        store_category_tile_set_category (tile, category);
    }
}
