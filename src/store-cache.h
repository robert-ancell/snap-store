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
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreCache, store_cache, STORE, CACHE, GObject)

StoreCache *store_cache_new           (void);

gboolean    store_cache_insert        (StoreCache *cache, const gchar *type, const gchar *name, gboolean hash, GBytes *data, GCancellable *cancellable, GError **error);

gboolean    store_cache_insert_json   (StoreCache *cache, const gchar *type, const gchar *name, gboolean hash, JsonNode *node, GCancellable *cancellable, GError **error);

void        store_cache_lookup_async  (StoreCache *cache, const gchar *type, const gchar *name, gboolean hash,
                                       GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

GBytes     *store_cache_lookup_finish (StoreCache *cache, GAsyncResult *result, GError **error);

GBytes     *store_cache_lookup_sync   (StoreCache *cache, const gchar *type, const gchar *name, gboolean hash, GCancellable *cancellable, GError **error);

JsonNode   *store_cache_lookup_json   (StoreCache *cache, const gchar *type, const gchar *name, gboolean hash, GCancellable *cancellable, GError **error);

G_END_DECLS
