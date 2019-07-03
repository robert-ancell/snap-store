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
#include "store-model.h"
#include "store-image.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreBannerTile, store_banner_tile, STORE, BANNER_TILE, GtkEventBox)

void      store_banner_tile_set_app   (StoreBannerTile *tile, StoreApp *app);

StoreApp *store_banner_tile_get_app   (StoreBannerTile *tile);

void      store_banner_tile_set_model (StoreBannerTile *tile, StoreModel *model);

G_END_DECLS
