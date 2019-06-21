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

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreAppPage, store_app_page, STORE, APP_PAGE, GtkBox)

StoreAppPage *store_app_page_new     (void);

void          store_app_page_set_app (StoreAppPage *page, StoreApp *app);

StoreApp     *store_app_page_get_app (StoreAppPage *page);

G_END_DECLS
