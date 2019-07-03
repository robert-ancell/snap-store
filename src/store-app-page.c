/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <glib/gi18n.h>
#include <math.h>

#include "store-app-page.h"

#include "store-channel-combo.h"
#include "store-image.h"
#include "store-rating-label.h"
#include "store-review-summary.h"
#include "store-review-view.h"
#include "store-screenshot-view.h"

struct _StoreAppPage
{
    StorePage parent_instance;

    StoreChannelCombo *channel_combo;
    GtkLabel *contact_label;
    GtkLabel *description_label;
    GtkLabel *details_installed_size_label;
    GtkLabel *details_license_label;
    GtkLabel *details_publisher_label;
    GtkLabel *details_updated_label;
    GtkLabel *details_version_label;
    StoreImage *icon_image;
    GtkButton *install_button;
    GtkButton *launch_button;
    GtkLabel *publisher_label;
    GtkImage *publisher_validated_image;
    StoreRatingLabel *rating_label;
    GtkButton *remove_button;
    GtkBox *review_count_label;
    StoreReviewSummary *review_summary;
    GtkBox *reviews_box;
    StoreScreenshotView *screenshot_view;
    GtkLabel *summary_label;
    GtkLabel *title_label;

    StoreApp *app;
    GCancellable *cancellable;
};

enum
{
    PROP_0,
    PROP_REVIEWS,
    PROP_LAST
};

G_DEFINE_TYPE (StoreAppPage, store_app_page, store_page_get_type ())

static gboolean
date_to_label (GBinding *binding G_GNUC_UNUSED, const GValue *from_value, GValue *to_value, gpointer user_data G_GNUC_UNUSED)
{
    GDateTime *date = g_value_get_boxed (from_value);
    if (date == NULL) {
        g_value_set_string (to_value, "");
        return TRUE;
    }

    g_autofree gchar *text = g_date_time_format (date, "%-e %B %Y");
    g_value_set_string (to_value, text);

    return TRUE;
}

static gboolean
installed_size_to_label (GBinding *binding G_GNUC_UNUSED, const GValue *from_value, GValue *to_value, gpointer user_data G_GNUC_UNUSED)
{
    gint64 size = g_value_get_int64 (from_value);
    if (size <= 0) {
        g_value_set_string (to_value, "");
        return TRUE;
    }

    g_autofree gchar *text = NULL;
    if (size >= 1000000000)
        text = g_strdup_printf ("%.0f GB", round (size / 1000000000.0));
    else if (size >= 1000000)
        text = g_strdup_printf ("%.0f MB", round (size / 1000000.0));
    else if (size >= 1000)
        text = g_strdup_printf ("%.0f kB", round (size / 1000.0));
    else
        text = g_strdup_printf ("%" G_GINT64_FORMAT " B", size);

    g_value_set_string (to_value, text);

    return TRUE;
}

static gboolean
ratings_total_to_label (GBinding *binding G_GNUC_UNUSED, const GValue *from_value, GValue *to_value, gpointer user_data G_GNUC_UNUSED)
{
    gint64 count = g_value_get_int64 (from_value);
    if (count > 0) {
        g_autofree gchar *text = g_strdup_printf ("(%" G_GINT64_FORMAT ")", count);
        g_value_set_string (to_value, text);
    }
    else
        g_value_set_string (to_value, "");
    return TRUE;
}

static void
refresh_cb (GObject *object, GAsyncResult *result, gpointer user_data G_GNUC_UNUSED)
{
    g_autoptr(GError) error = NULL;
    if (!store_app_refresh_finish (STORE_APP (object), result, &error)) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to refresh app: %s", error->message);
        return;
    }
}

static void
store_app_page_set_reviews (StoreAppPage *self, GPtrArray *reviews)
{
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->reviews_box));
    for (GList *link = children; link != NULL; link = link->next) {
        GtkWidget *child = link->data;
        gtk_container_remove (GTK_CONTAINER (self->reviews_box), child);
    }
    for (guint i = 0; i < reviews->len; i++) {
        StoreOdrsReview *review = g_ptr_array_index (reviews, i);
        StoreReviewView *view = store_review_view_new ();
        gtk_widget_show (GTK_WIDGET (view));
        store_review_view_set_review (view, review);
        gtk_container_add (GTK_CONTAINER (self->reviews_box), GTK_WIDGET (view));
    }
    gtk_widget_set_visible (GTK_WIDGET (self->reviews_box), reviews->len > 0);
}

static void
contact_link_cb (StoreAppPage *self, const gchar *uri)
{
    g_autoptr(GError) error = NULL;
    if (!gtk_show_uri_on_window (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))), uri, GDK_CURRENT_TIME, &error))
        g_warning ("Failed to show contact URI %s: %s", uri, error->message);
}

static void
install_cb (StoreAppPage *self)
{
    store_app_install_async (self->app, NULL, NULL, NULL, NULL);
}

static void
launch_cb (StoreAppPage *self)
{
    g_autoptr(GError) error = NULL;
    if (!store_app_launch (self->app, &error))
        g_warning ("Failed to launch app: %s", error->message); // FIXME: Show graphically
}

static void
remove_cb (StoreAppPage *self)
{
    store_app_remove_async (self->app, NULL, NULL, NULL);
}

