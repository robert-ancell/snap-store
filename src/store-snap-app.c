/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-snap-app.h"

struct _StoreSnapApp
{
    StoreApp parent_instance;
};

G_DEFINE_TYPE (StoreSnapApp, store_snap_app, store_app_get_type ())

static void
install_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    GTask *task = user_data;

    g_autoptr(GError) error = NULL;
    if (!snapd_client_install2_finish (SNAPD_CLIENT (object), result, &error)) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to install snap: %s", error->message);
        return;
    }

    g_task_return_boolean (task, TRUE);
}

static int
strv_index (GStrv values, const gchar *value)
{
    for (int i = 0; values[i] != NULL; i++)
       if (g_strcmp0 (values[i], value) == 0)
           return i;
    return -1;
}

static int
compare_indexes (GStrv values, const gchar *a, const gchar *b)
{
    int index_a = strv_index (values, a);
    int index_b = strv_index (values, b);
    if (index_a == index_b)
        return g_strcmp0 (a, b);
    else if (index_a < 0)
        return 1;
    else if (index_b < 0)
        return -1;
    else
        return index_a - index_b;
}

static int
compare_channel (gconstpointer a, gconstpointer b, gpointer user_data)
{
    SnapdChannel *channel_a = *(SnapdChannel **)a;
    SnapdChannel *channel_b = *(SnapdChannel **)b;
    SnapdSnap *snap = user_data;

    const gchar *track_a = snapd_channel_get_track (channel_a);
    const gchar *track_b = snapd_channel_get_track (channel_b);
    if (g_strcmp0 (track_a, track_b) != 0) {
        GStrv tracks = snapd_snap_get_tracks (snap);
        return compare_indexes (tracks, track_a, track_b);
    }

    const gchar *risk_a = snapd_channel_get_risk (channel_a);
    const gchar *risk_b = snapd_channel_get_risk (channel_b);
    if (g_strcmp0 (risk_a, risk_b) != 0) {
        gchar *risks[] = { "stable", "candidate", "beta", "edge", NULL };
        return compare_indexes (risks, risk_a, risk_b);
    }

    return g_strcmp0 (snapd_channel_get_branch (channel_a), snapd_channel_get_branch (channel_b));
}

static void
find_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(GTask) task = user_data;

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

    StoreSnapApp *self = g_task_get_source_object (task);
    SnapdSnap *snap = g_ptr_array_index (snaps, 0);

    store_snap_app_update_from_search (self, snap);

    g_task_return_boolean (task, TRUE);
}

static void
remove_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    GTask *task = user_data;

    g_autoptr(GError) error = NULL;
    if (!snapd_client_remove_finish (SNAPD_CLIENT (object), result, &error)) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to remove snap: %s", error->message);
        return;
    }

    g_task_return_boolean (task, TRUE);
}

static void
store_snap_app_install_async (StoreApp *self, StoreChannel *channel G_GNUC_UNUSED, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_autoptr(SnapdClient) client = snapd_client_new ();
    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, store_app_get_name (self), NULL, NULL, NULL, NULL, cancellable, install_cb, task); // FIXME: channel
}

