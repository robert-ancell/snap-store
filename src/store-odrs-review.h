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

G_DECLARE_FINAL_TYPE (StoreOdrsReview, store_odrs_review, STORE, ODRS_REVIEW, GObject)

StoreOdrsReview *store_odrs_review_new              (void);

void             store_odrs_review_set_author       (StoreOdrsReview *review, const gchar *author);

const gchar     *store_odrs_review_get_author       (StoreOdrsReview *review);

void             store_odrs_review_set_date_created (StoreOdrsReview *review, GDateTime *date_created);

GDateTime       *store_odrs_review_get_date_created (StoreOdrsReview *review);

void             store_odrs_review_set_description  (StoreOdrsReview *review, const gchar *description);

const gchar     *store_odrs_review_get_description  (StoreOdrsReview *review);

void             store_odrs_review_set_summary      (StoreOdrsReview *review, const gchar *summary);

const gchar     *store_odrs_review_get_summary      (StoreOdrsReview *review);

void             store_odrs_review_set_rating       (StoreOdrsReview *review, gint64 rating);

gint64           store_odrs_review_get_rating       (StoreOdrsReview *review);

G_END_DECLS
