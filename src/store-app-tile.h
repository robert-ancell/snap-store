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

G_DECLARE_FINAL_TYPE (StoreAppTile, store_app_tile, STORE, APP_TILE, GtkFlowBoxChild)

StoreAppTile *store_app_tile_new      (const gchar *name);

void          store_app_tile_set_name (StoreAppTile *tile, const gchar *name);

G_END_DECLS
