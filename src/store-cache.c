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

static GFile *
get_cache_file (const gchar *type, const gchar *name, gboolean hash)
{
    g_autofree gchar *filename = NULL;
    if (hash) {
        g_autofree gchar *hashed_name = g_compute_checksum_for_string (G_CHECKSUM_SHA1, name, -1);
        filename = g_build_filename (g_get_user_cache_dir (), "snap-store", type, hashed_name, NULL);
    }
    else
        filename = g_build_filename (g_get_user_cache_dir (), "snap-store", type, name, NULL);
    return g_file_new_for_path (filename);
}

static void
contents_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    GTask *task = user_data;

    g_autoptr(GError) error = NULL;
    g_autofree gchar *contents = NULL;
    gsize contents_length;
    if (!g_file_load_contents_finish (G_FILE (object), result, &contents, &contents_length, NULL, &error)) {
        g_task_return_error (task, g_steal_pointer (&error));
        return;
    }

    g_task_return_pointer (task, g_bytes_new_take (g_steal_pointer (&contents), contents_length), (GDestroyNotify) g_bytes_unref);
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

gboolean
store_cache_insert (StoreCache *self, const gchar *type, const gchar *name, gboolean hash, GBytes *data, GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (STORE_IS_CACHE (self), FALSE);

    g_autoptr(GFile) file = get_cache_file (type, name, hash);

    g_autofree gchar *dir = g_path_get_dirname (g_file_get_path (file));
    g_mkdir_with_parents (dir, 0700);

    gsize contents_length;
    const gchar *contents = g_bytes_get_data (data, &contents_length);
    return g_file_replace_contents (file, contents, contents_length, NULL, FALSE, G_FILE_CREATE_PRIVATE, NULL, cancellable, error);
}

gboolean
store_cache_insert_json (StoreCache *self, const gchar *type, const gchar *name, gboolean hash, JsonNode *node, GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (STORE_IS_CACHE (self), FALSE);

    g_autoptr(JsonGenerator) generator = json_generator_new ();
    json_generator_set_root (generator, node);
    gsize text_length;
    g_autofree gchar *text = json_generator_to_data (generator, &text_length);
    g_autoptr(GBytes) data = g_bytes_new_static (text, text_length);
    return store_cache_insert (self, type, name, hash, data, cancellable, error);
}

void
store_cache_lookup_async (StoreCache *self, const gchar *type, const gchar *name, gboolean hash,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_CACHE (self));

    g_autoptr(GFile) file = get_cache_file (type, name, hash);

    GTask *task = g_task_new (self, cancellable, callback, callback_data);
    g_file_load_contents_async (file, cancellable, contents_cb, task);
}

GBytes *
store_cache_lookup_finish (StoreCache *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_CACHE (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), self), FALSE);

    return g_task_propagate_pointer (G_TASK (result), error);
}

GBytes *
store_cache_lookup_sync (StoreCache *self, const gchar *type, const gchar *name, gboolean hash, GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (STORE_IS_CACHE (self), NULL);

    g_autoptr(GFile) file = get_cache_file (type, name, hash);

    g_autofree gchar *contents = NULL;
    gsize contents_length;
    if (!g_file_load_contents (file, cancellable, &contents, &contents_length, NULL, error))
        return NULL;

    return g_bytes_new_take (g_steal_pointer (&contents), contents_length);
}

JsonNode *
store_cache_lookup_json (StoreCache *self, const gchar *type, const gchar *name, gboolean hash, GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (STORE_IS_CACHE (self), NULL);

    g_autoptr(GBytes) value = store_cache_lookup_sync (self, type, name, hash, cancellable, error);
    if (value == NULL)
        return NULL;

    g_autoptr(JsonParser) parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, g_bytes_get_data (value, NULL), g_bytes_get_size (value), error))
        return NULL;

    JsonNode *root = json_parser_get_root (parser);
    if (root == NULL)
        return NULL;

    return json_node_ref (root);
}
