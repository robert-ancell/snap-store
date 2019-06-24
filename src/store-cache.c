/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-cache.h"

struct _StoreCache
{
    GObject parent_instance;
};

G_DEFINE_TYPE (StoreCache, store_cache, G_TYPE_OBJECT)

static gchar *
get_cache_path (const gchar *type, const gchar *name, gboolean hash)
{
    if (hash) {
        g_autofree gchar *hashed_name = g_compute_checksum_for_string (G_CHECKSUM_SHA1, name, -1);
        return g_build_filename (g_get_user_cache_dir (), "snap-store", type, hashed_name, NULL);
    }
    else
        return g_build_filename (g_get_user_cache_dir (), "snap-store", type, name, NULL);
}

static void
store_cache_class_init (StoreCacheClass *klass G_GNUC_UNUSED)
{
}

static void
store_cache_init (StoreCache *self G_GNUC_UNUSED)
{
}

StoreCache *
store_cache_new (void)
{
    return g_object_new (store_cache_get_type (), NULL);
}

void
store_cache_insert (StoreCache *self, const gchar *type, const gchar *name, gboolean hash, GBytes *data)
{
    g_return_if_fail (STORE_IS_CACHE (self));

    g_autofree gchar *path = get_cache_path (type, name, hash);

    g_autofree gchar *dir = g_path_get_dirname (path);
    g_mkdir_with_parents (dir, 0700);

    gsize contents_length;
    const gchar *contents = g_bytes_get_data (data, &contents_length);
    g_autoptr(GError) error = NULL;
    if (!g_file_set_contents (path, contents, contents_length, &error)) {
        g_warning ("Failed to write cache entry %s[%s] of size %" G_GSIZE_FORMAT ": %s", type, name, contents_length, error->message);
    }
}

GBytes *
store_cache_lookup (StoreCache *self, const gchar *type, const gchar *name, gboolean hash)
{
    g_return_val_if_fail (STORE_IS_CACHE (self), NULL);

    g_autofree gchar *path = get_cache_path (type, name, hash);

    g_autoptr(GError) error = NULL;
    g_autofree gchar *contents = NULL;
    gsize contents_length;
    if (!g_file_get_contents (path, &contents, &contents_length, &error)) {
        if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            g_warning ("Failed to read cache entry %s[%s]: %s", type, name, error->message);
        return NULL;
    }

    return g_bytes_new_take (g_steal_pointer (&contents), contents_length);
}
