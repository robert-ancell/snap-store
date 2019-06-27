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

G_DECLARE_FINAL_TYPE (StoreCategoryView, store_category_view, STORE, CATEGORY_VIEW, GtkFlowBoxChild)

StoreCategoryView *store_category_view_new       (void);

void               store_category_view_set_cache (StoreCategoryView *view, StoreCache *cache);

void               store_category_view_set_name  (StoreCategoryView *view, const gchar *name);

const gchar       *store_category_view_get_name  (StoreCategoryView *view);

void               store_category_view_set_title (StoreCategoryView *view, const gchar *title);

void               store_category_view_set_hero  (StoreCategoryView *view, StoreApp *app);

void               store_category_view_set_apps  (StoreCategoryView *view, GPtrArray *apps);

G_END_DECLS