static gboolean
store_snap_app_install_finish (StoreApp *self G_GNUC_UNUSED, GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

static gboolean
store_snap_app_launch (StoreApp *self, GError **error)
{
    g_autofree gchar *command_line = g_strdup_printf ("snap run %s", store_app_get_name (self));
    // FIXME: Use desktop file if available
    // FIXME: Won't handle command line apps
    return g_spawn_command_line_async (command_line, error);
}

static void
store_snap_app_refresh_async (StoreApp *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_autoptr(SnapdClient) client = snapd_client_new ();
    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    snapd_client_find_async (client, SNAPD_FIND_FLAGS_MATCH_NAME, store_app_get_name (self), cancellable, find_cb, task);
}

static gboolean
store_snap_app_refresh_finish (StoreApp *self G_GNUC_UNUSED, GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

static void
store_snap_app_remove_async (StoreApp *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_autoptr(SnapdClient) client = snapd_client_new ();
    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    snapd_client_remove_async (client, store_app_get_name (self), NULL, NULL, cancellable, remove_cb, task);
}

static gboolean
store_snap_app_remove_finish (StoreApp *self G_GNUC_UNUSED, GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

static void
store_snap_app_save_to_cache (StoreApp *self, StoreCache *cache)
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "appstream-id"); // FIXME: Move common fields into StoreApp
    json_builder_add_string_value (builder, store_app_get_appstream_id (self));
    if (store_app_get_banner (self) != NULL) {
        json_builder_set_member_name (builder, "banner");
        json_builder_add_value (builder, store_media_to_json (store_app_get_banner (self)));
    }
    GPtrArray *channels = store_app_get_channels (self);
    if (channels->len > 0) {
        json_builder_set_member_name (builder, "channels");
        json_builder_begin_array (builder);
        for (guint i = 0; i < channels->len; i++) {
            StoreChannel *channel = g_ptr_array_index (channels, i);
            json_builder_add_value (builder, store_channel_to_json (channel));
        }
        json_builder_end_array (builder);
    }
    if (store_app_get_contact (self) != NULL) {
        json_builder_set_member_name (builder, "contact");
        json_builder_add_string_value (builder, store_app_get_contact (self));
    }
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, store_app_get_description (self));
    if (store_app_get_icon (self) != NULL) {
        json_builder_set_member_name (builder, "icon");
        json_builder_add_value (builder, store_media_to_json (store_app_get_icon (self)));
    }
    if (store_app_get_license (self) != NULL) {
        json_builder_set_member_name (builder, "license");
        json_builder_add_string_value (builder, store_app_get_license (self));
    }
    json_builder_set_member_name (builder, "name");
    json_builder_add_string_value (builder, store_app_get_name (self));
    json_builder_set_member_name (builder, "publisher");
    json_builder_add_string_value (builder, store_app_get_publisher (self));
    json_builder_set_member_name (builder, "publisher-validated");
    json_builder_add_boolean_value (builder, store_app_get_publisher_validated (self));
    json_builder_set_member_name (builder, "screenshots");
    json_builder_begin_array (builder);
    GPtrArray *screenshots = store_app_get_screenshots (self);
    for (guint i = 0; i < screenshots->len; i++) {
        StoreMedia *screenshot = g_ptr_array_index (screenshots, i);
        json_builder_add_value (builder, store_media_to_json (screenshot));
    }
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "summary");
    json_builder_add_string_value (builder, store_app_get_summary (self));
    json_builder_set_member_name (builder, "title");
    json_builder_add_string_value (builder, store_app_get_title (self));
    json_builder_set_member_name (builder, "version");
    json_builder_add_string_value (builder, store_app_get_version (self));
    json_builder_end_object (builder);

    g_autoptr(JsonNode) node = json_builder_get_root (builder);
    store_cache_insert_json (cache, "snaps", store_app_get_name (self), FALSE, node, NULL, NULL);
}

static void
store_snap_app_update_from_cache (StoreApp *self, StoreCache *cache)
{
    const gchar *name = store_app_get_name (STORE_APP (self));
    g_autoptr(JsonNode) node = store_cache_lookup_json (cache, "snaps", name, FALSE, NULL, NULL);
    if (node == NULL)
        return;

    JsonObject *object = json_node_get_object (node);
    store_app_set_appstream_id (STORE_APP (self), json_object_get_string_member (object, "appstream-id")); // FIXME: Move common fields into StoreApp
    if (json_object_has_member (object, "banner")) {
        g_autoptr(StoreMedia) banner = store_media_new_from_json (json_object_get_member (object, "banner"));
        store_app_set_banner (STORE_APP (self), banner);
    }
    if (json_object_has_member (object, "channels")) {
        g_autoptr(GPtrArray) channels = g_ptr_array_new_with_free_func (g_object_unref);
        JsonArray *channels_array = json_object_get_array_member (object, "channels");
        for (guint i = 0; i < json_array_get_length (channels_array); i++) {
            JsonNode *node = json_array_get_element (channels_array, i);
            g_autoptr(StoreChannel) channel = store_channel_new_from_json (node);
            g_ptr_array_add (channels, g_steal_pointer (&channel));
        }
        store_app_set_channels (STORE_APP (self), channels);
    }
    if (json_object_has_member (object, "contact"))
        store_app_set_contact (STORE_APP (self), json_object_get_string_member (object, "contact"));
    store_app_set_description (STORE_APP (self), json_object_get_string_member (object, "description"));
    if (json_object_has_member (object, "icon")) {
        g_autoptr(StoreMedia) icon = store_media_new_from_json (json_object_get_member (object, "icon"));
        store_app_set_icon (STORE_APP (self), icon);
    }
    if (json_object_has_member (object, "license"))
        store_app_set_license (STORE_APP (self), json_object_get_string_member (object, "license"));
    store_app_set_name (STORE_APP (self), json_object_get_string_member (object, "name"));
    store_app_set_publisher (STORE_APP (self), json_object_get_string_member (object, "publisher"));
    store_app_set_publisher_validated (STORE_APP (self), json_object_get_boolean_member (object, "publisher-validated"));
    g_autoptr(GPtrArray) screenshots = g_ptr_array_new_with_free_func (g_object_unref);
    JsonArray *screenshots_array = json_object_get_array_member (object, "screenshots");
    for (guint i = 0; i < json_array_get_length (screenshots_array); i++) {
        JsonNode *node = json_array_get_element (screenshots_array, i);
        g_autoptr(StoreMedia) screenshot = store_media_new_from_json (node);
        g_ptr_array_add (screenshots, g_steal_pointer (&screenshot));
    }
    store_app_set_screenshots (STORE_APP (self), screenshots);
    store_app_set_summary (STORE_APP (self), json_object_get_string_member (object, "summary"));
    store_app_set_title (STORE_APP (self), json_object_get_string_member (object, "title"));
    if (json_object_has_member (object, "version"))
        store_app_set_version (STORE_APP (self), json_object_get_string_member (object, "version"));
}

