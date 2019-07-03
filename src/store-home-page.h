/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "store-odrs-client.h"
#include "store-page.h"
#include "store-snap-pool.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreHomePage, store_home_page, STORE, HOME_PAGE, StorePage)

void store_home_page_set_categories  (StoreHomePage *page, GPtrArray *categories);

void store_home_page_set_odrs_client (StoreHomePage *page, StoreOdrsClient *odrs_client);

void store_home_page_set_snap_pool   (StoreHomePage *page, StoreSnapPool *pool);

void store_home_page_load            (StoreHomePage *page);

G_END_DECLS
