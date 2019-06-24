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

#include "store-odrs-review.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreReviewView, store_review_view, STORE, REVIEW_VIEW, GtkBox)

StoreReviewView *store_review_view_new        (void);

void             store_review_view_set_review (StoreReviewView *view, StoreOdrsReview *review);

StoreOdrsReview *store_review_view_get_review (StoreReviewView *view);

G_END_DECLS
