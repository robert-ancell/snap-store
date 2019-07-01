/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-ratings-view.h"

#include "store-rating-bar.h"
#include "store-rating-label.h"

struct _StoreRatingsView
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
    PROP_ONE_STAR_REVIEW_COUNT,
    PROP_TWO_STAR_REVIEW_COUNT,
    PROP_THREE_STAR_REVIEW_COUNT,
    PROP_FOUR_STAR_REVIEW_COUNT,
    PROP_FIVE_STAR_REVIEW_COUNT,
    PROP_LAST
};


G_DEFINE_TYPE (StoreRatingsView, store_ratings_view, GTK_TYPE_GRID)

static gint64
get_review_count_total (StoreRatingsView *self)
{
    return self->one_star_review_count + self->two_star_review_count + self->three_star_review_count + self->four_star_review_count + self->five_star_review_count;
}

static void
set_label_int64 (GtkLabel *label, gint64 value)
{
    g_autofree gchar *text = g_strdup_printf ("%" G_GINT64_FORMAT, value);
    gtk_label_set_label (label, text);
}

static void
store_ratings_view_get_property (GObject *object, guint prop_id, GValue *value G_GNUC_UNUSED, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
store_ratings_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreRatingsView *self = STORE_RATINGS_VIEW (object);

    switch (prop_id)
    {
    case PROP_ONE_STAR_REVIEW_COUNT:
        store_ratings_view_set_one_star_review_count (self, g_value_get_int64 (value));
        break;
    case PROP_TWO_STAR_REVIEW_COUNT:
        store_ratings_view_set_two_star_review_count (self, g_value_get_int64 (value));
        break;
    case PROP_THREE_STAR_REVIEW_COUNT:
        store_ratings_view_set_three_star_review_count (self, g_value_get_int64 (value));
        break;
    case PROP_FOUR_STAR_REVIEW_COUNT:
        store_ratings_view_set_four_star_review_count (self, g_value_get_int64 (value));
        break;
    case PROP_FIVE_STAR_REVIEW_COUNT:
        store_ratings_view_set_five_star_review_count (self, g_value_get_int64 (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_ratings_view_class_init (StoreRatingsViewClass *klass)
{
    G_OBJECT_CLASS (klass)->get_property = store_ratings_view_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_ratings_view_set_property;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_ONE_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("one-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_TWO_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("two-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_THREE_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("three-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_FOUR_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("four-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_FIVE_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("five-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-ratings-view.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, five_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, five_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, four_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, four_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, one_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, one_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, three_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, three_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, two_star_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, two_star_count_label);
}

static void
store_ratings_view_init (StoreRatingsView *self)
{
    store_rating_bar_get_type ();
    store_rating_label_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_ratings_view_set_one_star_review_count (StoreRatingsView *self, gint64 count)
{
    g_return_if_fail (STORE_IS_RATINGS_VIEW (self));

    self->one_star_review_count = count;
    set_label_int64 (self->one_star_count_label, count);
    store_rating_bar_set_count (self->one_star_bar, count);
    store_rating_bar_set_total (self->one_star_bar, get_review_count_total (self));
}

void
store_ratings_view_set_two_star_review_count (StoreRatingsView *self, gint64 count)
{
    g_return_if_fail (STORE_IS_RATINGS_VIEW (self));

    self->two_star_review_count = count;
    set_label_int64 (self->two_star_count_label, count);
    store_rating_bar_set_count (self->two_star_bar, count);
    store_rating_bar_set_total (self->two_star_bar, get_review_count_total (self));
}

void
store_ratings_view_set_three_star_review_count (StoreRatingsView *self, gint64 count)
{
    g_return_if_fail (STORE_IS_RATINGS_VIEW (self));

    self->three_star_review_count = count;
    set_label_int64 (self->three_star_count_label, count);
    store_rating_bar_set_count (self->three_star_bar, count);
    store_rating_bar_set_total (self->three_star_bar, get_review_count_total (self));
}

void
store_ratings_view_set_four_star_review_count (StoreRatingsView *self, gint64 count)
{
    g_return_if_fail (STORE_IS_RATINGS_VIEW (self));

    self->four_star_review_count = count;
    set_label_int64 (self->four_star_count_label, count);
    store_rating_bar_set_count (self->four_star_bar, count);
    store_rating_bar_set_total (self->four_star_bar, get_review_count_total (self));
}

void
store_ratings_view_set_five_star_review_count (StoreRatingsView *self, gint64 count)
{
    g_return_if_fail (STORE_IS_RATINGS_VIEW (self));

    self->five_star_review_count = count;
    set_label_int64 (self->five_star_count_label, count);
    store_rating_bar_set_count (self->five_star_bar, count);
    store_rating_bar_set_total (self->five_star_bar, get_review_count_total (self));
}