/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-category-tile.h"
#include "store-image.h"

struct _StoreCategoryTile
{
    GtkEventBox parent_instance;

    StoreImage *icon_image;
    GtkLabel *title_label;

    StoreCategory *category;
};

G_DEFINE_TYPE (StoreCategoryTile, store_category_tile, GTK_TYPE_EVENT_BOX)

enum
{
    SIGNAL_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

static gboolean
button_release_event_cb (StoreCategoryTile *self)
{
    g_signal_emit (self, signals[SIGNAL_ACTIVATED], 0);
    return TRUE;
}

static void
store_category_tile_dispose (GObject *object)
{
    StoreCategoryTile *self = STORE_CATEGORY_TILE (object);

    g_clear_object (&self->category);

    G_OBJECT_CLASS (store_category_tile_parent_class)->dispose (object);
}

static void
store_category_tile_class_init (StoreCategoryTileClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_category_tile_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-category-tile.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryTile, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreCategoryTile, title_label);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), button_release_event_cb);

    signals[SIGNAL_ACTIVATED] = g_signal_new ("activated",
                                              G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                              G_SIGNAL_RUN_LAST,
                                              0,
                                              NULL, NULL,
                                              NULL,
                                              G_TYPE_NONE,
                                              0);
}

static void
store_category_tile_init (StoreCategoryTile *self)
{
    store_image_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreCategoryTile *
store_category_tile_new (void)
{
    return g_object_new (store_category_tile_get_type (), NULL);
}

void
store_category_tile_set_category (StoreCategoryTile *self, StoreCategory *category)
{
    g_return_if_fail (STORE_IS_CATEGORY_TILE (self));

    if (self->category == category)
        return;

    g_set_object (&self->category, category);

    g_object_bind_property (category, "title", self->title_label, "label", G_BINDING_SYNC_CREATE);
}

StoreCategory *
store_category_tile_get_category (StoreCategoryTile *self)
{
    g_return_val_if_fail (STORE_IS_CATEGORY_TILE (self), NULL);
    return self->category;
}
