/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <libsoup/soup.h>

#include "store-image.h"

struct _StoreImage
{
    GtkImage parent_instance;

    GCancellable *cancellable;

    int size;
    gchar *url;
};

G_DEFINE_TYPE (StoreImage, store_image, GTK_TYPE_IMAGE)

static void
store_image_dispose (GObject *object)
{
    StoreImage *self = STORE_IMAGE (object);
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_clear_pointer (&self->url, g_free);   
}

static void
store_image_class_init (StoreImageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_image_dispose;
}

static void
store_image_init (StoreImage *self)
{
    store_image_set_size (self, 64);
}

StoreImage *
store_image_new (void)
{
    return g_object_new (store_image_get_type (), NULL);
}

static void
pixbuf_cb (GObject *object G_GNUC_UNUSED, GAsyncResult *result, gpointer user_data)
{
    StoreImage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GdkPixbuf) pixbuf = gdk_pixbuf_new_from_stream_finish (result, &error);
    if (pixbuf == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to decode image: %s", error->message);
        return;
    }

    gtk_image_set_from_pixbuf (GTK_IMAGE (self), pixbuf);
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

    gdk_pixbuf_new_from_stream_at_scale_async (stream, self->size, self->size, TRUE, self->cancellable, pixbuf_cb, self);
}

void
store_image_set_size (StoreImage *self, guint size)
{
    g_return_if_fail (STORE_IS_IMAGE (self));
    self->size = size;
    store_image_set_url (self, self->url);
}

void
store_image_set_url (StoreImage *self, const gchar *url)
{
    g_return_if_fail (STORE_IS_IMAGE (self));

    g_free (self->url);
    self->url = g_strdup (url);

    /* Cancel existing operation */
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    self->cancellable = g_cancellable_new ();

    if (url == NULL || g_strcmp0 (url, "") == 0) {
        g_autoptr(GdkPixbuf) pixbuf = gdk_pixbuf_new_from_resource_at_scale ("/com/ubuntu/SnapStore/default-snap-icon.svg", self->size, self->size, TRUE, NULL); // FIXME: Make a property
        gtk_image_set_from_pixbuf (GTK_IMAGE (self), pixbuf);
    }
    else {
        g_autoptr(SoupSession) session = soup_session_new (); // FIXME: common session?
        g_autoptr(SoupMessage) message = soup_message_new ("GET", url);
        soup_session_send_async (session, message, self->cancellable, send_cb, self);
    }
}
