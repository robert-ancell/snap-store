/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <json-glib/json-glib.h>

#include "mock-odrs-server.h"

struct _MockOdrsServer
{
    SoupServer parent_instance;

    GPtrArray *apps;
};

G_DEFINE_TYPE (MockOdrsServer, mock_odrs_server, SOUP_TYPE_SERVER)

struct _MockReview
{
    gint64 id;
    gint64 rating;
    gchar *locale;
    gchar *distro;
    gchar *version;
    gchar *user_display;
    gint64 date_created;
    gchar *summary;
    gchar *description;
    gint64 upvote_count;
    gint64 downvote_count;
    gint64 report_count;
};

static MockReview *
mock_review_new (void)
{
    MockReview *review = g_new0 (MockReview, 1);
    return review;
}

static void
mock_review_free (MockReview *review)
{
    g_free (review->locale);
    g_free (review->distro);
    g_free (review->version);
    g_free (review->user_display);
    g_free (review->summary);
    g_free (review->description);
    g_free (review);
}

struct _MockApp
{
    gchar *id;
    GPtrArray *reviews;
};

static MockApp *
mock_app_new (const gchar *id)
{
    MockApp *app = g_new0 (MockApp, 1);
    app->id = g_strdup (id);
    app->reviews = g_ptr_array_new_with_free_func ((GDestroyNotify) mock_review_free);
    return app;
}

static void
mock_app_free (MockApp *app)
{
    g_free (app->id);
    g_ptr_array_unref (app->reviews);
    g_free (app);
}

static gboolean
validate_user (const gchar *user_hash G_GNUC_UNUSED, const gchar *user_skey G_GNUC_UNUSED)
{
    return TRUE; // FIXME
}

static gchar *
calculate_user_skey (const gchar *user_hash)
{
    return g_strdup (user_hash); // FIXME
}

static void
ratings_cb (SoupServer *server G_GNUC_UNUSED, SoupMessage *msg, const gchar *path G_GNUC_UNUSED, GHashTable *query G_GNUC_UNUSED, SoupClientContext *context G_GNUC_UNUSED, gpointer user_data)
{
    MockOdrsServer *self = user_data;

    if (msg->method != SOUP_METHOD_GET) {
        soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    for (guint i = 0; i < self->apps->len; i++) {
        MockApp *app = g_ptr_array_index (self->apps, i);

        gint64 count0, count1, count2 = 0, count3 = 0, count4 = 0, count5 = 0;
        for (guint j = 0; j < app->reviews->len; j++) {
            MockReview *review = g_ptr_array_index (app->reviews, j);
            if (review->rating == 0)
                count0++;
            if (review->rating == 1)
                count1++;
            if (review->rating == 2)
                count2++;
            if (review->rating == 3)
                count3++;
            if (review->rating == 4)
                count4++;
            if (review->rating == 5)
                count5++;
        }

        json_builder_set_member_name (builder, app->id);
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "star0");
        json_builder_add_int_value (builder, count0);
        json_builder_set_member_name (builder, "star1");
        json_builder_add_int_value (builder, count1);
        json_builder_set_member_name (builder, "star2");
        json_builder_add_int_value (builder, count2);
        json_builder_set_member_name (builder, "star3");
        json_builder_add_int_value (builder, count3);
        json_builder_set_member_name (builder, "star4");
        json_builder_add_int_value (builder, count4);
        json_builder_set_member_name (builder, "star5");
        json_builder_add_int_value (builder, count5);
        json_builder_set_member_name (builder, "total");
        json_builder_add_int_value (builder, count0 + count1 + count2 + count3 + count4 + count5);
        json_builder_end_object (builder);
    }
    json_builder_end_object (builder);

    g_autoptr(JsonGenerator) generator = json_generator_new ();
    g_autoptr(JsonNode) root = json_builder_get_root (builder);
    json_generator_set_root (generator, root);
    gsize json_text_length;
    g_autofree gchar *json_text = json_generator_to_data (generator, &json_text_length);

    soup_message_set_status (msg, SOUP_STATUS_OK);
    soup_message_set_response (msg, "application/json; charset=utf-8", SOUP_MEMORY_TAKE, g_steal_pointer (&json_text), json_text_length);
}

