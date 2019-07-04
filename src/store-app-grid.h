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

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreAppGrid, store_app_grid, STORE, APP_GRID, GtkGrid)

StoreAppGrid *store_app_grid_new       (void);

void          store_app_grid_set_apps  (StoreAppGrid *grid, GPtrArray *apps);

void          store_app_grid_set_model (StoreAppGrid *grid, StoreModel *model);

G_END_DECLS
