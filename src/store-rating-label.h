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

G_DECLARE_FINAL_TYPE (StoreRatingLabel, store_rating_label, STORE, RATING_LABEL, GtkLabel)

void   store_rating_label_set_rating (StoreRatingLabel *label, gint64 rating);

gint64 store_rating_label_get_rating (StoreRatingLabel *label);

G_END_DECLS
