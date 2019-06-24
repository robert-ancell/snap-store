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

void        store_cache_insert        (StoreCache *cache, const gchar *type, const gchar *name, gboolean hash, GBytes *data);

void        store_cache_insert_json   (StoreCache *cache, const gchar *type, const gchar *name, gboolean hash, JsonNode *node);

GBytes     *store_cache_lookup        (StoreCache *cache, const gchar *type, const gchar *name, gboolean hash);

JsonNode   *store_cache_lookup_json   (StoreCache *cache, const gchar *type, const gchar *name, gboolean hash);

G_END_DECLS
