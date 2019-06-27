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

#include "store-application.h"
#include "store-cache.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreWindow, store_window, STORE, WINDOW, GtkApplicationWindow)

StoreWindow *store_window_new       (StoreApplication *application);

void         store_window_set_cache (StoreWindow *window, StoreCache *cache);

void         store_window_load      (StoreWindow *self);

G_END_DECLS
