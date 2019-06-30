/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-snap-pool.h"

struct _StoreSnapPool
{
    GObject parent_instance;

    GHashTable *snaps;
};

G_DEFINE_TYPE (StoreSnapPool, store_snap_pool, G_TYPE_OBJECT)

static void
store_snap_pool_dispose (GObject *object)
{
    StoreSnapPool *self = STORE_SNAP_POOL (object);

    g_clear_pointer (&self->snaps, g_hash_table_unref);

    G_OBJECT_CLASS (store_snap_pool_parent_class)->dispose (object);
}

static void
store_snap_pool_class_init (StoreSnapPoolClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_snap_pool_dispose;
}

static void
store_snap_pool_init (StoreSnapPool *self)
{
    self->snaps = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

StoreSnapPool *
store_snap_pool_new (void)
{
    return g_object_new (store_snap_pool_get_type (), NULL);
}

StoreSnapApp *
store_snap_pool_get_snap (StoreSnapPool *self, const gchar *name)
{
    g_return_val_if_fail (STORE_IS_SNAP_POOL (self), NULL);

    StoreSnapApp *snap = g_hash_table_lookup (self->snaps, name);
    if (snap == NULL) {
        snap = store_snap_app_new ();
        store_app_set_name (STORE_APP (snap), name);
        g_hash_table_insert (self->snaps, g_strdup (name), snap); // FIXME: Use a weak ref to clean out when no-longer used
    }

    return g_object_ref (snap);
}

GPtrArray *
store_snap_pool_get_snaps (StoreSnapPool *self)
{
    g_return_val_if_fail (STORE_IS_SNAP_POOL (self), NULL);

    g_autoptr(GPtrArray) snaps = g_ptr_array_new_with_free_func (g_object_unref);
    GHashTableIter iter;
    g_hash_table_iter_init (&iter, self->snaps);
    gpointer key, value;
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        StoreSnapApp *snap = value;
        g_ptr_array_add (snaps, g_object_ref (snap));
    }

    return g_steal_pointer (&snaps);
}
