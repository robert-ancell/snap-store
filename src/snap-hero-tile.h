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

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapHeroTile, snap_hero_tile, SNAP, HERO_TILE, GtkFlowBoxChild)

SnapHeroTile *snap_hero_tile_new      (const gchar *name);

void          snap_hero_tile_set_name (SnapHeroTile *tile, const gchar *name);

G_END_DECLS
