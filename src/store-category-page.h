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

#include "store-cache.h"
#include "store-category.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreCategoryPage, store_category_page, STORE, CATEGORY_PAGE, GtkBox)

void store_category_page_set_cache    (StoreCategoryPage *page, StoreCache *cache);

void store_category_page_set_category (StoreCategoryPage *page, StoreCategory *category);

G_END_DECLS
