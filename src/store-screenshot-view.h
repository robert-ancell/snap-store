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

G_DECLARE_FINAL_TYPE (StoreScreenshotView, store_screenshot_view, STORE, SCREENSHOT_VIEW, GtkBox)

StoreScreenshotView *store_screenshot_view_new       (void);

void                 store_screenshot_view_set_cache (StoreScreenshotView *view, StoreCache *cache);

void                 store_screenshot_view_set_app   (StoreScreenshotView *view, StoreApp *app);

G_END_DECLS
