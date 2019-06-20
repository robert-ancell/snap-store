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

G_DECLARE_FINAL_TYPE (SnapAppTile, snap_app_tile, SNAP, APP_TILE, GtkFlowBoxChild)

SnapAppTile *snap_app_tile_new      (const gchar *name);

void         snap_app_tile_set_name (SnapAppTile *tile, const gchar *name);

G_END_DECLS
