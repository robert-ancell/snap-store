/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "store-page.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreCategoriesPage, store_categories_page, STORE, CATEGORIES_PAGE, StorePage)

void store_categories_page_set_categories (StoreCategoriesPage *self, GPtrArray *categories);

G_END_DECLS
