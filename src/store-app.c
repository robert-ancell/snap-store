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
    gchar *contact;
    gchar *description;
    StoreMedia *icon;
    gchar *name;
    gchar *publisher;
    gboolean publisher_validated;
    GPtrArray *screenshots;
    gchar *summary;
    gchar *title;
} StoreAppPrivate;

enum
{
    PROP_0,
    PROP_CHANNELS,
    PROP_CONTACT,
    PROP_DESCRIPTION,
    PROP_ICON,
    PROP_NAME,
    PROP_PUBLISHER,
    PROP_SCREENSHOTS,
    PROP_SUMMARY,
    PROP_TITLE,
    PROP_LAST
};

G_DEFINE_TYPE_WITH_PRIVATE (StoreApp, store_app, G_TYPE_OBJECT)

static void
store_app_dispose (GObject *object)
{
    StoreApp *self = STORE_APP (object);
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_clear_pointer (&priv->appstream_id, g_free);
    g_clear_pointer (&priv->channels, g_ptr_array_unref);
    g_clear_pointer (&priv->contact, g_free);
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
store_app_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    StoreApp *self = STORE_APP (object);
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    switch (prop_id)
    {
    case PROP_CHANNELS:
        g_value_set_boxed (value, priv->channels);
        break;
    case PROP_CONTACT:
        g_value_set_string (value, priv->contact);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, priv->description);
        break;
    case PROP_ICON:
        g_value_set_object (value, priv->icon);
        break;
    case PROP_NAME:
        g_value_set_string (value, priv->name);
        break;
    case PROP_PUBLISHER:
        g_value_set_string (value, priv->publisher);
        break;
    case PROP_SCREENSHOTS:
        g_value_set_boxed (value, priv->screenshots);
        break;
    case PROP_SUMMARY:
        g_value_set_string (value, priv->summary);
        break;
    case PROP_TITLE:
        g_value_set_string (value, priv->title);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_app_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreApp *self = STORE_APP (object);

    switch (prop_id)
    {
    case PROP_CHANNELS:
        store_app_set_channels (self, g_value_get_boxed (value));
        break;
    case PROP_CONTACT:
        store_app_set_contact (self, g_value_get_string (value));
        break;
    case PROP_DESCRIPTION:
        store_app_set_description (self, g_value_get_string (value));
        break;
    case PROP_ICON:
        store_app_set_icon (self, g_value_get_object (value));
        break;
    case PROP_NAME:
        store_app_set_name (self, g_value_get_string (value));
        break;
    case PROP_PUBLISHER:
        store_app_set_publisher (self, g_value_get_string (value));
        break;
    case PROP_SCREENSHOTS:
        store_app_set_screenshots (self, g_value_get_boxed (value));
        break;
    case PROP_SUMMARY:
        store_app_set_summary (self, g_value_get_string (value));
        break;
    case PROP_TITLE:
        store_app_set_title (self, g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
install_string_property (StoreAppClass *klass, guint property_id, const gchar *name)
{
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     property_id,
                                     g_param_spec_string (name, NULL, NULL, NULL, G_PARAM_READWRITE));
}

static void
install_array_property (StoreAppClass *klass, guint property_id, const gchar *name)
{
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     property_id,
                                     g_param_spec_boxed (name, NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_READWRITE));
}

static void
install_object_property (StoreAppClass *klass, guint property_id, const gchar *name, GType type)
{
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     property_id,
                                     g_param_spec_object (name, NULL, NULL, type, G_PARAM_READWRITE));
}

static void
store_app_class_init (StoreAppClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_dispose;
    G_OBJECT_CLASS (klass)->get_property = store_app_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_app_set_property;

    install_array_property (klass, PROP_CHANNELS, "channels");
    install_string_property (klass, PROP_CONTACT, "contact");
    install_string_property (klass, PROP_DESCRIPTION, "description");
    install_object_property (klass, PROP_ICON, "icon", store_media_get_type ());
    install_string_property (klass, PROP_NAME, "name");
    install_string_property (klass, PROP_PUBLISHER, "publisher");
    install_array_property (klass, PROP_SCREENSHOTS, "screenshots");
    install_string_property (klass, PROP_SUMMARY, "summary");
    install_string_property (klass, PROP_TITLE, "title");
}

static void
store_app_init (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    priv->channels = g_ptr_array_new ();
    priv->screenshots = g_ptr_array_new ();
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

    g_object_notify (G_OBJECT (self), "channels");
}

GPtrArray *
store_app_get_channels (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->channels;
}

void
store_app_set_contact (StoreApp *self, const gchar *contact)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->contact, g_free);
    priv->contact = g_strdup (contact);

    g_object_notify (G_OBJECT (self), "contact");
}

const gchar *
store_app_get_contact (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->contact;
}

void
store_app_set_description (StoreApp *self, const gchar *description)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->description, g_free);
    priv->description = g_strdup (description);

    g_object_notify (G_OBJECT (self), "description");
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

    g_object_notify (G_OBJECT (self), "icon");
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

    g_object_notify (G_OBJECT (self), "name");
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

    g_object_notify (G_OBJECT (self), "publisher");
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

    g_object_notify (G_OBJECT (self), "screenshots");
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

    g_object_notify (G_OBJECT (self), "summary");
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

    g_object_notify (G_OBJECT (self), "title");
}

const gchar *
store_app_get_title (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->title;
}
