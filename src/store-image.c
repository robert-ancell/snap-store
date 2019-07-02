/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-image.h"

#include "store-cache.h"

struct _StoreImage
{
    GtkDrawingArea parent_instance;

    GByteArray *buffer;
    StoreCache *cache;
    GCancellable *cache_cancellable;
    GCancellable *cancellable;
    guint height;
    GdkPixbuf *pixbuf;
    guint width;
    SoupSession *session;
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
image_size_cb (StoreImage *self, gint width, gint height, GdkPixbufLoader *loader)
{
    if (self->width == 0 || self->height == 0)
        return;

    gint w, h;
    if (width * self->height > height * self->width) {
        w = self->width;
        h = height * self->width / width;
    }
    else {
        h = self->height;
        w = width * self->height / height;
    }

    gdk_pixbuf_loader_set_size (loader, w, h);
}

static gboolean
process_image (StoreImage *self, GBytes *data)
{
    g_autoptr(GdkPixbufLoader) loader = gdk_pixbuf_loader_new ();

    g_signal_connect_object (loader, "size-prepared", G_CALLBACK (image_size_cb), self, G_CONNECT_SWAPPED);

    g_autoptr(GError) error = NULL;
    if (!gdk_pixbuf_loader_write_bytes (loader, data, &error) ||
        !gdk_pixbuf_loader_close (loader, &error)) {
        g_warning ("Failed to decode image %s: %s", self->uri, error->message);
        return FALSE;
    }

    g_set_object (&self->pixbuf, gdk_pixbuf_loader_get_pixbuf (loader));

    gtk_widget_queue_draw (GTK_WIDGET (self));

    return TRUE;
}

static void
read_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreImage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GBytes) data = g_input_stream_read_bytes_finish (G_INPUT_STREAM (object), result, &error);
    if (data == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to download image %s: %s", self->uri, error->message);
        return;
    }

    g_byte_array_append (self->buffer, g_bytes_get_data (data, NULL), g_bytes_get_size (data));

    /* Read until EOF */
    if (g_bytes_get_size (data) != 0) {
        g_input_stream_read_bytes_async (G_INPUT_STREAM (object), 65535, G_PRIORITY_DEFAULT, self->cancellable, read_cb, self);
        return;
    }

    /* Cancel cached image if we got there first */
    g_cancellable_cancel (self->cache_cancellable);
    g_clear_object (&self->cache_cancellable);

    g_autoptr(GBytes) full_data = g_bytes_new_static (self->buffer->data, self->buffer->len);
    gboolean used = process_image (self, full_data);

    /* Save in cache */
    if (self->cache != NULL && used)
        store_cache_insert (self->cache, "images", self->uri, TRUE, full_data, self->cancellable, NULL); // FIXME: Report error?

    g_clear_pointer (&self->buffer, g_byte_array_unref);
}

static void
send_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreImage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GInputStream) stream = soup_session_send_finish (SOUP_SESSION (object), result, &error);
    if (stream == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to download image: %s", error->message);
        return;
    }

    g_input_stream_read_bytes_async (stream, 65535, G_PRIORITY_DEFAULT, self->cancellable, read_cb, self);
}

static void
cache_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreImage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GBytes) data = store_cache_lookup_finish (STORE_CACHE (object), result, &error);
    if (data == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
            g_warning ("Failed to load cached image: %s", error->message);
        return;
    }

    process_image (self, data);
}

static void
store_image_dispose (GObject *object)
{
    StoreImage *self = STORE_IMAGE (object);

    g_clear_pointer (&self->buffer, g_byte_array_unref);
    g_clear_object (&self->cache);
    g_cancellable_cancel (self->cache_cancellable);
    g_clear_object (&self->cache_cancellable);
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_clear_object (&self->pixbuf);
    g_clear_object (&self->session);
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
    *minimum_height = *natural_height = self->height;
}

static void
store_image_get_preferred_width (GtkWidget *widget, gint *minimum_width, gint *natural_width)
{
    StoreImage *self = STORE_IMAGE (widget);
    *minimum_width = *natural_width = self->width;
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
store_image_init (StoreImage *self)
{
    self->session = soup_session_new ();
}

StoreImage *
store_image_new (void)
{
    return g_object_new (store_image_get_type (), NULL);
}

void
store_image_set_cache (StoreImage *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_IMAGE (self));
    g_set_object (&self->cache, cache);
}

void
store_image_set_media (StoreImage *self, StoreMedia *media)
{
    g_return_if_fail (STORE_IS_IMAGE (self));

    store_image_set_uri (self, media != NULL ? store_media_get_uri (media) : NULL);
}

void
store_image_set_session (StoreImage *self, SoupSession *session)
{
    g_return_if_fail (STORE_IS_IMAGE (self));
    g_return_if_fail (SOUP_IS_SESSION (session));
    g_clear_object (&self->session);
    self->session = g_object_ref (session);
}

void
store_image_set_size (StoreImage *self, guint width, guint height)
{
    g_return_if_fail (STORE_IS_IMAGE (self));
    self->width = width;
    self->height = height;
    store_image_set_uri (self, self->uri);
}

void
store_image_set_uri (StoreImage *self, const gchar *uri)
{
    g_return_if_fail (STORE_IS_IMAGE (self));

    if (g_strcmp0 (uri, self->uri) == 0)
        return;
    g_free (self->uri);
    self->uri = g_strdup (uri);

    /* Cancel existing operation */
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_cancellable_cancel (self->cache_cancellable);
    g_clear_object (&self->cache_cancellable);

    g_clear_object (&self->pixbuf);
    if (uri == NULL || g_strcmp0 (uri, "") == 0) {
        self->pixbuf = gdk_pixbuf_new_from_resource_at_scale ("/io/snapcraft/Store/default-snap-icon.svg", self->width, self->height, TRUE, NULL); // FIXME: Make a property
        return;
    }

    g_clear_pointer (&self->buffer, g_byte_array_unref);
    self->buffer = g_byte_array_new ();

    g_autoptr(SoupMessage) message = soup_message_new ("GET", uri);
    self->cancellable = g_cancellable_new ();
    soup_session_send_async (self->session, message, self->cancellable, send_cb, self);

    /* Load cached version */
    if (self->cache != NULL) {
        self->cache_cancellable = g_cancellable_new ();
        store_cache_lookup_async (self->cache, "images", uri, TRUE, self->cache_cancellable, cache_cb, self);
    }
}
