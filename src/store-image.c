/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-image.h"

#include "store-model.h"

struct _StoreImage
{
    GtkDrawingArea parent_instance;

    GCancellable *cache_cancellable;
    GCancellable *cancellable;
    guint height;
    StoreModel *model;
    GdkPixbuf *pixbuf;
    guint width;
    gchar *uri;
};

enum
{
    PROP_0,
    PROP_HEIGHT,
    PROP_MEDIA,
    PROP_WIDTH,
    PROP_URI,
    PROP_LAST
};

G_DEFINE_TYPE (StoreImage, store_image, GTK_TYPE_DRAWING_AREA)

static void
set_pixbuf (StoreImage *self, GdkPixbuf *pixbuf)
{
    g_set_object (&self->pixbuf, pixbuf);
    gtk_widget_queue_resize (GTK_WIDGET (self));
    gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
image_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreImage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GdkPixbuf) pixbuf = store_model_get_image_finish (STORE_MODEL (object), result, &error);
    if (pixbuf == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to load image: %s", error->message);
        return;
    }

    /* Cancel cached image if we got there first */
    g_cancellable_cancel (self->cache_cancellable);
    g_clear_object (&self->cache_cancellable);

    set_pixbuf (self, pixbuf);
}

static void
cache_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreImage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GdkPixbuf) pixbuf = store_model_get_cached_image_finish (STORE_MODEL (object), result, &error);
    if (pixbuf == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
            g_warning ("Failed to load cached image: %s", error->message);
        return;
    }

    set_pixbuf (self, pixbuf);
}

static void
store_image_dispose (GObject *object)
{
    StoreImage *self = STORE_IMAGE (object);

    g_cancellable_cancel (self->cache_cancellable);
    g_clear_object (&self->cache_cancellable);
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_clear_object (&self->model);
    g_clear_object (&self->pixbuf);
    g_clear_pointer (&self->uri, g_free);

    G_OBJECT_CLASS (store_image_parent_class)->dispose (object);
}

static void
store_image_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    StoreImage *self = STORE_IMAGE (object);

    switch (prop_id)
    {
    case PROP_HEIGHT:
        g_value_set_int (value, self->height);
        break;
    case PROP_WIDTH:
        g_value_set_int (value, self->width);
        break;
    case PROP_URI:
        g_value_set_string (value, self->uri);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_image_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreImage *self = STORE_IMAGE (object);

    switch (prop_id)
    {
    case PROP_HEIGHT:
        self->height = g_value_get_int (value);
        break;
    case PROP_MEDIA:
        store_image_set_media (self, g_value_get_object (value));
        break;
    case PROP_WIDTH:
        self->width = g_value_get_int (value);
        break;
    case PROP_URI:
        store_image_set_uri (self, g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_image_get_preferred_height (GtkWidget *widget, gint *minimum_height, gint *natural_height)
{
    StoreImage *self = STORE_IMAGE (widget);

    gint height = self->height;
    if (height == 0 && self->pixbuf != NULL && gdk_pixbuf_get_width (self->pixbuf) != 0)
        height = gdk_pixbuf_get_height (self->pixbuf) * self->width / gdk_pixbuf_get_width (self->pixbuf);
    *minimum_height = *natural_height = height;
}

static void
store_image_get_preferred_width (GtkWidget *widget, gint *minimum_width, gint *natural_width)
{
    StoreImage *self = STORE_IMAGE (widget);

    gint width = self->width;
    if (width == 0 && self->pixbuf != NULL && gdk_pixbuf_get_height (self->pixbuf) != 0)
        width = gdk_pixbuf_get_width (self->pixbuf) * self->height / gdk_pixbuf_get_height (self->pixbuf);
    *minimum_width = *natural_width = width;
}

static gboolean
store_image_draw (GtkWidget *widget, cairo_t *cr)
{
    StoreImage *self = STORE_IMAGE (widget);

    if (self->pixbuf == NULL)
        return FALSE;

    int width = gtk_widget_get_allocated_width (widget);
    int height = gtk_widget_get_allocated_height (widget);
    g_autoptr(GdkPixbuf) pixbuf = gdk_pixbuf_scale_simple (self->pixbuf, width, height, GDK_INTERP_BILINEAR); // FIXME: Super inefficient
    gtk_render_icon (gtk_widget_get_style_context (widget), cr, pixbuf, 0, 0); // FIXME: Store surface instead of sending pixbuf each time to GPU

    return TRUE;
}

static void
store_image_class_init (StoreImageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_image_dispose;
    G_OBJECT_CLASS (klass)->get_property = store_image_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_image_set_property;

    GTK_WIDGET_CLASS (klass)->get_preferred_height = store_image_get_preferred_height;
    GTK_WIDGET_CLASS (klass)->get_preferred_width = store_image_get_preferred_width;
    GTK_WIDGET_CLASS (klass)->draw = store_image_draw;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_HEIGHT,
                                     g_param_spec_int ("height", NULL, NULL, G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_MEDIA,
                                     g_param_spec_object ("media", NULL, NULL, store_media_get_type (), G_PARAM_WRITABLE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_WIDTH,
                                     g_param_spec_int ("width", NULL, NULL, G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_URI,
                                     g_param_spec_string ("uri", NULL, NULL, NULL, G_PARAM_READWRITE));
}

static void
store_image_init (StoreImage *self G_GNUC_UNUSED)
{
}

StoreImage *
store_image_new (void)
{
    return g_object_new (store_image_get_type (), NULL);
}

void
store_image_set_media (StoreImage *self, StoreMedia *media)
{
    g_return_if_fail (STORE_IS_IMAGE (self));

    store_image_set_uri (self, media != NULL ? store_media_get_uri (media) : NULL);
}

void
store_image_set_model (StoreImage *self, StoreModel *model)
{
    g_return_if_fail (STORE_IS_IMAGE (self));
    g_set_object (&self->model, model);
}

void
store_image_set_size (StoreImage *self, guint width, guint height)
{
    g_return_if_fail (STORE_IS_IMAGE (self));
    self->width = width;
    self->height = height;
    store_image_set_uri (self, self->uri);

    gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
store_image_set_uri (StoreImage *self, const gchar *uri)
{
    g_return_if_fail (STORE_IS_IMAGE (self));

    if (self->pixbuf != NULL && g_strcmp0 (uri, self->uri) == 0)
        return;
    g_free (self->uri);
    self->uri = g_strdup (uri);

    /* Cancel existing operation */
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_cancellable_cancel (self->cache_cancellable);
    g_clear_object (&self->cache_cancellable);

    g_autoptr(GdkPixbuf) pixbuf = gdk_pixbuf_new_from_resource_at_scale ("/io/snapcraft/Store/default-snap-icon.svg", self->width, self->height, TRUE, NULL); // FIXME: Make a property
    set_pixbuf (self, pixbuf);

    if (uri == NULL)
        return;

    /* Load cache information */
    g_autofree gchar *etag = NULL;
    g_autoptr(GError) error = NULL;
    if (!store_model_get_cached_image_metadata_sync (self->model, uri, &etag, NULL, NULL, self->cancellable, &error))
        g_warning ("Failed to cached image metadata: %s", error->message);

    self->cancellable = g_cancellable_new ();
    store_model_get_image_async (self->model, uri, etag, self->width, self->height, self->cancellable, image_cb, self);

    /* Load cached version */
    self->cache_cancellable = g_cancellable_new ();
    store_model_get_cached_image_async (self->model, uri, self->width, self->height, self->cache_cancellable, cache_cb, self);
}
