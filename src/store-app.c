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
    g_clear_pointer (&priv->description, g_free);
    g_clear_object (&priv->icon);
    g_clear_pointer (&priv->name, g_free);
    g_clear_pointer (&priv->publisher, g_free);
    g_clear_pointer (&priv->screenshots, g_ptr_array_unref);
    g_clear_pointer (&priv->summary, g_free);
    g_clear_pointer (&priv->title, g_free);
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
    priv->description = g_strdup ("");
    priv->publisher = g_strdup ("");
    priv->screenshots = g_ptr_array_new ();
    priv->summary = g_strdup ("");
    priv->title = g_strdup ("");
}

StoreApp *
store_app_new (void)
{
    return g_object_new (store_app_get_type (), NULL);
}


StoreApp *
store_app_new_from_json (JsonNode *node)
{
    StoreApp *self = store_app_new ();

    JsonObject *object = json_node_get_object (node);
    store_app_set_appstream_id (self, json_object_get_string_member (object, "appstream-id"));
    store_app_set_description (self, json_object_get_string_member (object, "description"));
    g_autoptr(StoreMedia) icon = store_media_new_from_json (json_object_get_member (object, "icon"));
    store_app_set_icon (self, icon);
    store_app_set_name (self, json_object_get_string_member (object, "name"));
    store_app_set_publisher (self, json_object_get_string_member (object, "publisher"));
    store_app_set_publisher_validated (self, json_object_get_boolean_member (object, "publisher-validated"));
    GPtrArray *screenshots = g_ptr_array_new_with_free_func (g_object_unref);
    //FIXMEstore_app_set_screenshots (self, json_object_get_string_member (object, "screenshots"));
    store_app_set_screenshots (self, screenshots);
    store_app_set_summary (self, json_object_get_string_member (object, "summary"));
    store_app_set_title (self, json_object_get_string_member (object, "title"));

    return self;
}

JsonNode *
store_app_to_json (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "appstream-id");
    json_builder_add_string_value (builder, priv->appstream_id);
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, priv->description);
    if (priv->icon != NULL) {
        json_builder_set_member_name (builder, "icon");
        json_builder_add_value (builder, store_media_to_json (priv->icon));
    }
    json_builder_set_member_name (builder, "name");
    json_builder_add_string_value (builder, priv->name);
    json_builder_set_member_name (builder, "publisher");
    json_builder_add_string_value (builder, priv->publisher);
    json_builder_set_member_name (builder, "publisher-validated");
    json_builder_add_boolean_value (builder, priv->publisher_validated);
    json_builder_set_member_name (builder, "screenshots");
    json_builder_begin_array (builder);
    for (guint i = 0; i < priv->screenshots->len; i++) {
        StoreMedia *screenshot = g_ptr_array_index (priv->screenshots, i);
        json_builder_add_value (builder, store_media_to_json (screenshot));
    }
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "summary");
    json_builder_add_string_value (builder, priv->summary);
    json_builder_set_member_name (builder, "title");
    json_builder_add_string_value (builder, priv->title);
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

void
store_app_refresh_async (StoreApp *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    STORE_APP_GET_CLASS (self)->refresh_async (self, cancellable, callback, callback_data);
}

gboolean
store_app_refresh_finish (StoreApp *self, GAsyncResult *result, GError **error)
{
    return STORE_APP_GET_CLASS (self)->refresh_finish (self, result, error);
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
