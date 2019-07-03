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

#include "store-odrs-review.h"

struct _StoreOdrsClient
{
    GObject parent_instance;

    GCancellable *cancellable;
    gchar *distro;
    gchar *locale;
    GHashTable *ratings;
    gchar *server_uri;
    SoupSession *soup_session;
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
    g_clear_pointer (&self->ratings, g_hash_table_unref);
    g_clear_pointer (&self->server_uri, g_free);
    g_clear_object (&self->soup_session);
    g_clear_pointer (&self->user_hash, g_free);

    G_OBJECT_CLASS (store_odrs_client_parent_class)->dispose (object);
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
    self->server_uri = g_strdup ("https://odrs.gnome.org/1.0/reviews/api");
    self->soup_session = soup_session_new (); // FIXME: Support common session
    self->user_hash = get_user_hash ();
}

StoreOdrsClient *
store_odrs_client_new (void)
{
    return g_object_new (store_odrs_client_get_type (), NULL);
}

void
store_odrs_client_set_distro (StoreOdrsClient *self, const gchar *distro)
{
    g_return_if_fail (STORE_IS_ODRS_CLIENT (self));
    g_free (self->distro);
    self->distro = g_strdup (distro);
}

void
store_odrs_client_set_locale (StoreOdrsClient *self, const gchar *locale)
{
    g_return_if_fail (STORE_IS_ODRS_CLIENT (self));
    g_free (self->locale);
    self->locale = g_strdup (locale);
}

static JsonNode *
send_finish (GTask *task, GObject *object, GAsyncResult *result, GError **error)
{
    g_autoptr(GInputStream) stream = soup_session_send_finish (SOUP_SESSION (object), result, error);
    if (stream == NULL)
        return NULL;

    StoreOdrsClient *self = g_task_get_source_object (task);

    g_autoptr(JsonParser) parser = json_parser_new ();
    if (!json_parser_load_from_stream (parser, stream, self->cancellable, error))
        return NULL;

    return json_node_ref (json_parser_get_root (parser));
}

static void
get_ratings_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(GTask) task = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonNode) root = send_finish (task, object, result, &error);
    if (root == NULL) {
        g_task_return_error (task, g_steal_pointer (&error));
        return;
    }

    StoreOdrsClient *self = g_task_get_source_object (task);

    if (json_node_get_node_type (root) != JSON_NODE_OBJECT) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to get ratings, server returned non JSON object");
        return;
    }

    g_clear_pointer (&self->ratings, g_hash_table_unref);
    self->ratings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    if (json_node_get_node_type (root) != JSON_NODE_OBJECT) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "ODRS server returned non-object ratings");
        return;
    }
    JsonObject *ratings_object = json_node_get_object (root);
    JsonObjectIter iter;
    json_object_iter_init (&iter, ratings_object);
    const gchar *app_id;
    JsonNode *node;
    while (json_object_iter_next (&iter, &app_id, &node)) {
        if (json_node_get_node_type (node) != JSON_NODE_OBJECT)
            continue;
        JsonObject *o = json_node_get_object (node);
        gint64 *values = g_new0 (gint64, 5);
        if (json_object_has_member (o, "star1"))
            values[0] = json_object_get_int_member (o, "star1");
        if (json_object_has_member (o, "star2"))
            values[1] = json_object_get_int_member (o, "star2");
        if (json_object_has_member (o, "star3"))
            values[2] = json_object_get_int_member (o, "star3");
        if (json_object_has_member (o, "star4"))
            values[3] = json_object_get_int_member (o, "star4");
        if (json_object_has_member (o, "star5"))
            values[4] = json_object_get_int_member (o, "star5");
        g_hash_table_insert (self->ratings, g_strdup (app_id), values);
    }

    g_task_return_boolean (task, TRUE);
}

