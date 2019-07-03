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

G_BEGIN_DECLS

G_DECLARE_DERIVABLE_TYPE (StorePage, store_page, STORE, PAGE, GtkBin)

struct _StorePageClass
{
    GtkBinClass parent_class;

    void (*set_cache) (StorePage *page, StoreCache *cache);
};

void        store_page_set_cache (StorePage *page, StoreCache *cache);

StoreCache *store_page_get_cache (StorePage *page);

G_END_DECLS
