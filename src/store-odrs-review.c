/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-odrs-review.h"

struct _StoreOdrsReview
{
    GObject parent_instance;

    gchar *author;
    GDateTime *date_created;
    gchar *description;
    gchar *summary;
    gint64 rating;
};

G_DEFINE_TYPE (StoreOdrsReview, store_odrs_review, G_TYPE_OBJECT)

static void
store_odrs_review_dispose (GObject *object)
{
    StoreOdrsReview *self = STORE_ODRS_REVIEW (object);
    g_clear_pointer (&self->author, g_free);
    g_clear_pointer (&self->date_created, g_date_time_unref);
    g_clear_pointer (&self->description, g_free);
    g_clear_pointer (&self->summary, g_free);
}

static void
store_odrs_review_class_init (StoreOdrsReviewClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_odrs_review_dispose;
}

static void
store_odrs_review_init (StoreOdrsReview *self)
{
    self->author = g_strdup ("");
    self->description = g_strdup ("");
    self->summary = g_strdup ("");
}

StoreOdrsReview *
store_odrs_review_new (void)
{
    return g_object_new (store_odrs_review_get_type (), NULL);
}

void
store_odrs_review_set_author (StoreOdrsReview *self, const gchar *author)
{
    g_return_if_fail (STORE_IS_ODRS_REVIEW (self));
    g_clear_pointer (&self->author, g_free);
    self->author = g_strdup (author);
}

const gchar *
store_odrs_review_get_author (StoreOdrsReview *self)
{
    g_return_val_if_fail (STORE_IS_ODRS_REVIEW (self), NULL);
    return self->author;
}

void
store_odrs_review_set_date_created (StoreOdrsReview *self, GDateTime *date_created)
{
    g_return_if_fail (STORE_IS_ODRS_REVIEW (self));
    g_clear_pointer (&self->date_created, g_date_time_unref);
    self->date_created = g_date_time_ref (date_created);
}

GDateTime *
store_odrs_review_get_date_created (StoreOdrsReview *self)
{
    g_return_val_if_fail (STORE_IS_ODRS_REVIEW (self), NULL);
    return self->date_created;
}

void
store_odrs_review_set_description (StoreOdrsReview *self, const gchar *description)
{
    g_return_if_fail (STORE_IS_ODRS_REVIEW (self));
    g_clear_pointer (&self->description, g_free);
    self->description = g_strdup (description);
}

const gchar *
store_odrs_review_get_description (StoreOdrsReview *self)
{
    g_return_val_if_fail (STORE_IS_ODRS_REVIEW (self), NULL);
    return self->description;
}

void
store_odrs_review_set_summary (StoreOdrsReview *self, const gchar *summary)
{
    g_return_if_fail (STORE_IS_ODRS_REVIEW (self));
    g_clear_pointer (&self->summary, g_free);
    self->summary = g_strdup (summary);
}

const gchar *
store_odrs_review_get_summary (StoreOdrsReview *self)
{
    g_return_val_if_fail (STORE_IS_ODRS_REVIEW (self), NULL);
    return self->summary;
}

void
store_odrs_review_set_rating (StoreOdrsReview *self, gint64 rating)
{
    g_return_if_fail (STORE_IS_ODRS_REVIEW (self));
    self->rating = rating;
}

gint64
store_odrs_review_get_rating (StoreOdrsReview *self)
{
    g_return_val_if_fail (STORE_IS_ODRS_REVIEW (self), -1);
    return self->rating;
}
