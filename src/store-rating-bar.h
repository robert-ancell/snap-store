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

G_DECLARE_FINAL_TYPE (StoreRatingBar, store_rating_bar, STORE, RATING_BAR, GtkDrawingArea)

void store_rating_bar_set_count (StoreRatingBar *bar, gint count);

void store_rating_bar_set_total (StoreRatingBar *bar, gint total);

G_END_DECLS
