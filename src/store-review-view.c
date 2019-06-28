/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-review-view.h"

#include "store-rating-label.h"

struct _StoreReviewView
{
    GtkBox parent_instance;

    GtkLabel *author_date_label;
    GtkLabel *description_label;
    StoreRatingLabel *rating_label;
    GtkLabel *summary_label;

    StoreOdrsReview *review;
};

G_DEFINE_TYPE (StoreReviewView, store_review_view, GTK_TYPE_BOX)

static void
store_review_view_dispose (GObject *object)
{
    StoreReviewView *self = STORE_REVIEW_VIEW (object);

    g_clear_object (&self->review);

    G_OBJECT_CLASS (store_review_view_parent_class)->dispose (object);
}

static void
store_review_view_class_init (StoreReviewViewClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_review_view_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-review-view.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewView, author_date_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewView, description_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewView, rating_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewView, summary_label);
}

static void
store_review_view_init (StoreReviewView *self)
{
    store_rating_label_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreReviewView *
store_review_view_new (void)
{
    return g_object_new (store_review_view_get_type (), NULL);
}

void
store_review_view_set_review (StoreReviewView *self, StoreOdrsReview *review)
{
    g_return_if_fail (STORE_IS_REVIEW_VIEW (self));

    if (self->review == review)
        return;

    g_clear_object (&self->review);
    if (review != NULL)
        self->review = g_object_ref (review);

    store_rating_label_set_rating (self->rating_label, store_odrs_review_get_rating (review));
    gtk_label_set_label (self->summary_label, store_odrs_review_get_summary (review));
    g_autofree gchar *author_date_text = NULL;
    GDateTime *date_created = store_odrs_review_get_date_created (review);
    if (date_created != NULL) {
        g_autofree gchar *date_text = g_date_time_format (date_created, "%e %B %Y");
        author_date_text = g_strdup_printf ("%s - %s", store_odrs_review_get_author (review), date_text);
    }
    else
       author_date_text = g_strdup (store_odrs_review_get_author (review));
    gtk_label_set_label (self->author_date_label, author_date_text);
    gtk_label_set_label (self->description_label, store_odrs_review_get_description (review));
}

StoreOdrsReview *
store_review_view_get_review (StoreReviewView *self)
{
    g_return_val_if_fail (STORE_IS_REVIEW_VIEW (self), NULL);
    return self->review;
}
