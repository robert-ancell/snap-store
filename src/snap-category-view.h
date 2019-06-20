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

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapCategoryView, snap_category_view, SNAP, CATEGORY_VIEW, GtkFlowBoxChild)

SnapCategoryView *snap_category_view_new      (const gchar *name);

void              snap_category_view_set_name (SnapCategoryView *view, const gchar *name);

void              snap_category_view_set_hero (SnapCategoryView *view, const gchar *name);

void              snap_category_view_set_apps (SnapCategoryView *view, GStrv names);

G_END_DECLS