static JsonNode *
decode_request (SoupMessage *msg, GError **error)
{
    g_autoptr(SoupBuffer) buffer = soup_message_body_flatten (msg->request_body);

    g_autoptr(JsonParser) parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, buffer->data, buffer->length, error))
        return NULL;

    JsonNode *root = json_parser_get_root (parser);
    if (root == NULL)
        return NULL;

    return json_node_ref (root);
}

static void
respond (SoupMessage *msg, gboolean success, const gchar *message)
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "success");
    json_builder_add_boolean_value (builder, success);
    if (message != NULL) {
        json_builder_set_member_name (builder, "msg");
        json_builder_add_string_value (builder, message);
    }
    json_builder_end_object (builder);

    g_autoptr(JsonGenerator) generator = json_generator_new ();
    g_autoptr(JsonNode) root = json_builder_get_root (builder);
    json_generator_set_root (generator, root);
    gsize json_text_length;
    g_autofree gchar *json_text = json_generator_to_data (generator, &json_text_length);

    if (success)
        soup_message_set_status (msg, SOUP_STATUS_OK);
    else
        soup_message_set_status (msg, SOUP_STATUS_BAD_REQUEST);
    soup_message_set_response (msg, "application/json; charset=utf-8", SOUP_MEMORY_TAKE, g_steal_pointer (&json_text), json_text_length);
}

static void
fetch_cb (SoupServer *server G_GNUC_UNUSED, SoupMessage *msg, const gchar *path G_GNUC_UNUSED, GHashTable *query G_GNUC_UNUSED, SoupClientContext *context G_GNUC_UNUSED, gpointer user_data)
{
    MockOdrsServer *self = user_data;

    if (msg->method != SOUP_METHOD_POST) {
        soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonNode) root = decode_request (msg, &error);
    if (root == NULL) {
        respond (msg, FALSE, NULL);
        return;
    }

    JsonObject *object = json_node_get_object (root);
    const gchar *user_hash = json_object_get_string_member (object, "user_hash");
    const gchar *app_id = json_object_get_string_member (object, "app_id");
    //json_object_get_array_member (object, "compat_ids");
    gint64 limit = json_object_get_int_member (object, "limit");

    MockApp *app = mock_odrs_server_find_app (self, app_id);
    if (app == NULL) {
        respond (msg, FALSE, NULL);
        return;
    }

    g_autofree gchar *user_skey = calculate_user_skey (user_hash);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (guint i = 0; i < app->reviews->len && i < limit; i++) {
        MockReview *review = g_ptr_array_index (app->reviews, i);
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "user_skey");
        json_builder_add_string_value (builder, user_skey);
        json_builder_set_member_name (builder, "review_id");
        json_builder_add_int_value (builder, review->id);
        json_builder_set_member_name (builder, "rating");
        json_builder_add_int_value (builder, review->rating);
        json_builder_set_member_name (builder, "user_display");
        json_builder_add_string_value (builder, review->user_display);
        json_builder_set_member_name (builder, "date_created");
        json_builder_add_int_value (builder, review->date_created);
        json_builder_set_member_name (builder, "summary");
        json_builder_add_string_value (builder, review->summary);
        json_builder_set_member_name (builder, "description");
        json_builder_add_string_value (builder, review->description);
        json_builder_end_object (builder);
    }
    if (app->reviews->len == 0) {
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "user_skey");
        json_builder_add_string_value (builder, user_skey);
        json_builder_end_object (builder);
    }
    json_builder_end_array (builder);

    g_autoptr(JsonGenerator) generator = json_generator_new ();
    g_autoptr(JsonNode) response_root = json_builder_get_root (builder);
    json_generator_set_root (generator, response_root);
    gsize json_text_length;
    g_autofree gchar *json_text = json_generator_to_data (generator, &json_text_length);

    soup_message_set_status (msg, SOUP_STATUS_OK);
    soup_message_set_response (msg, "application/json; charset=utf-8", SOUP_MEMORY_TAKE, g_steal_pointer (&json_text), json_text_length);
}

