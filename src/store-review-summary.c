/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-review-summary.h"

#include "store-rating-bar.h"
#include "store-rating-label.h"

struct _StoreReviewSummary
{
    GtkGrid parent_instance;

    StoreRatingBar *five_star_bar;
    GtkLabel *five_star_count_label;
    StoreRatingBar *four_star_bar;
    GtkLabel *four_star_count_label;
    StoreRatingBar *one_star_bar;
    GtkLabel *one_star_count_label;
    StoreRatingBar *three_star_bar;
    GtkLabel *three_star_count_label;
    StoreRatingBar *two_star_bar;
    GtkLabel *two_star_count_label;

    gint64 one_star_review_count;
    gint64 two_star_review_count;
    gint64 three_star_review_count;
    gint64 four_star_review_count;
    gint64 five_star_review_count;
};

enum
{
    PROP_0,
    PROP_REVIEW_COUNT_ONE_STAR,
    PROP_REVIEW_COUNT_TWO_STAR,
    PROP_REVIEW_COUNT_THREE_STAR,
    PROP_REVIEW_COUNT_FOUR_STAR,
    PROP_REVIEW_COUNT_FIVE_STAR,
    PROP_LAST
};


G_DEFINE_TYPE (StoreReviewSummary, store_review_summary, GTK_TYPE_GRID)

static void
update_total (StoreReviewSummary *self)
{
    gint64 total = self->one_star_review_count + self->two_star_review_count + self->three_star_review_count + self->four_star_review_count + self->five_star_review_count;
    store_rating_bar_set_total (self->one_star_bar, total);
    store_rating_bar_set_total (self->two_star_bar, total);
    store_rating_bar_set_total (self->three_star_bar, total);
    store_rating_bar_set_total (self->four_star_bar, total);
    store_rating_bar_set_total (self->five_star_bar, total);
}

static void
set_label_int64 (GtkLabel *label, gint64 value)
{
    g_autofree gchar *text = g_strdup_printf ("%" G_GINT64_FORMAT, value);
    gtk_label_set_label (label, text);
}

static void
store_review_summary_get_property (GObject *object, guint prop_id, GValue *value G_GNUC_UNUSED, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
store_review_summary_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreReviewSummary *self = STORE_REVIEW_SUMMARY (object);

    switch (prop_id)
    {
    case PROP_REVIEW_COUNT_ONE_STAR:
        store_review_summary_set_review_count_one_star (self, g_value_get_int64 (value));
        break;
    case PROP_REVIEW_COUNT_TWO_STAR:
        store_review_summary_set_review_count_two_star (self, g_value_get_int64 (value));
        break;
    case PROP_REVIEW_COUNT_THREE_STAR:
        store_review_summary_set_review_count_three_star (self, g_value_get_int64 (value));
        break;
    case PROP_REVIEW_COUNT_FOUR_STAR:
        store_review_summary_set_review_count_four_star (self, g_value_get_int64 (value));
        break;
    case PROP_REVIEW_COUNT_FIVE_STAR:
        store_review_summary_set_review_count_five_star (self, g_value_get_int64 (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_review_summary_class_init (StoreReviewSummaryClass *klass)
{
    G_OBJECT_CLASS (klass)->get_property = store_review_summary_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_review_summary_set_property;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_REVIEW_COUNT_ONE_STAR,
                                     g_param_spec_int64 ("review-count-one-star", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_REVIEW_COUNT_TWO_STAR,
                                     g_param_spec_int64 ("review-count-two-star", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_REVIEW_COUNT_THREE_STAR,
                                     g_param_spec_int64 ("review-count-three-star", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_REVIEW_COUNT_FOUR_STAR,
                                     g_param_spec_int64 ("review-count-four-star", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_REVIEW_COUNT_FIVE_STAR,
                                     g_param_spec_int64 ("review-count-five-star", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-review-summary.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, five_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, five_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, four_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, four_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, one_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, one_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, three_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, three_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, two_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreReviewSummary, two_star_count_label);
}

static void
store_review_summary_init (StoreReviewSummary *self)
{
    store_rating_bar_get_type ();
    store_rating_label_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_review_summary_set_review_count_one_star (StoreReviewSummary *self, gint64 count)
{
    g_return_if_fail (STORE_IS_REVIEW_SUMMARY (self));

    self->one_star_review_count = count;
    set_label_int64 (self->one_star_count_label, count);
    store_rating_bar_set_count (self->one_star_bar, count);
    update_total (self);
}

void
store_review_summary_set_review_count_two_star (StoreReviewSummary *self, gint64 count)
{
    g_return_if_fail (STORE_IS_REVIEW_SUMMARY (self));

    self->two_star_review_count = count;
    set_label_int64 (self->two_star_count_label, count);
    store_rating_bar_set_count (self->two_star_bar, count);
    update_total (self);
}

void
store_review_summary_set_review_count_three_star (StoreReviewSummary *self, gint64 count)
{
    g_return_if_fail (STORE_IS_REVIEW_SUMMARY (self));

    self->three_star_review_count = count;
    set_label_int64 (self->three_star_count_label, count);
    store_rating_bar_set_count (self->three_star_bar, count);
    update_total (self);
}

void
store_review_summary_set_review_count_four_star (StoreReviewSummary *self, gint64 count)
{
    g_return_if_fail (STORE_IS_REVIEW_SUMMARY (self));

    self->four_star_review_count = count;
    set_label_int64 (self->four_star_count_label, count);
    store_rating_bar_set_count (self->four_star_bar, count);
    update_total (self);
}

void
store_review_summary_set_review_count_five_star (StoreReviewSummary *self, gint64 count)
{
    g_return_if_fail (STORE_IS_REVIEW_SUMMARY (self));

    self->five_star_review_count = count;
    set_label_int64 (self->five_star_count_label, count);
    store_rating_bar_set_count (self->five_star_bar, count);
    update_total (self);
}
