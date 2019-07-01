/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <glib-object.h>

#include "store-cache.h"
#include "store-channel.h"
#include "store-media.h"

G_BEGIN_DECLS

G_DECLARE_DERIVABLE_TYPE (StoreApp, store_app, STORE, APP, GObject)

struct _StoreAppClass
{
    GObjectClass parent_class;

    void      (*install_async)     (StoreApp *app, StoreChannel *channel, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);
    gboolean  (*install_finish)    (StoreApp *app, GAsyncResult *result, GError **error);
    void      (*refresh_async)     (StoreApp *app, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);
    gboolean  (*refresh_finish)    (StoreApp *app, GAsyncResult *result, GError **error);
    void      (*remove_async)      (StoreApp *app, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);
    gboolean  (*remove_finish)     (StoreApp *app, GAsyncResult *result, GError **error);
    void      (*save_to_cache)     (StoreApp *app, StoreCache *cache);
    void      (*update_from_cache) (StoreApp *app, StoreCache *cache);
};

void          store_app_install_async               (StoreApp *app, StoreChannel *channel, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean      store_app_install_finish              (StoreApp *app, GAsyncResult *result, GError **error);

void          store_app_refresh_async               (StoreApp *app, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean      store_app_refresh_finish              (StoreApp *app, GAsyncResult *result, GError **error);

void          store_app_remove_async                (StoreApp *app, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean      store_app_remove_finish               (StoreApp *app, GAsyncResult *result, GError **error);

void          store_app_save_to_cache               (StoreApp *app, StoreCache *cache);

void          store_app_update_from_cache           (StoreApp *app, StoreCache *cache);

void          store_app_set_appstream_id            (StoreApp *app, const gchar *appstream_id);

const gchar  *store_app_get_appstream_id            (StoreApp *app);

void          store_app_set_channels                (StoreApp *app, GPtrArray *channels);

GPtrArray    *store_app_get_channels                (StoreApp *app);

void          store_app_set_contact                 (StoreApp *app, const gchar *contact);

const gchar  *store_app_get_contact                 (StoreApp *app);

void          store_app_set_description             (StoreApp *app, const gchar *description);

const gchar  *store_app_get_description             (StoreApp *app);

void          store_app_set_icon                    (StoreApp *app, StoreMedia *icon);

StoreMedia   *store_app_get_icon                    (StoreApp *app);

void          store_app_set_installed               (StoreApp *app, gboolean installed);

gboolean      store_app_get_installed               (StoreApp *app);

void          store_app_set_license                 (StoreApp *app, const gchar *license);

const gchar  *store_app_get_license                 (StoreApp *app);

void          store_app_set_name                    (StoreApp *app, const gchar *name);

const gchar  *store_app_get_name                    (StoreApp *app);

void          store_app_set_publisher               (StoreApp *app, const gchar *publisher);

const gchar  *store_app_get_publisher               (StoreApp *app);

void          store_app_set_publisher_validated     (StoreApp *app, gboolean validated);

gboolean      store_app_get_publisher_validated     (StoreApp *app);

void          store_app_set_one_star_review_count   (StoreApp *app, const gint64 count);

void          store_app_set_two_star_review_count   (StoreApp *app, const gint64 count);

void          store_app_set_three_star_review_count (StoreApp *app, const gint64 count);

void          store_app_set_four_star_review_count  (StoreApp *app, const gint64 count);

void          store_app_set_five_star_review_count  (StoreApp *app, const gint64 count);

gint          store_app_get_ratings_average         (StoreApp *app);

gint64        store_app_get_ratings_total           (StoreApp *app);

void          store_app_set_screenshots             (StoreApp *app, GPtrArray *screenshots);

GPtrArray    *store_app_get_screenshots             (StoreApp *app);

void          store_app_set_summary                 (StoreApp *app, const gchar *summary);

const gchar  *store_app_get_summary                 (StoreApp *app);

void          store_app_set_title                   (StoreApp *app, const gchar *title);

const gchar  *store_app_get_title                   (StoreApp *app);

G_END_DECLS