static void
submit_cb (SoupServer *server G_GNUC_UNUSED, SoupMessage *msg, const gchar *path G_GNUC_UNUSED, GHashTable *query G_GNUC_UNUSED, SoupClientContext *context G_GNUC_UNUSED, gpointer user_data)
{
    MockOdrsServer *self = user_data;

    if (msg->method != SOUP_METHOD_POST) {
        soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonNode) root = decode_request (msg, &error);
    if (root == NULL) {
        respond (msg, FALSE, NULL);
        return;
    }

    JsonObject *object = json_node_get_object (root);
    const gchar *user_hash = json_object_get_string_member (object, "user_hash");
    const gchar *user_skey = json_object_get_string_member (object, "user_skey");
    const gchar *app_id = json_object_get_string_member (object, "app_id");
    const gchar *locale = json_object_get_string_member (object, "locale");
    const gchar *distro = json_object_get_string_member (object, "distro");
    const gchar *version = json_object_get_string_member (object, "version");
    const gchar *user_display = json_object_get_string_member (object, "user_display");
    const gchar *summary = json_object_get_string_member (object, "summary");
    const gchar *description = json_object_get_string_member (object, "description");
    gint64 rating = json_object_get_int_member (object, "rating");

    if (!validate_user (user_hash, user_skey)) {
        respond (msg, FALSE, NULL);
        return;
    }

    MockApp *app = mock_odrs_server_add_app (self, app_id);
    MockReview *review = mock_app_add_review (app);
    mock_review_set_locale (review, locale);
    mock_review_set_distro (review, distro);
    mock_review_set_version (review, version);
    mock_review_set_user_display (review, user_display);
    mock_review_set_summary (review, summary);
    mock_review_set_description (review, description);
    mock_review_set_rating (review, rating);

    respond (msg, TRUE, NULL);
}

static void
feedback_cb (SoupServer *server G_GNUC_UNUSED, SoupMessage *msg, const gchar *path, GHashTable *query G_GNUC_UNUSED, SoupClientContext *context G_GNUC_UNUSED, gpointer user_data)
{
    MockOdrsServer *self = user_data;

    if (msg->method != SOUP_METHOD_POST) {
        soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonNode) root = decode_request (msg, &error);
    if (root == NULL) {
        respond (msg, FALSE, NULL);
        return;
    }

    JsonObject *object = json_node_get_object (root);
    const gchar *user_hash = json_object_get_string_member (object, "user_hash");
    const gchar *user_skey = json_object_get_string_member (object, "user_skey");
    const gchar *app_id = json_object_get_string_member (object, "app_id");
    gint64 review_id = json_object_get_int_member (object, "review_id");

    if (!validate_user (user_hash, user_skey)) {
        respond (msg, FALSE, NULL);
        return;
    }

    MockApp *app = mock_odrs_server_find_app (self, app_id);
    if (app == NULL) {
        respond (msg, FALSE, NULL);
        return;
    }

    MockReview *review = mock_app_find_review (app, review_id);
    if (review == NULL) {
        respond (msg, FALSE, NULL);
        return;
    }

    if (g_strcmp0 (path, "/upvote") == 0) {
        review->upvote_count++;
    }
    else if (g_strcmp0 (path, "/downvote") == 0) {
        review->downvote_count++;
    }
    else if (g_strcmp0 (path, "/report") == 0) {
        review->report_count++;
    }

    respond (msg, TRUE, NULL);
}

static void
mock_odrs_server_dispose (GObject *object)
{
    MockOdrsServer *self = MOCK_ODRS_SERVER (object);

    g_clear_pointer (&self->apps, g_ptr_array_unref);

    G_OBJECT_CLASS (mock_odrs_server_parent_class)->dispose (object);
}

static void
mock_odrs_server_class_init (MockOdrsServerClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = mock_odrs_server_dispose;
}

