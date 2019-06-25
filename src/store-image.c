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
    GtkImage parent_instance;

    GByteArray *buffer;
    StoreCache *cache;
    GCancellable *cancellable;
    guint height;
    guint width;
    SoupSession *session;
    gchar *url;
};

G_DEFINE_TYPE (StoreImage, store_image, GTK_TYPE_IMAGE)

static void
store_image_dispose (GObject *object)
{
    StoreImage *self = STORE_IMAGE (object);
    g_clear_pointer (&self->buffer, g_byte_array_unref);
    g_clear_object (&self->cache);
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_clear_object (&self->session);
    g_clear_pointer (&self->url, g_free);

    G_OBJECT_CLASS (store_image_parent_class)->dispose (object);
}

static void
store_image_class_init (StoreImageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_image_dispose;
}

static void
store_image_init (StoreImage *self)
{
    self->cache = store_cache_new (); // FIXME: Make shared?
    self->session = soup_session_new ();
    store_image_set_size (self, 64, 64); // FIXME: Hard-coded
}

StoreImage *
store_image_new (void)
{
    return g_object_new (store_image_get_type (), NULL);
}

static void
image_size_cb (StoreImage *self, gint width, gint height, GdkPixbufLoader *loader)
{
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
        g_warning ("Failed to decode image %s: %s", self->url, error->message);
        return FALSE;
    }

    gtk_image_set_from_pixbuf (GTK_IMAGE (self), gdk_pixbuf_loader_get_pixbuf (loader));
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
        g_warning ("Failed to download image %s: %s", self->url, error->message);
        return;
    }

    g_byte_array_append (self->buffer, g_bytes_get_data (data, NULL), g_bytes_get_size (data));

    /* Read until EOF */
    if (g_bytes_get_size (data) != 0) {
        g_input_stream_read_bytes_async (G_INPUT_STREAM (object), 65535, G_PRIORITY_DEFAULT, self->cancellable, read_cb, self);
        return;
    }

    g_autoptr(GBytes) full_data = g_bytes_new_static (self->buffer->data, self->buffer->len);
    gboolean used = process_image (self, full_data);

    /* Save in cache */
    if (used)
        store_cache_insert (self->cache, "images", self->url, TRUE, full_data);

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
    store_image_set_url (self, self->url);
}

void
store_image_set_url (StoreImage *self, const gchar *url)
{
    g_return_if_fail (STORE_IS_IMAGE (self));

    if (url != self->url) {
        g_free (self->url);
        self->url = g_strdup (url);
    }

    /* Cancel existing operation */
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    self->cancellable = g_cancellable_new ();

    if (url == NULL || g_strcmp0 (url, "") == 0) {
        g_autoptr(GdkPixbuf) pixbuf = gdk_pixbuf_new_from_resource_at_scale ("/io/snapcraft/Store/default-snap-icon.svg", self->width, self->height, TRUE, NULL); // FIXME: Make a property
        gtk_image_set_from_pixbuf (GTK_IMAGE (self), pixbuf);
        return;
    }

    g_clear_pointer (&self->buffer, g_byte_array_unref);
    self->buffer = g_byte_array_new ();

    g_autoptr(SoupMessage) message = soup_message_new ("GET", url);
    soup_session_send_async (self->session, message, self->cancellable, send_cb, self);

    /* Load cached version */
    g_autoptr(GBytes) data = store_cache_lookup (self->cache, "images", url, TRUE);
    if (data != NULL)
        process_image (self, data);
}
