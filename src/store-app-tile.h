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

G_DECLARE_FINAL_TYPE (StoreAppTile, store_app_tile, STORE, APP_TILE, GtkEventBox)

StoreAppTile *store_app_tile_new       (void);

void          store_app_tile_set_cache (StoreAppTile *tile, StoreCache *cache);

void          store_app_tile_set_app   (StoreAppTile *tile, StoreApp *app);

StoreApp     *store_app_tile_get_app   (StoreAppTile *tile);

G_END_DECLS