static void
mock_odrs_server_init (MockOdrsServer *self)
{
    self->apps = g_ptr_array_new_with_free_func ((GDestroyNotify) mock_app_free);

    g_object_set (self, "server-header", "mock-odrs", NULL);
    soup_server_add_handler (SOUP_SERVER (self), "/ratings", ratings_cb, self, NULL);
    soup_server_add_handler (SOUP_SERVER (self), "/fetch", fetch_cb, self, NULL);
    soup_server_add_handler (SOUP_SERVER (self), "/submit", submit_cb, self, NULL);
    soup_server_add_handler (SOUP_SERVER (self), "/upvote", feedback_cb, self, NULL);
    soup_server_add_handler (SOUP_SERVER (self), "/downvote", feedback_cb, self, NULL);
    soup_server_add_handler (SOUP_SERVER (self), "/report", feedback_cb, self, NULL);
}

MockOdrsServer *
mock_odrs_server_new (void)
{
    return g_object_new (mock_odrs_server_get_type (), NULL);
}

gboolean
mock_odrs_server_start (MockOdrsServer *self, GError **error)
{
    g_return_val_if_fail (MOCK_IS_ODRS_SERVER (self), FALSE);

    return soup_server_listen_local (SOUP_SERVER (self), 0, 0, error);
}

guint
mock_odrs_server_get_port (MockOdrsServer *self)
{
    g_return_val_if_fail (MOCK_IS_ODRS_SERVER (self), 0);

    GSList *uris = soup_server_get_uris (SOUP_SERVER (self));

    guint port = 0;
    for (GSList *link = uris; link != NULL; link = link->next) {
        SoupURI *uri = link->data;
        port = soup_uri_get_port (uri);
    }

    g_slist_free_full (uris, (GDestroyNotify) soup_uri_free);

    return port;
}

MockApp *
mock_odrs_server_add_app (MockOdrsServer *self, const gchar *id)
{
    g_return_val_if_fail (MOCK_IS_ODRS_SERVER (self), NULL);

    MockApp *app = mock_odrs_server_find_app (self, id);
    if (app == NULL) {
        app = mock_app_new (id);
        g_ptr_array_add (self->apps, app);
    }

    return app;
}

MockApp *
mock_odrs_server_find_app (MockOdrsServer *self, const gchar *id)
{
    g_return_val_if_fail (MOCK_IS_ODRS_SERVER (self), NULL);

    for (guint i = 0; i < self->apps->len; i++) {
        MockApp *app = g_ptr_array_index (self->apps, i);
        if (g_strcmp0 (app->id, id) == 0)
            return app;
    }

    return NULL;
}

MockReview *
mock_app_add_review (MockApp *app)
{
    MockReview *review = mock_review_new ();
    g_ptr_array_add (app->reviews, review);
    return review;
}

MockReview *
mock_app_find_review (MockApp *app, gint64 id)
{
    for (guint i = 0; i < app->reviews->len; i++) {
        MockReview *review = g_ptr_array_index (app->reviews, i);
        if (review->id == id)
            return review;
    }

    return NULL;
}

void
mock_review_set_locale (MockReview *review, const gchar *locale)
{
    g_free (review->locale);
    review->locale = g_strdup (locale);
}

void
mock_review_set_distro (MockReview *review, const gchar *distro)
{
    g_free (review->distro);
    review->distro = g_strdup (distro);
}

void
mock_review_set_version (MockReview *review, const gchar *version)
{
    g_free (review->version);
    review->version = g_strdup (version);
}

void
mock_review_set_user_display (MockReview *review, const gchar *user_display)
{
    g_free (review->user_display);
    review->user_display = g_strdup (user_display);
}

void
mock_review_set_summary (MockReview *review, const gchar *summary)
{
    g_free (review->summary);
    review->summary = g_strdup (summary);
}

void
mock_review_set_description (MockReview *review, const gchar *description)
{
    g_free (review->description);
    review->description = g_strdup (description);
}

void
mock_review_set_rating (MockReview *review, gint64 rating)
{
    review->rating = rating;
}

int
main (int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED)
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockOdrsServer) server = mock_odrs_server_new ();
    g_autoptr(GError) error = NULL;
    if (!mock_odrs_server_start (server, &error)) {
        g_printerr ("Failed to start server: %s\n", error->message);
        return EXIT_FAILURE;
    }
    g_printerr ("Listening on port %u\n", mock_odrs_server_get_port (server));

    g_main_loop_run (loop);

    return EXIT_SUCCESS;
}