static void
store_snap_app_class_init (StoreSnapAppClass *klass)
{
    STORE_APP_CLASS (klass)->install_async = store_snap_app_install_async;
    STORE_APP_CLASS (klass)->install_finish = store_snap_app_install_finish;
    STORE_APP_CLASS (klass)->launch = store_snap_app_launch;
    STORE_APP_CLASS (klass)->refresh_async = store_snap_app_refresh_async;
    STORE_APP_CLASS (klass)->refresh_finish = store_snap_app_refresh_finish;
    STORE_APP_CLASS (klass)->remove_async = store_snap_app_remove_async;
    STORE_APP_CLASS (klass)->remove_finish = store_snap_app_remove_finish;
    STORE_APP_CLASS (klass)->save_to_cache = store_snap_app_save_to_cache;
    STORE_APP_CLASS (klass)->update_from_cache = store_snap_app_update_from_cache;
}

static void
store_snap_app_init (StoreSnapApp *self G_GNUC_UNUSED)
{
}

StoreSnapApp *
store_snap_app_new (void)
{
    return g_object_new (store_snap_app_get_type (), NULL);
}

static gboolean
is_screenshot (SnapdMedia *media)
{
    if (g_strcmp0 (snapd_media_get_media_type (media), "screenshot") != 0)
        return FALSE;

    /* Hide special legacy promotion screenshots */
    const gchar *uri = snapd_media_get_url (media);
    g_autofree gchar *basename = g_path_get_basename (uri);
    if (g_regex_match_simple ("^banner(?:_[a-zA-Z0-9]{7})?\\.(?:png|jpg)$", basename, 0, 0))
        return FALSE;
    if (g_regex_match_simple ("^banner-icon(?:_[a-zA-Z0-9]{7})?\\.(?:png|jpg)$", basename, 0, 0))
        return FALSE;

    return TRUE;
}

