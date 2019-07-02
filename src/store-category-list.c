/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-category-list.h"

#include "store-app-small-tile.h"

struct _StoreCategoryList
{
    GtkBox parent_instance;

    GtkBox *app_box;
    GtkLabel *title_label;

    StoreCache *cache;
    gchar *name;
};

G_DEFINE_TYPE (StoreCategoryList, store_category_list, GTK_TYPE_BOX)

enum
{
    SIGNAL_APP_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
app_activated_cb (StoreCategoryList *self, StoreAppSmallTile *tile)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, store_app_small_tile_get_app (tile));
}

static void
store_category_list_dispose (GObject *object)
{
    StoreCategoryList *self = STORE_CATEGORY_LIST (object);

    g_clear_object (&self->cache);
    g_clear_pointer (&self->name, g_free);

    G_OBJECT_CLASS (store_category_list_parent_class)->dispose (object);
}

static void
store_category_list_class_init (StoreCategoryListClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_category_list_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-category-list.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryList, app_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryList, title_label);

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
store_category_list_init (StoreCategoryList *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreCategoryList *
store_category_list_new (void)
{
    return g_object_new (store_category_list_get_type (), NULL);
}

void
store_category_list_set_cache (StoreCategoryList *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_CATEGORY_LIST (self));

    g_set_object (&self->cache, cache);
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->app_box));
    for (GList *link = children; link != NULL; link = link->next) {
        StoreAppSmallTile *tile = link->data;
        store_app_small_tile_set_cache (tile, cache);
    }
}

void
store_category_list_set_name (StoreCategoryList *self, const gchar *name)
{
    g_return_if_fail (STORE_IS_CATEGORY_LIST (self));

    g_free (self->name);
    self->name = g_strdup (name);

    gtk_label_set_label (self->title_label, name);
    gtk_widget_set_visible (GTK_WIDGET (self->title_label), name != NULL);
}

const gchar *
store_category_list_get_name (StoreCategoryList *self)
{
    g_return_val_if_fail (STORE_IS_CATEGORY_LIST (self), NULL);
    return self->name;
}

void
store_category_list_set_title (StoreCategoryList *self, const gchar *title)
{
    g_return_if_fail (STORE_IS_CATEGORY_LIST (self));

    gtk_label_set_label (self->title_label, title);
    gtk_widget_set_visible (GTK_WIDGET (self->title_label), title != NULL);
}

void
store_category_list_set_apps (StoreCategoryList *self, GPtrArray *apps)
{
    g_return_if_fail (STORE_IS_CATEGORY_LIST (self));

    /* Ensure correct number of app tiles */
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->app_box));
    guint n_tiles = g_list_length (children);
    while (n_tiles < apps->len) {
        StoreAppSmallTile *tile = store_app_small_tile_new ();
        gtk_widget_show (GTK_WIDGET (tile));
        g_signal_connect_object (tile, "activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
        store_app_small_tile_set_cache (tile, self->cache);
        gtk_container_add (GTK_CONTAINER (self->app_box), GTK_WIDGET (tile));
        n_tiles++;
        children = g_list_append (children, tile);
    }
    while (n_tiles > apps->len) {
        for (GList *link = g_list_nth (children, apps->len); link != NULL; link = link->next) {
            StoreAppSmallTile *tile = link->data;
            gtk_container_remove (GTK_CONTAINER (self->app_box), GTK_WIDGET (tile));
        }
    }
    for (guint i = 0; i < apps->len; i++) {
        StoreApp *app = g_ptr_array_index (apps, i);
        StoreAppSmallTile *tile = g_list_nth_data (children, i);
        store_app_small_tile_set_app (tile, app);
    }
}
