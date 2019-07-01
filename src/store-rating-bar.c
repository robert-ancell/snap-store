/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-rating-bar.h"

struct _StoreRatingBar
{
    GtkDrawingArea parent_instance;

    gint64 count;
    gint64 total;
};

G_DEFINE_TYPE (StoreRatingBar, store_rating_bar, GTK_TYPE_DRAWING_AREA)

static void
store_rating_bar_get_preferred_height (GtkWidget *widget G_GNUC_UNUSED, gint *minimum_height, gint *natural_height)
{
    *minimum_height = *natural_height = 18;
}

static void
store_rating_bar_get_preferred_width (GtkWidget *widget G_GNUC_UNUSED, gint *minimum_width, gint *natural_width)
{
    *minimum_width = *natural_width = 140;
}

static gboolean
store_rating_bar_draw (GtkWidget *widget, cairo_t *cr)
{
    StoreRatingBar *self = STORE_RATING_BAR (widget);

    int width = gtk_widget_get_allocated_width (widget);
    int height = gtk_widget_get_allocated_height (widget);
    gtk_render_background (gtk_widget_get_style_context (widget), cr, 0, 0, width, height);

    gdouble w = 0;
    if (self->total != 0)
        w = width * self->count / self->total; // FIXME round
    cairo_rectangle (cr, 0, 0, w, height);
    GdkRGBA color;
    gtk_style_context_get_color (gtk_widget_get_style_context (widget), gtk_widget_get_state_flags (widget), &color);
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_fill (cr);

    return TRUE;
}

static void
store_rating_bar_class_init (StoreRatingBarClass *klass)
{
    GTK_WIDGET_CLASS (klass)->get_preferred_height = store_rating_bar_get_preferred_height;
    GTK_WIDGET_CLASS (klass)->get_preferred_width = store_rating_bar_get_preferred_width;
    GTK_WIDGET_CLASS (klass)->draw = store_rating_bar_draw;
}

static void
store_rating_bar_init (StoreRatingBar *self)
{
    gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)), "rating-bar");
}

void
store_rating_bar_set_count (StoreRatingBar *self, gint count)
{
    g_return_if_fail (STORE_IS_RATING_BAR (self));

    if (self->count == count)
        return;
    self->count = count;

    gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
store_rating_bar_set_total (StoreRatingBar *self, gint total)
{
    g_return_if_fail (STORE_IS_RATING_BAR (self));

    if (self->total == total)
        return;
    self->total = total;

    gtk_widget_queue_draw (GTK_WIDGET (self));
}
