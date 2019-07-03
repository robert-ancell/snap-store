/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <gio/gio.h>

#include "store-odrs-review.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreOdrsClient, store_odrs_client, STORE, ODRS_CLIENT, GObject)

StoreOdrsClient *store_odrs_client_new                  (void);

void             store_odrs_client_set_distro           (StoreOdrsClient *client, const gchar *distro);

void             store_odrs_client_set_locale           (StoreOdrsClient *client, const gchar *locale);

gint64          *store_odrs_client_get_ratings          (StoreOdrsClient *client, const gchar *app_id);

void             store_odrs_client_update_ratings_async (StoreOdrsClient *client,
                                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean         store_odrs_client_update_ratings_finish (StoreOdrsClient *client, GAsyncResult *result, GError **error);

void             store_odrs_client_get_reviews_async    (StoreOdrsClient *client, const gchar *app_id, GStrv compat_ids, const gchar *version, gint64 limit,
                                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

GPtrArray       *store_odrs_client_get_reviews_finish   (StoreOdrsClient *client, GAsyncResult *result, gchar **user_skey, GError **error);

void             store_odrs_client_submit_async         (StoreOdrsClient *client, const gchar *user_skey, const gchar *app_id, const gchar *version, const gchar *user_display, const gchar *summary, const gchar *description, gint64 rating,
                                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean         store_odrs_client_submit_finish        (StoreOdrsClient *client, GAsyncResult *result, GError **error);

void             store_odrs_client_upvote_async         (StoreOdrsClient *client, const gchar *user_skey, const gchar *app_id, gint64 review_id,
                                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean         store_odrs_client_upvote_finish        (StoreOdrsClient *client, GAsyncResult *result, GError **error);

void             store_odrs_client_downvote_async       (StoreOdrsClient *client, const gchar *user_skey, const gchar *app_id, gint64 review_id,
                                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean         store_odrs_client_downvote_finish      (StoreOdrsClient *client, GAsyncResult *result, GError **error);

G_END_DECLS