static void
store_app_page_dispose (GObject *object)
{
    StoreAppPage *self = STORE_APP_PAGE (object);

    g_clear_object (&self->app);
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);

    G_OBJECT_CLASS (store_app_page_parent_class)->dispose (object);
}

static void
store_app_page_get_property (GObject *object, guint prop_id, GValue *value G_GNUC_UNUSED, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
store_app_page_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreAppPage *self = STORE_APP_PAGE (object);

    switch (prop_id)
    {
    case PROP_REVIEWS:
        store_app_page_set_reviews (self, g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_app_page_set_model (StorePage *page, StoreModel *model)
{
    StoreAppPage *self = STORE_APP_PAGE (page);

    store_image_set_model (self->icon_image, model);
    store_screenshot_view_set_model (self->screenshot_view, model);
    // FIXME: Should apply to children

    STORE_PAGE_CLASS (store_app_page_parent_class)->set_model (page, model);
}

static void
store_app_page_class_init (StoreAppPageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_page_dispose;
    G_OBJECT_CLASS (klass)->get_property = store_app_page_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_app_page_set_property;
    STORE_PAGE_CLASS (klass)->set_model = store_app_page_set_model;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_REVIEWS,
                                     g_param_spec_boxed ("reviews", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_WRITABLE));

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-app-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, channel_combo);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, contact_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, description_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, details_installed_size_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, details_license_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, details_publisher_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, details_updated_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, details_version_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, install_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, launch_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, publisher_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, publisher_validated_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, rating_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, remove_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, review_count_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, review_summary);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, reviews_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, screenshot_view);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, summary_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, title_label);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), contact_link_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), install_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), launch_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), remove_cb);
}

static void
store_app_page_init (StoreAppPage *self)
{
    store_channel_combo_get_type ();
    store_image_get_type ();
    store_page_get_type ();
    store_rating_label_get_type ();
    store_review_summary_get_type ();
    store_screenshot_view_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_app_page_set_app (StoreAppPage *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_APP_PAGE (self));

    if (self->app == app)
        return;

    g_set_object (&self->app, app);

    g_cancellable_cancel (self->cancellable);
    self->cancellable = g_cancellable_new ();
    store_app_refresh_async (app, self->cancellable, refresh_cb, self);

    g_object_bind_property (app, "title", self->title_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "publisher", self->publisher_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "publisher-validated", self->publisher_validated_image, "visible", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "summary", self->summary_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "description", self->description_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "icon", self->icon_image, "media", G_BINDING_SYNC_CREATE);

    g_object_bind_property (app, "version", self->details_version_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property_full (app, "updated-date", self->details_updated_label, "label", G_BINDING_SYNC_CREATE, date_to_label, NULL, NULL, NULL); // FIXME: Support updated for uninstalled snaps
    g_object_bind_property (app, "license", self->details_license_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "publisher", self->details_publisher_label, "label", G_BINDING_SYNC_CREATE);
    g_object_bind_property_full (app, "installed-size", self->details_installed_size_label, "label", G_BINDING_SYNC_CREATE, installed_size_to_label, NULL, NULL, NULL); // FIXME: Support download size for uninstalled snaps

    g_object_bind_property (app, "review-average", self->rating_label, "rating", G_BINDING_SYNC_CREATE);
    g_object_bind_property_full (app, "review-count", self->review_count_label, "label", G_BINDING_SYNC_CREATE, ratings_total_to_label, NULL, NULL, NULL);
    g_object_bind_property (app, "review-count-one-star", self->review_summary, "review-count-one-star", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "review-count-two-star", self->review_summary, "review-count-two-star", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "review-count-three-star", self->review_summary, "review-count-three-star", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "review-count-four-star", self->review_summary, "review-count-four-star", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "review-count-five-star", self->review_summary, "review-count-five-star", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "reviews", self, "reviews", G_BINDING_SYNC_CREATE);

    if (store_app_get_contact (app) != NULL) {
        /* Link shown below app description to contact app publisher. */
        const gchar *contact_label = _("Contact");
        g_autofree gchar *link_text = g_markup_printf_escaped ("<a href=\"%s\">%s</a>", store_app_get_contact (app), contact_label);
        gtk_label_set_label (self->contact_label, link_text);
        gtk_widget_show (GTK_WIDGET (self->contact_label));
    }
    else
        gtk_widget_hide (GTK_WIDGET (self->contact_label));

    g_object_bind_property (app, "channels", self->channel_combo, "channels", G_BINDING_SYNC_CREATE);

    g_object_bind_property (app, "installed", self->channel_combo, "visible", G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
    g_object_bind_property (app, "installed", self->launch_button, "visible", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "installed", self->remove_button, "visible", G_BINDING_SYNC_CREATE);
    g_object_bind_property (app, "installed", self->install_button, "visible", G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

    gtk_widget_hide (GTK_WIDGET (self->reviews_box));

    store_model_update_reviews_async (store_page_get_model (STORE_PAGE (self)), app, self->cancellable, NULL, NULL);

    store_screenshot_view_set_app (self->screenshot_view, app);
    GPtrArray *screenshots = store_app_get_screenshots (app);
    gtk_widget_set_visible (GTK_WIDGET (self->screenshot_view), screenshots->len > 0);
}

StoreApp *
store_app_page_get_app (StoreAppPage *self)
{
    g_return_val_if_fail (STORE_IS_APP_PAGE (self), NULL);
    return self->app;
}
