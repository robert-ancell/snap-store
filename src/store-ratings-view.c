/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-ratings-view.h"

#include "store-rating-label.h"

struct _StoreRatingsView
{
    GtkGrid parent_instance;

    GtkLabel *five_star_count_label;
    GtkLabel *four_star_count_label;
    GtkLabel *one_star_count_label;
    GtkLabel *three_star_count_label;
    GtkLabel *two_star_count_label;
};

enum
{
    PROP_0,
    PROP_RATINGS,
    PROP_LAST
};


G_DEFINE_TYPE (StoreRatingsView, store_ratings_view, GTK_TYPE_GRID)

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
    case PROP_RATINGS:
        store_ratings_view_set_ratings (self, g_value_get_pointer (value));
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
                                     PROP_RATINGS,
                                     g_param_spec_pointer ("ratings", NULL, NULL, G_PARAM_WRITABLE));

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-ratings-view.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, five_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, four_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, one_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, three_star_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreRatingsView, two_star_count_label);
}

static void
store_ratings_view_init (StoreRatingsView *self)
{
    store_rating_label_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_ratings_view_set_ratings (StoreRatingsView *self, const gint64 *ratings)
{
    g_return_if_fail (STORE_IS_RATINGS_VIEW (self));

    set_label_int64 (self->one_star_count_label, ratings[0]);
    set_label_int64 (self->two_star_count_label, ratings[1]);
    set_label_int64 (self->three_star_count_label, ratings[2]);
    set_label_int64 (self->four_star_count_label, ratings[3]);
    set_label_int64 (self->five_star_count_label, ratings[4]);
}
