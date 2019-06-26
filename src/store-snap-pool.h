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

#include "store-snap-app.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreSnapPool, store_snap_pool, STORE, SNAP_POOL, GObject)

StoreSnapPool *store_snap_pool_new      (void);

StoreSnapApp  *store_snap_pool_get_snap (StoreSnapPool *pool, const gchar *name);

G_END_DECLS
