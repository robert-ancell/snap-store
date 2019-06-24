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

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreHeroTile, store_hero_tile, STORE, HERO_TILE, GtkEventBox)

StoreHeroTile *store_hero_tile_new      (void);

void           store_hero_tile_set_app (StoreHeroTile *tile, StoreApp *app);

StoreApp      *store_hero_tile_get_app (StoreHeroTile *tile);

G_END_DECLS
