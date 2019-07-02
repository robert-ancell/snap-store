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

G_DECLARE_FINAL_TYPE (StoreCategoryList, store_category_list, STORE, CATEGORY_LIST, GtkBox)

StoreCategoryList *store_category_list_new       (void);

void               store_category_list_set_cache (StoreCategoryList *list, StoreCache *cache);

void               store_category_list_set_name  (StoreCategoryList *list, const gchar *name);

const gchar       *store_category_list_get_name  (StoreCategoryList *list);

void               store_category_list_set_title (StoreCategoryList *list, const gchar *title);

void               store_category_list_set_apps  (StoreCategoryList *list, GPtrArray *apps);

G_END_DECLS
