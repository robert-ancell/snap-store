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

G_DECLARE_FINAL_TYPE (StoreCategoryGrid, store_category_grid, STORE, CATEGORY_GRID, GtkFlowBoxChild)

StoreCategoryGrid *store_category_grid_new       (void);

void               store_category_grid_set_cache (StoreCategoryGrid *grid, StoreCache *cache);

void               store_category_grid_set_name  (StoreCategoryGrid *grid, const gchar *name);

const gchar       *store_category_grid_get_name  (StoreCategoryGrid *grid);

void               store_category_grid_set_title (StoreCategoryGrid *grid, const gchar *title);

void               store_category_grid_set_apps  (StoreCategoryGrid *grid, GPtrArray *apps);

G_END_DECLS