static void
get_reviews_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(GTask) task = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonNode) root = send_finish (task, object, result, &error);
    if (root == NULL) {
        g_task_return_error (task, g_steal_pointer (&error));
        return;
    }

    if (json_node_get_node_type (root) != JSON_NODE_ARRAY) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to get reviews, server returned non JSON array");
        return;
    }

    JsonArray *array = json_node_get_array (root);
    g_autoptr(GPtrArray) reviews = g_ptr_array_new_with_free_func (g_object_unref);
    g_autofree gchar *user_skey = NULL;
    for (guint i = 0; i < json_array_get_length (array); i++) {
        JsonNode *element = json_array_get_element (array, i);

        if (json_node_get_node_type (element) != JSON_NODE_OBJECT) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "ODRS server returned non-object review");
            return;
        }
        JsonObject *object = json_node_get_object (element);

        /* Skip empty reviews (one always sent to provide secret key) */
        if (!json_object_has_member (object, "rating"))
            continue;

        if (user_skey == NULL)
            user_skey = g_strdup (json_object_get_string_member (object, "user_skey"));

        g_autoptr(StoreOdrsReview) review = store_odrs_review_new ();
        store_odrs_review_set_id (review, json_object_get_int_member (object, "review_id"));
        store_odrs_review_set_rating (review, json_object_get_int_member (object, "rating"));
        store_odrs_review_set_author (review, json_object_get_string_member (object, "user_display"));
        g_autoptr(GDateTime) date_created = g_date_time_new_from_unix_utc (json_object_get_int_member (object, "date_created"));
        store_odrs_review_set_date_created (review, date_created);
        store_odrs_review_set_summary (review, json_object_get_string_member (object, "summary"));
        store_odrs_review_set_description (review, json_object_get_string_member (object, "description"));
        g_ptr_array_add (reviews, g_steal_pointer (&review));
    }

    /* Attach a fake review on the end with the secret key */
    g_autoptr(StoreOdrsReview) review = store_odrs_review_new ();
    store_odrs_review_set_summary (review, user_skey);
    g_ptr_array_add (reviews, g_steal_pointer (&review));

    g_task_return_pointer (task, g_steal_pointer (&reviews), (GDestroyNotify) g_ptr_array_unref);
}

static void
submit_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(GTask) task = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonNode) root = send_finish (task, object, result, &error);
    if (root == NULL) {
        g_task_return_error (task, g_steal_pointer (&error));
        return;
    }

    g_task_return_boolean (task, TRUE);
}

static void
upvote_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(GTask) task = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonNode) root = send_finish (task, object, result, &error);
    if (root == NULL) {
        g_task_return_error (task, g_steal_pointer (&error));
        return;
    }

    g_task_return_boolean (task, TRUE);
}

static void
downvote_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(GTask) task = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonNode) root = send_finish (task, object, result, &error);
    if (root == NULL) {
        g_task_return_error (task, g_steal_pointer (&error));
        return;
    }

    g_task_return_boolean (task, TRUE);
}

static void
report_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(GTask) task = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonNode) root = send_finish (task, object, result, &error);
    if (root == NULL) {
        g_task_return_error (task, g_steal_pointer (&error));
        return;
    }

    g_task_return_boolean (task, TRUE);
}

gint64 *
store_odrs_client_get_ratings (StoreOdrsClient *self, const gchar *app_id)
{
    g_return_val_if_fail (STORE_IS_ODRS_CLIENT (self), NULL);
    g_return_val_if_fail (app_id != NULL, NULL);

    if (self->ratings == NULL)
        return NULL;

    return g_hash_table_lookup (self->ratings, app_id);
}

void
store_odrs_client_update_ratings_async (StoreOdrsClient *self,
                                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_ODRS_CLIENT (self));

    g_autofree gchar *uri= g_strdup_printf ("%s/ratings", self->server_uri);
    g_autoptr(SoupMessage) message = soup_message_new ("GET", uri);

    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    soup_session_send_async (self->soup_session, message, self->cancellable, get_ratings_cb, task);
}

