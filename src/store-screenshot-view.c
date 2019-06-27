/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-screenshot-view.h"

#include "store-image.h"

struct _StoreScreenshotView
{
    GtkBox parent_instance;

    GtkBox *screenshots_box;

    StoreApp *app;
    StoreCache *cache;
};

G_DEFINE_TYPE (StoreScreenshotView, store_screenshot_view, GTK_TYPE_BOX)

static void
store_screenshot_view_dispose (GObject *object)
{
    StoreScreenshotView *self = STORE_SCREENSHOT_VIEW (object);

    g_clear_object (&self->cache);
    g_clear_object (&self->app);

    G_OBJECT_CLASS (store_screenshot_view_parent_class)->dispose (object);
}

static void
store_screenshot_view_class_init (StoreScreenshotViewClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_screenshot_view_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-screenshot-view.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreScreenshotView, screenshots_box);
}

static void
store_screenshot_view_init (StoreScreenshotView *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreScreenshotView *
store_screenshot_view_new (void)
{
    return g_object_new (store_screenshot_view_get_type (), NULL);
}

void
store_screenshot_view_set_cache (StoreScreenshotView *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_SCREENSHOT_VIEW (self));

    g_set_object (&self->cache, cache);
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->screenshots_box));
    for (GList *link = children; link != NULL; link = link->next) {
        StoreImage *image = link->data;
        store_image_set_cache (image, cache);
    }
}

void
store_screenshot_view_set_app (StoreScreenshotView *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_SCREENSHOT_VIEW (self));

    if (self->app == app)
        return;

    g_set_object (&self->app, app);

    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->screenshots_box));
    for (GList *link = children; link != NULL; link = link->next) {
        GtkWidget *child = link->data;
        gtk_container_remove (GTK_CONTAINER (self->screenshots_box), child);
    }
    GPtrArray *screenshots = store_app_get_screenshots (app);
    for (guint i = 0; i < screenshots->len; i++) {
        StoreMedia *screenshot = g_ptr_array_index (screenshots, i);
        StoreImage *image = store_image_new ();
        gtk_widget_show (GTK_WIDGET (image));
        store_image_set_cache (image, self->cache);
        store_image_set_uri (image, store_media_get_uri (screenshot));
        guint width, height = 500;
        if (store_media_get_width (screenshot) > 0 && store_media_get_height (screenshot) > 0)
            width = store_media_get_width (screenshot) * height / store_media_get_height (screenshot);
        else
            width = height;
        store_image_set_size (image, width, height);
        gtk_container_add (GTK_CONTAINER (self->screenshots_box), GTK_WIDGET (image));
    }
    gtk_widget_set_visible (GTK_WIDGET (self->screenshots_box), screenshots->len > 0);
}