void
store_snap_app_update_from_search (StoreSnapApp *self, SnapdSnap *snap)
{
    g_return_if_fail (STORE_IS_SNAP_APP (self));

    store_app_set_name (STORE_APP (self), snapd_snap_get_name (snap));
    if (snapd_snap_get_title (snap) != NULL)
        store_app_set_title (STORE_APP (self), snapd_snap_get_title (snap));
    else
        store_app_set_title (STORE_APP (self), snapd_snap_get_name (snap));
    store_app_set_version (STORE_APP (self), snapd_snap_get_version (snap));
    store_app_set_license (STORE_APP (self), snapd_snap_get_license (snap));
    if (snapd_snap_get_publisher_display_name (snap) != NULL)
        store_app_set_publisher (STORE_APP (self), snapd_snap_get_publisher_display_name (snap));
    else
        store_app_set_publisher (STORE_APP (self), snapd_snap_get_publisher_username (snap));
    store_app_set_publisher_validated (STORE_APP (self), snapd_snap_get_publisher_validation (snap) == SNAPD_PUBLISHER_VALIDATION_VERIFIED);
    store_app_set_summary (STORE_APP (self), snapd_snap_get_summary (snap));
    store_app_set_description (STORE_APP (self), snapd_snap_get_description (snap));
    store_app_set_contact (STORE_APP (self), snapd_snap_get_contact (snap));

    if (snapd_snap_get_install_date (snap) != NULL)
        store_app_set_updated_date (STORE_APP (self), snapd_snap_get_install_date (snap));
    if (snapd_snap_get_installed_size (snap) != 0)
        store_app_set_installed_size (STORE_APP (self), snapd_snap_get_installed_size (snap));

    /* Channels are only returned on searches for a particular snap */
    GPtrArray *channels = snapd_snap_get_channels (snap);
    if (channels->len > 0) {
        g_autoptr(GPtrArray) store_channels = g_ptr_array_new_with_free_func (g_object_unref);
        g_autoptr(GPtrArray) sorted_channels = g_ptr_array_new ();
        for (guint i = 0; i < channels->len; i++)
            g_ptr_array_add (sorted_channels, g_ptr_array_index (channels, i));
        g_ptr_array_sort_with_data (sorted_channels, compare_channel, snap);
        for (guint i = 0; i < sorted_channels->len; i++) {
            SnapdChannel *c = g_ptr_array_index (sorted_channels, i);
            g_autoptr(StoreChannel) channel = store_channel_new ();
            g_autofree gchar *name = NULL;
            if (snapd_channel_get_branch (c) != NULL)
                name = g_strdup_printf ("%s/%s/%s", snapd_channel_get_track (c), snapd_channel_get_risk (c), snapd_channel_get_branch (c));
            else
                name = g_strdup_printf ("%s/%s", snapd_channel_get_track (c), snapd_channel_get_risk (c));
            store_channel_set_name (channel, name);
            store_channel_set_size (channel, snapd_channel_get_size (c));
            store_channel_set_version (channel, snapd_channel_get_version (c));
            store_channel_set_release_date (channel, snapd_channel_get_released_at (c));
            g_ptr_array_add (store_channels, g_steal_pointer (&channel));
        }
        store_app_set_channels (STORE_APP (self), store_channels);
    }

    GPtrArray *media = snapd_snap_get_media (snap);
    g_autoptr(GPtrArray) screenshots = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < media->len; i++) {
        SnapdMedia *m = g_ptr_array_index (media, i);
        if (g_strcmp0 (snapd_media_get_media_type (m), "icon") == 0 && store_app_get_icon (STORE_APP (self)) == NULL) {
            g_autoptr(StoreMedia) icon = store_media_new ();
            store_media_set_uri (icon, snapd_media_get_url (m));
            store_media_set_width (icon, snapd_media_get_width (m));
            store_media_set_height (icon, snapd_media_get_height (m));
            store_app_set_icon (STORE_APP (self), icon);
        }
        else if (g_strcmp0 (snapd_media_get_media_type (m), "banner") == 0 && store_app_get_banner (STORE_APP (self)) == NULL) {
            g_autoptr(StoreMedia) banner = store_media_new ();
            store_media_set_uri (banner, snapd_media_get_url (m));
            store_media_set_width (banner, snapd_media_get_width (m));
            store_media_set_height (banner, snapd_media_get_height (m));
            store_app_set_banner (STORE_APP (self), banner);
        }
        else if (is_screenshot (m)) {
            g_autoptr(StoreMedia) screenshot = store_media_new ();
            store_media_set_uri (screenshot, snapd_media_get_url (m));
            store_media_set_width (screenshot, snapd_media_get_width (m));
            store_media_set_height (screenshot, snapd_media_get_height (m));
            g_ptr_array_add (screenshots, g_steal_pointer (&screenshot));
        }
    }
    store_app_set_screenshots (STORE_APP (self), screenshots);

    g_autofree gchar *appstream_id = g_strdup_printf ("io.snapcraft.%s-%s", snapd_snap_get_name (snap), snapd_snap_get_id (snap));
    store_app_set_appstream_id (STORE_APP (self), appstream_id);
}