gboolean
store_odrs_client_update_ratings_finish (StoreOdrsClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_ODRS_CLIENT (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

void
store_odrs_client_get_reviews_async (StoreOdrsClient *self, const gchar *app_id, GStrv compat_ids, const gchar *version, gint64 limit, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_ODRS_CLIENT (self));
    g_return_if_fail (app_id != NULL);

    if (version == NULL)
        version = "unknown";

    g_autofree gchar *uri= g_strdup_printf ("%s/fetch", self->server_uri);
    g_autoptr(SoupMessage) message = soup_message_new ("POST", uri);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "user_hash");
    json_builder_add_string_value (builder, self->user_hash);
    json_builder_set_member_name (builder, "app_id");
    json_builder_add_string_value (builder, app_id);
    if (compat_ids != NULL) {
        json_builder_set_member_name (builder, "compat_ids");
        json_builder_begin_array (builder);
        for (int i = 0; compat_ids[i] != NULL; i++)
            json_builder_add_string_value (builder, compat_ids[i]);
        json_builder_end_array (builder);
    }
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

    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    soup_session_send_async (self->soup_session, message, self->cancellable, get_reviews_cb, task);
}

GPtrArray *
store_odrs_client_get_reviews_finish (StoreOdrsClient *self, GAsyncResult *result, gchar **user_skey, GError **error)
{
    g_return_val_if_fail (STORE_IS_ODRS_CLIENT (self), NULL);
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), self), NULL);

    g_autoptr(GPtrArray) reviews = g_task_propagate_pointer (G_TASK (result), error);
    if (reviews == NULL)
        return NULL;

    /* Pull off fake review to get secret key */
    if (user_skey != NULL) {
        StoreOdrsReview *review = g_ptr_array_index (reviews, reviews->len - 1);
        *user_skey = g_strdup (store_odrs_review_get_summary (review));
    }
    g_ptr_array_remove_index (reviews, reviews->len - 1);

    return g_steal_pointer (&reviews);
}

void
store_odrs_client_submit_async (StoreOdrsClient *self, const gchar *user_skey, const gchar *app_id, const gchar *version, const gchar *user_display, const gchar *summary, const gchar *description, gint64 rating,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_ODRS_CLIENT (self));
    g_return_if_fail (app_id != NULL);

    g_autofree gchar *uri= g_strdup_printf ("%s/submit", self->server_uri);
    g_autoptr(SoupMessage) message = soup_message_new ("POST", uri);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "user_hash");
    json_builder_add_string_value (builder, self->user_hash);
    json_builder_set_member_name (builder, "user_skey");
    json_builder_add_string_value (builder, user_skey);
    json_builder_set_member_name (builder, "app_id");
    json_builder_add_string_value (builder, app_id);
    json_builder_set_member_name (builder, "locale");
    json_builder_add_string_value (builder, self->locale);
    json_builder_set_member_name (builder, "distro");
    json_builder_add_string_value (builder, self->distro);
    json_builder_set_member_name (builder, "version");
    json_builder_add_string_value (builder, version);
    json_builder_set_member_name (builder, "user_display");
    json_builder_add_string_value (builder, user_display);
    json_builder_set_member_name (builder, "summary");
    json_builder_add_string_value (builder, summary);
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, description);
    json_builder_set_member_name (builder, "rating");
    json_builder_add_int_value (builder, rating);
    json_builder_end_object (builder);
    g_autoptr(JsonGenerator) generator = json_generator_new ();
    g_autoptr(JsonNode) root = json_builder_get_root (builder);
    json_generator_set_root (generator, root);
    gsize json_text_length;
    g_autofree gchar *json_text = json_generator_to_data (generator, &json_text_length);
    soup_message_set_request (message, "application/json; charset=utf-8", SOUP_MEMORY_COPY, json_text, json_text_length);

    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    soup_session_send_async (self->soup_session, message, self->cancellable, submit_cb, task);
}

