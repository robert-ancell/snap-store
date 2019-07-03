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

#include "store-category.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreCategoryTile, store_category_tile, STORE, CATEGORY_TILE, GtkEventBox)

StoreCategoryTile *store_category_tile_new          (void);

void               store_category_tile_set_category (StoreCategoryTile *tile, StoreCategory *category);

StoreCategory     *store_category_tile_get_category (StoreCategoryTile *tile);

G_END_DECLS
