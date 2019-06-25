/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-app.h"

typedef struct
{
    gchar *appstream_id;
    GPtrArray *channels;
    gchar *description;
    StoreMedia *icon;
    gchar *name;
    gchar *publisher;
    gboolean publisher_validated;
    GPtrArray *screenshots;
    gchar *summary;
    gchar *title;
} StoreAppPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (StoreApp, store_app, G_TYPE_OBJECT)

static void
store_app_dispose (GObject *object)
{
    StoreApp *self = STORE_APP (object);
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_clear_pointer (&priv->appstream_id, g_free);
    g_clear_pointer (&priv->channels, g_ptr_array_unref);
    g_clear_pointer (&priv->description, g_free);
    g_clear_object (&priv->icon);
    g_clear_pointer (&priv->name, g_free);
    g_clear_pointer (&priv->publisher, g_free);
    g_clear_pointer (&priv->screenshots, g_ptr_array_unref);
    g_clear_pointer (&priv->summary, g_free);
    g_clear_pointer (&priv->title, g_free);

    G_OBJECT_CLASS (store_app_parent_class)->dispose (object);
}

static void
store_app_class_init (StoreAppClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_dispose;
}

static void
store_app_init (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    priv->appstream_id = g_strdup ("");
    priv->channels = g_ptr_array_new ();
    priv->description = g_strdup ("");
    priv->publisher = g_strdup ("");
    priv->screenshots = g_ptr_array_new ();
    priv->summary = g_strdup ("");
    priv->title = g_strdup ("");
}

void
store_app_refresh_async (StoreApp *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_APP (self));
    STORE_APP_GET_CLASS (self)->refresh_async (self, cancellable, callback, callback_data);
}

gboolean
store_app_refresh_finish (StoreApp *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_APP (self), FALSE);
    return STORE_APP_GET_CLASS (self)->refresh_finish (self, result, error);
}

void
store_app_save_to_cache (StoreApp *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_APP (self));
    STORE_APP_GET_CLASS (self)->save_to_cache (self, cache);
}

void
store_app_update_from_cache (StoreApp *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_APP (self));
    STORE_APP_GET_CLASS (self)->update_from_cache (self, cache);
}

void
store_app_set_appstream_id (StoreApp *self, const gchar *appstream_id)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->appstream_id, g_free);
    priv->appstream_id = g_strdup (appstream_id);
}

const gchar *
store_app_get_appstream_id (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->appstream_id;
}

void
store_app_set_channels (StoreApp *self, GPtrArray *channels)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->channels, g_ptr_array_unref);
    if (channels != NULL)
        priv->channels = g_ptr_array_ref (channels);
}

GPtrArray *
store_app_get_channels (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->channels;
}

void
store_app_set_description (StoreApp *self, const gchar *description)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->description, g_free);
    priv->description = g_strdup (description);
}

const gchar *
store_app_get_description (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->description;
}

void
store_app_set_icon (StoreApp *self, StoreMedia *icon)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_object (&priv->icon);
    if (icon != NULL)
        priv->icon = g_object_ref (icon);
}

StoreMedia *
store_app_get_icon (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->icon;
}

void
store_app_set_name (StoreApp *self, const gchar *name)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->name, g_free);
    priv->name = g_strdup (name);
}

const gchar *
store_app_get_name (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->name;
}

void
store_app_set_publisher (StoreApp *self, const gchar *publisher)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->publisher, g_free);
    priv->publisher = g_strdup (publisher);
}

const gchar *
store_app_get_publisher (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->publisher;
}

void
store_app_set_publisher_validated (StoreApp *self, gboolean validated)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    priv->publisher_validated = validated;
}

gboolean
store_app_get_publisher_validated (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), FALSE);

    return priv->publisher_validated;
}

void
store_app_set_screenshots (StoreApp *self, GPtrArray *screenshots)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->screenshots, g_ptr_array_unref);
    if (screenshots != NULL)
        priv->screenshots = g_ptr_array_ref (screenshots);
}

GPtrArray *
store_app_get_screenshots (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->screenshots;
}

void
store_app_set_summary (StoreApp *self, const gchar *summary)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->summary, g_free);
    priv->summary = g_strdup (summary);
}

const gchar *
store_app_get_summary (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->summary;
}

void
store_app_set_title (StoreApp *self, const gchar *title)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->title, g_free);
    priv->title = g_strdup (title);
}

const gchar *
store_app_get_title (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->title;
}