gboolean
store_odrs_client_submit_finish (StoreOdrsClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_ODRS_CLIENT (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

void
store_odrs_client_upvote_async (StoreOdrsClient *self, const gchar *user_skey, const gchar *app_id, gint64 review_id,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_ODRS_CLIENT (self));
    g_return_if_fail (app_id != NULL);

    g_autofree gchar *uri= g_strdup_printf ("%s/upvote", self->server_uri);
    g_autoptr(SoupMessage) message = soup_message_new ("POST", uri);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "user_hash");
    json_builder_add_string_value (builder, self->user_hash);
    json_builder_set_member_name (builder, "user_skey");
    json_builder_add_string_value (builder, user_skey);
    json_builder_set_member_name (builder, "app_id");
    json_builder_add_string_value (builder, app_id);
    json_builder_set_member_name (builder, "review_id");
    json_builder_add_int_value (builder, review_id);
    json_builder_end_object (builder);
    g_autoptr(JsonGenerator) generator = json_generator_new ();
    g_autoptr(JsonNode) root = json_builder_get_root (builder);
    json_generator_set_root (generator, root);
    gsize json_text_length;
    g_autofree gchar *json_text = json_generator_to_data (generator, &json_text_length);
    soup_message_set_request (message, "application/json; charset=utf-8", SOUP_MEMORY_COPY, json_text, json_text_length);

    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    soup_session_send_async (self->soup_session, message, self->cancellable, upvote_cb, task);
}

gboolean
store_odrs_client_upvote_finish (StoreOdrsClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_ODRS_CLIENT (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

void
store_odrs_client_downvote_async (StoreOdrsClient *self, const gchar *user_skey, const gchar *app_id, gint64 review_id,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_ODRS_CLIENT (self));
    g_return_if_fail (app_id != NULL);

    g_autofree gchar *uri= g_strdup_printf ("%s/downvote", self->server_uri);
    g_autoptr(SoupMessage) message = soup_message_new ("POST", uri);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "user_hash");
    json_builder_add_string_value (builder, self->user_hash);
    json_builder_set_member_name (builder, "user_skey");
    json_builder_add_string_value (builder, user_skey);
    json_builder_set_member_name (builder, "app_id");
    json_builder_add_string_value (builder, app_id);
    json_builder_set_member_name (builder, "review_id");
    json_builder_add_int_value (builder, review_id);
    json_builder_end_object (builder);
    g_autoptr(JsonGenerator) generator = json_generator_new ();
    g_autoptr(JsonNode) root = json_builder_get_root (builder);
    json_generator_set_root (generator, root);
    gsize json_text_length;
    g_autofree gchar *json_text = json_generator_to_data (generator, &json_text_length);
    soup_message_set_request (message, "application/json; charset=utf-8", SOUP_MEMORY_COPY, json_text, json_text_length);

    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    soup_session_send_async (self->soup_session, message, self->cancellable, downvote_cb, task);
}

gboolean
store_odrs_client_downvote_finish (StoreOdrsClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_ODRS_CLIENT (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

void
store_odrs_client_report_async (StoreOdrsClient *self, const gchar *user_skey, const gchar *app_id, gint64 review_id,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_ODRS_CLIENT (self));
    g_return_if_fail (app_id != NULL);

    g_autofree gchar *uri= g_strdup_printf ("%s/report", self->server_uri);
    g_autoptr(SoupMessage) message = soup_message_new ("POST", uri);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "user_hash");
    json_builder_add_string_value (builder, self->user_hash);
    json_builder_set_member_name (builder, "user_skey");
    json_builder_add_string_value (builder, user_skey);
    json_builder_set_member_name (builder, "app_id");
    json_builder_add_string_value (builder, app_id);
    json_builder_set_member_name (builder, "review_id");
    json_builder_add_int_value (builder, review_id);
    json_builder_end_object (builder);
    g_autoptr(JsonGenerator) generator = json_generator_new ();
    g_autoptr(JsonNode) root = json_builder_get_root (builder);
    json_generator_set_root (generator, root);
    gsize json_text_length;
    g_autofree gchar *json_text = json_generator_to_data (generator, &json_text_length);
    soup_message_set_request (message, "application/json; charset=utf-8", SOUP_MEMORY_COPY, json_text, json_text_length);

    GTask *task = g_task_new (self, cancellable, callback, callback_data); // FIXME: Need to combine cancellables?
    soup_session_send_async (self->soup_session, message, self->cancellable, report_cb, task);
}

gboolean
store_odrs_client_report_finish (StoreOdrsClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_ODRS_CLIENT (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}
