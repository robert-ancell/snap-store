/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreCategory, store_category, STORE, CATEGORY, GObject)

StoreCategory *store_category_new (void);

void           store_category_set_apps    (StoreCategory *category, GPtrArray *apps);

GPtrArray     *store_category_get_apps    (StoreCategory *category);

void           store_category_set_name    (StoreCategory *category, const gchar *name);

const gchar   *store_category_get_name    (StoreCategory *category);

void           store_category_set_summary (StoreCategory *category, const gchar *summary);

const gchar   *store_category_get_summary (StoreCategory *category);

void           store_category_set_title   (StoreCategory *category, const gchar *title);

const gchar   *store_category_get_title   (StoreCategory *category);

G_END_DECLS
