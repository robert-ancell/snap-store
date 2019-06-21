/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

#include "store-odrs-client.h"

struct _StoreOdrsClient
{
    GObject parent_instance;

    GCancellable *cancellable;
    gchar *distro;
    gchar *locale;
    gchar *user_hash;
};

G_DEFINE_TYPE (StoreOdrsClient, store_odrs_client, G_TYPE_OBJECT)

static void
store_odrs_client_dispose (GObject *object)
{
    StoreOdrsClient *self = STORE_ODRS_CLIENT (object);
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_clear_pointer (&self->distro, g_free);
    g_clear_pointer (&self->locale, g_free);
    g_clear_pointer (&self->user_hash, g_free);
}

static void
store_odrs_client_class_init (StoreOdrsClientClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_odrs_client_dispose;
}

static gchar *
get_user_hash (void)
{
    g_autofree gchar *machine_id = NULL;
    g_autoptr(GError) error = NULL;
    if (!g_file_get_contents ("/etc/machine-id", &machine_id, NULL, &error)) {
        g_warning ("Failed to determine machine ID: %s", error->message);
        return NULL;
    }

    g_autofree gchar *salted = g_strdup_printf ("gnome-software[%s:%s]", g_get_user_name (), machine_id);
    return g_compute_checksum_for_string (G_CHECKSUM_SHA1, salted, -1);
}

static void
store_odrs_client_init (StoreOdrsClient *self)
{
    self->cancellable = g_cancellable_new ();
    self->distro = g_strdup ("Ubuntu"); // FIXME
    self->locale = g_strdup ("en"); // FIXME
    self->user_hash = get_user_hash ();
}

StoreOdrsClient *
store_odrs_client_new (void)
{
    return g_object_new (store_odrs_client_get_type (), NULL);
}

static void
send_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreOdrsClient *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GInputStream) stream = soup_session_send_finish (SOUP_SESSION (object), result, &error);
    if (stream == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to download image: %s", error->message);
        return;
    }

    g_autoptr(JsonParser) parser = json_parser_new ();
    if (!json_parser_load_from_stream (parser, stream, self->cancellable, &error)) {
        g_warning ("Failed to parse ODRS response: %s", error->message);
        return;
    }

    JsonNode *root = json_parser_get_root (parser);
    if (json_node_get_node_type (root) != JSON_NODE_ARRAY) {
        g_warning ("Failed to get reviews, server returned non JSON array");
        return;
    }

    JsonArray *array = json_node_get_array (root);
    for (guint i = 0; i < json_array_get_length (array); i++) {
        JsonNode *element = json_array_get_element (array, i);

        if (json_node_get_node_type (element) != JSON_NODE_OBJECT) {
            g_warning ("ODRS server returned non-object review");
            return;
        }

        JsonObject *review = json_node_get_object (element);
        g_printerr ("rating: %" G_GINT64_FORMAT "\n", json_object_get_int_member (review, "rating"));
        g_printerr ("summary: %s\n", json_object_get_string_member (review, "summary"));
        g_printerr ("description: %s\n", json_object_get_string_member (review, "description"));
    }
}

void
store_odrs_client_get_reviews_async (StoreOdrsClient *self, const gchar *app_id, const gchar *version, gint64 limit, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (STORE_IS_ODRS_CLIENT (self));
    g_return_if_fail (app_id != NULL);

    if (version == NULL)
        version = "";

    g_autoptr(SoupSession) session = soup_session_new (); // FIXME: common session?
    g_autoptr(SoupMessage) message = soup_message_new ("POST", "https://odrs.gnome.org/1.0/reviews/api/fetch");

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "user_hash");
    json_builder_add_string_value (builder, self->user_hash);
    json_builder_set_member_name (builder, "app_id");
    json_builder_add_string_value (builder, app_id);
    json_builder_set_member_name (builder, "locale");
    json_builder_add_string_value (builder, self->locale);
    json_builder_set_member_name (builder, "distro");
    json_builder_add_string_value (builder, self->distro);
    json_builder_set_member_name (builder, "version");
    json_builder_add_string_value (builder, version);
    json_builder_set_member_name (builder, "limit");
    json_builder_add_int_value (builder, limit);
    json_builder_end_object (builder);
    g_autoptr(JsonGenerator) generator = json_generator_new ();
    g_autoptr(JsonNode) root = json_builder_get_root (builder);
    json_generator_set_root (generator, root);
    gsize json_text_length;
    g_autofree gchar *json_text = json_generator_to_data (generator, &json_text_length);
    soup_message_set_request (message, "application/json; charset=utf-8", SOUP_MEMORY_COPY, json_text, json_text_length);

    soup_session_send_async (session, message, self->cancellable, send_cb, self);
}

GPtrArray *
store_odrs_client_get_reviews_finish (StoreOdrsClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_ODRS_CLIENT (self), NULL);
    return NULL;
}
