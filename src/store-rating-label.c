/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-rating-label.h"

struct _StoreRatingLabel
{
    GtkLabel parent_instance;

    gint rating;
};

enum
{
    PROP_0,
    PROP_RATING,
    PROP_LAST
};

G_DEFINE_TYPE (StoreRatingLabel, store_rating_label, GTK_TYPE_LABEL)

static void
store_rating_label_get_property (GObject *object, guint prop_id, GValue *value G_GNUC_UNUSED, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
store_rating_label_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreRatingLabel *self = STORE_RATING_LABEL (object);

    switch (prop_id)
    {
    case PROP_RATING:
        store_rating_label_set_rating (self, g_value_get_int (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_rating_label_class_init (StoreRatingLabelClass *klass)
{
    G_OBJECT_CLASS (klass)->get_property = store_rating_label_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_rating_label_set_property;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_RATING,
                                     g_param_spec_int ("rating", NULL, NULL, G_MININT, G_MAXINT, 0, G_PARAM_WRITABLE));
}

static void
store_rating_label_init (StoreRatingLabel *self)
{
    store_rating_label_set_rating (self, 0);
}

void
store_rating_label_set_rating (StoreRatingLabel *self, gint rating)
{
    g_return_if_fail (STORE_IS_RATING_LABEL (self));

    self->rating = rating;

    const gchar *stars;
    if (rating > 90)
        stars = "★★★★★";
    else if (rating > 70)
        stars = "★★★★☆";
    else if (rating > 50)
        stars = "★★★☆☆";
    else if (rating > 30)
        stars = "★★☆☆☆";
    else if (rating > 10)
        stars = "★☆☆☆☆";
    else
        stars = "☆☆☆☆☆";
    gtk_label_set_label (GTK_LABEL (self), stars);
}
