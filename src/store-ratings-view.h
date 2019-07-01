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

void store_ratings_view_set_review_count_one_star   (StoreRatingsView *view, const gint64 count);

void store_ratings_view_set_review_count_two_star   (StoreRatingsView *view, const gint64 count);

void store_ratings_view_set_review_count_three_star (StoreRatingsView *view, const gint64 count);

void store_ratings_view_set_review_count_four_star  (StoreRatingsView *view, const gint64 count);

void store_ratings_view_set_review_count_five_star  (StoreRatingsView *view, const gint64 count);


G_END_DECLS
