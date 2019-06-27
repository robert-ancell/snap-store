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

    gint64 rating;
};

G_DEFINE_TYPE (StoreRatingLabel, store_rating_label, GTK_TYPE_LABEL)

static void
store_rating_label_class_init (StoreRatingLabelClass *klass G_GNUC_UNUSED)
{
}

static void
store_rating_label_init (StoreRatingLabel *self)
{
    gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)), "rating-label");

    store_rating_label_set_rating (self, 0);
}

void
store_rating_label_set_rating (StoreRatingLabel *self, gint64 rating)
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

gint64
store_rating_label_get_rating (StoreRatingLabel *self)
{
    g_return_val_if_fail (STORE_IS_RATING_LABEL (self), -1);
    return self->rating;
}
