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

G_DECLARE_FINAL_TYPE (StoreRatingsView, store_ratings_view, STORE, RATINGS_VIEW, GtkGrid)

void store_ratings_view_set_ratings (StoreRatingsView *view, const gint64 *ratings);

G_END_DECLS
