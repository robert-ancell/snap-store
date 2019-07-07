/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>

#include "store-cache.h"
#include "store-category.h"
#include "store-snap-app.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE   (StoreModel, store_model, STORE, MODEL, GObject)

StoreModel    *store_model_new                            (void);

void           store_model_load                           (StoreModel *model);

void           store_model_set_cache                      (StoreModel *model, StoreCache *cache);

StoreCache    *store_model_get_cache                      (StoreModel *model);

void           store_model_set_odrs_server_uri            (StoreModel *model, const gchar *uri);

const gchar   *store_model_get_odrs_server_uri            (StoreModel *model);

void           store_model_set_snapd_socket_path          (StoreModel *model, const gchar *path);

StoreSnapApp  *store_model_get_snap                       (StoreModel *model, const gchar *name);

GPtrArray     *store_model_get_categories                 (StoreModel *model);

void           store_model_update_categories_async        (StoreModel *model,
                                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean       store_model_update_categories_finish       (StoreModel *model, GAsyncResult *result, GError **error);

GPtrArray     *store_model_get_installed                  (StoreModel *model);

void           store_model_update_installed_async         (StoreModel *model,
                                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean       store_model_update_installed_finish        (StoreModel *model, GAsyncResult *result, GError **error);

void           store_model_update_ratings_async           (StoreModel *model,
                                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean       store_model_update_ratings_finish          (StoreModel *model, GAsyncResult *result, GError **error);

void           store_model_update_reviews_async           (StoreModel *model, StoreApp *app,
                                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean       store_model_update_reviews_finish          (StoreModel *model, GAsyncResult *result, GError **error);

void           store_model_search_async                   (StoreModel *model, const gchar *query,
                                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

GPtrArray     *store_model_search_finish                  (StoreModel *model, GAsyncResult *result, GError **error);

void           store_model_get_image_async                (StoreModel *model, const gchar *uri, gint width, gint height,
                                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

GdkPixbuf     *store_model_get_image_finish               (StoreModel *model, GAsyncResult *result, GError **error);

G_END_DECLS
