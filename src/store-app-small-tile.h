/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <gtk/gtk.h>

#include "store-app.h"
#include "store-cache.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreAppSmallTile, store_app_small_tile, STORE, APP_SMALL_TILE, GtkEventBox)

StoreAppSmallTile *store_app_small_tile_new       (void);

void               store_app_small_tile_set_cache (StoreAppSmallTile *tile, StoreCache *cache);

void               store_app_small_tile_set_app   (StoreAppSmallTile *tile, StoreApp *app);

StoreApp          *store_app_small_tile_get_app   (StoreAppSmallTile *tile);

G_END_DECLS
