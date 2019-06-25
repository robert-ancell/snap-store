/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <snapd-glib/snapd-glib.h> // FIXME: subclass

#include "store-app.h"

struct _StoreApp
{
    GObject parent_instance;

    gchar *appstream_id;
    gchar *description;
    StoreMedia *icon;
    gchar *name;
    gchar *publisher;
    gboolean publisher_validated;
    GPtrArray *screenshots;
    gchar *summary;
    gchar *title;
};

G_DEFINE_TYPE (StoreApp, store_app, G_TYPE_OBJECT)

static void
store_app_dispose (GObject *object)
{
    StoreApp *self = STORE_APP (object);
    g_clear_pointer (&self->appstream_id, g_free);
    g_clear_pointer (&self->description, g_free);
    g_clear_object (&self->icon);
    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->publisher, g_free);
    g_clear_pointer (&self->screenshots, g_ptr_array_unref);
    g_clear_pointer (&self->summary, g_free);
    g_clear_pointer (&self->title, g_free);
}

static void
store_app_class_init (StoreAppClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_dispose;
}

static void
store_app_init (StoreApp *self)
{
    self->appstream_id = g_strdup ("");
    self->description = g_strdup ("");
    self->publisher = g_strdup ("");
    self->screenshots = g_ptr_array_new ();
    self->summary = g_strdup ("");
    self->title = g_strdup ("");
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
    g_autoptr(JsonBuilder) builder = json_builder_new ();

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "appstream-id");
    json_builder_add_string_value (builder, self->appstream_id);
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, self->description);
    if (self->icon != NULL) {
        json_builder_set_member_name (builder, "icon");
        json_builder_add_value (builder, store_media_to_json (self->icon));
    }
    json_builder_set_member_name (builder, "name");
    json_builder_add_string_value (builder, self->name);
    json_builder_set_member_name (builder, "publisher");
    json_builder_add_string_value (builder, self->publisher);
    json_builder_set_member_name (builder, "publisher-validated");
    json_builder_add_boolean_value (builder, self->publisher_validated);
    json_builder_set_member_name (builder, "screenshots");
    json_builder_begin_array (builder);
    for (guint i = 0; i < self->screenshots->len; i++) {
        StoreMedia *screenshot = g_ptr_array_index (self->screenshots, i);
        json_builder_add_value (builder, store_media_to_json (screenshot));
    }
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "summary");
    json_builder_add_string_value (builder, self->summary);
    json_builder_set_member_name (builder, "title");
    json_builder_add_string_value (builder, self->title);
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static void
find_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    GTask *task = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_find_finish (SNAPD_CLIENT (object), result, NULL, &error);
    if (snaps == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to get snap information: %s", error->message);
        return;
    }

    if (snaps->len != 1) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Snap find returned %d results, expected 1", snaps->len);
        return;
    }

    SnapdSnap *snap = g_ptr_array_index (snaps, 0);

    // FIXME: Merge in updated data
    // FIXME: Save in cache

    g_task_return_boolean (task, TRUE);
}


void
store_app_refresh_async (StoreApp *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_APP (self));

    g_autoptr(SnapdClient) client = snapd_client_new ();
    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    snapd_client_find_async (client, SNAPD_FIND_FLAGS_MATCH_NAME, self->name, cancellable, find_cb, task);
}

gboolean
store_app_refresh_finish (StoreApp *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_APP (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

void
store_app_set_appstream_id (StoreApp *self, const gchar *appstream_id)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->appstream_id, g_free);
    self->appstream_id = g_strdup (appstream_id);
}

const gchar *
store_app_get_appstream_id (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->appstream_id;
}

void
store_app_set_description (StoreApp *self, const gchar *description)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->description, g_free);
    self->description = g_strdup (description);
}

const gchar *
store_app_get_description (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->description;
}

void
store_app_set_icon (StoreApp *self, StoreMedia *icon)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_object (&self->icon);
    if (icon != NULL)
        self->icon = g_object_ref (icon);
}

StoreMedia *
store_app_get_icon (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->icon;
}

void
store_app_set_name (StoreApp *self, const gchar *name)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->name, g_free);
    self->name = g_strdup (name);
}

const gchar *
store_app_get_name (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->name;
}

void
store_app_set_publisher (StoreApp *self, const gchar *publisher)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->publisher, g_free);
    self->publisher = g_strdup (publisher);
}

const gchar *
store_app_get_publisher (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->publisher;
}

void
store_app_set_publisher_validated (StoreApp *self, gboolean validated)
{
    g_return_if_fail (STORE_IS_APP (self));
    self->publisher_validated = validated;
}

gboolean
store_app_get_publisher_validated (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), FALSE);
    return self->publisher_validated;
}

void
store_app_set_screenshots (StoreApp *self, GPtrArray *screenshots)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->screenshots, g_ptr_array_unref);
    if (screenshots != NULL)
        self->screenshots = g_ptr_array_ref (screenshots);
}

GPtrArray *
store_app_get_screenshots (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->screenshots;
}

void
store_app_set_summary (StoreApp *self, const gchar *summary)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->summary, g_free);
    self->summary = g_strdup (summary);
}

const gchar *
store_app_get_summary (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->summary;
}

void
store_app_set_title (StoreApp *self, const gchar *title)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->title, g_free);
    self->title = g_strdup (title);
}

const gchar *
store_app_get_title (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->title;
}
