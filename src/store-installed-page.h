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
#include "store-snap-pool.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreInstalledPage, store_installed_page, STORE, INSTALLED_PAGE, GtkBox)

void store_installed_page_set_cache     (StoreInstalledPage *page, StoreCache *cache);

void store_installed_page_set_snap_pool (StoreInstalledPage *page, StoreSnapPool *pool);

void store_installed_page_load          (StoreInstalledPage *page);

G_END_DECLS
