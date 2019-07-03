/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-categories-page.h"
#include "store-category.h"
#include "store-category-tile.h"

struct _StoreCategoriesPage
{
    StorePage parent_instance;

    GtkGrid *category_grid;
};

enum
{
    PROP_0,
    PROP_CATEGORIES,
    PROP_LAST
};

G_DEFINE_TYPE (StoreCategoriesPage, store_categories_page, store_page_get_type ())

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
store_categories_page_get_property (GObject *object, guint prop_id, GValue *value G_GNUC_UNUSED, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
store_categories_page_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreCategoriesPage *self = STORE_CATEGORIES_PAGE (object);

    switch (prop_id)
    {
    case PROP_CATEGORIES:
        store_categories_page_set_categories (self, g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_categories_page_set_model (StorePage *page, StoreModel *model)
{
    StoreCategoriesPage *self = STORE_CATEGORIES_PAGE (page);

    g_object_bind_property (model, "categories", self, "categories", G_BINDING_SYNC_CREATE);

    STORE_PAGE_CLASS (store_categories_page_parent_class)->set_model (page, model);
}

static void
store_categories_page_class_init (StoreCategoriesPageClass *klass)
{
    G_OBJECT_CLASS (klass)->get_property = store_categories_page_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_categories_page_set_property;
    STORE_PAGE_CLASS (klass)->set_model = store_categories_page_set_model;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_CATEGORIES,
                                     g_param_spec_boxed ("categories", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_WRITABLE));

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
    // FIXME: Make a new widget that does this
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->category_grid));
    guint n_tiles = g_list_length (children);
    while (n_tiles < categories->len) {
        StoreCategoryTile *tile = store_category_tile_new ();
        gtk_widget_show (GTK_WIDGET (tile));
        g_signal_connect_object (tile, "activated", G_CALLBACK (category_activated_cb), self, G_CONNECT_SWAPPED);
        gtk_grid_attach (self->category_grid, GTK_WIDGET (tile), n_tiles % 3, n_tiles / 3, 1, 1);
        n_tiles++;
    }
    for (GList *link = children; link != NULL; link = link->next) {
        GtkWidget *child = link->data;
        int left_attach, top_attach;
        gtk_container_child_get (GTK_CONTAINER (self->category_grid), child, "left-attach", &left_attach, "top-attach", &top_attach, NULL);
        guint index = top_attach * 3 + left_attach;
        if (index >= categories->len)
            gtk_container_remove (GTK_CONTAINER (self->category_grid), child);
    }

    for (guint i = 0; i < categories->len; i++) {
        StoreCategory *category = g_ptr_array_index (categories, i);
        StoreCategoryTile *tile = STORE_CATEGORY_TILE (gtk_grid_get_child_at (self->category_grid, i % 3, i / 3));
        store_category_tile_set_category (tile, category);
    }
}
