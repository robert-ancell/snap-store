/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-review-view.h"

struct _StoreReviewView
{
    GtkBox parent_instance;

    GtkLabel *author_label;
    GtkLabel *description_label;
    GtkLabel *summary_label;

    StoreOdrsReview *review;
};

G_DEFINE_TYPE (StoreReviewView, store_review_view, GTK_TYPE_BOX)

static void
store_review_view_dispose (GObject *object)
{
    StoreReviewView *self = STORE_REVIEW_VIEW (object);
    g_clear_object (&self->review);
}

static void
store_review_view_class_init (StoreReviewViewClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_review_view_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/store-review-view.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewView, author_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewView, description_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewView, summary_label);
}

static void
store_review_view_init (StoreReviewView *self)
{
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

    gtk_label_set_label (self->author_label, store_odrs_review_get_author (review));
    gtk_label_set_label (self->summary_label, store_odrs_review_get_summary (review));
    gtk_label_set_label (self->description_label, store_odrs_review_get_description (review));
}

StoreOdrsReview *
store_review_view_get_review (StoreReviewView *self)
{
    g_return_val_if_fail (STORE_IS_REVIEW_VIEW (self), NULL);
    return self->review;
}
