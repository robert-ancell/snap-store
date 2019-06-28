/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <glib/gi18n.h>

#include "store-app-page.h"

#include "store-channel-combo.h"
#include "store-image.h"
#include "store-install-button.h"
#include "store-odrs-client.h"
#include "store-rating-label.h"
#include "store-review-view.h"
#include "store-screenshot-view.h"

struct _StoreAppPage
{
    GtkBox parent_instance;

    StoreChannelCombo *channel_combo;
    GtkLabel *contact_label;
    GtkLabel *description_label;
    StoreImage *icon_image;
    StoreInstallButton *install_button;
    GtkLabel *license_label;
    GtkLabel *publisher_label;
    GtkImage *publisher_validated_image;
    StoreRatingLabel *rating_label;
    GtkBox *reviews_box;
    StoreScreenshotView *screenshot_view;
    GtkLabel *summary_label;
    GtkLabel *title_label;

    StoreApp *app;
    StoreCache *cache;
    GCancellable *cancellable;
};

G_DEFINE_TYPE (StoreAppPage, store_app_page, GTK_TYPE_BOX)

static void
refresh_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreAppPage *self = user_data;

    g_autoptr(GError) error = NULL;
    if (!store_app_refresh_finish (STORE_APP (object), result, &error)) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to refresh app: %s", error->message);
        return;
    }

    if (self->cache != NULL)
        store_app_save_to_cache (self->app, self->cache);
}

static void
set_reviews (StoreAppPage *self, GPtrArray *reviews)
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
reviews_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreAppPage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autofree gchar *user_skey = NULL;
    g_autoptr(GPtrArray) reviews = store_odrs_client_get_reviews_finish (STORE_ODRS_CLIENT (object), result, &user_skey, &error);
    if (reviews == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to get ODRS reviews: %s", error->message);
        return;
    }
    // FIXME: Store and use review key

    set_reviews (self, reviews);

    /* Save in cache */
    if (self->cache != NULL) {
        g_autoptr(JsonBuilder) builder = json_builder_new ();
        json_builder_begin_array (builder);
        for (guint i = 0; i < reviews->len; i++) {
            StoreOdrsReview *review = g_ptr_array_index (reviews, i);
            json_builder_add_value (builder, store_odrs_review_to_json (review));
        }
        json_builder_end_array (builder);
        g_autoptr(JsonNode) root = json_builder_get_root (builder);
        store_cache_insert_json (self->cache, "reviews", store_app_get_name (self->app), FALSE, root, NULL, NULL);
    }
}

static void
contact_link_cb (StoreAppPage *self, const gchar *uri)
{
    g_autoptr(GError) error = NULL;
    if (!gtk_show_uri_on_window (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))), uri, GDK_CURRENT_TIME, &error))
        g_warning ("Failed to show contact URI %s: %s", uri, error->message);
}

static void
store_app_page_dispose (GObject *object)
{
    StoreAppPage *self = STORE_APP_PAGE (object);

    g_clear_object (&self->app);
    g_clear_object (&self->cache);
    g_cancellable_cancel (self->cancellable);
    self->cancellable = g_cancellable_new ();

    G_OBJECT_CLASS (store_app_page_parent_class)->dispose (object);
}

static void
store_app_page_class_init (StoreAppPageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_page_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-app-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, channel_combo);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, contact_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, description_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, install_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, license_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, publisher_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, publisher_validated_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, rating_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, reviews_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, screenshot_view);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, summary_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, title_label);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), contact_link_cb);
}

static void
store_app_page_init (StoreAppPage *self)
{
    store_channel_combo_get_type ();
    store_image_get_type ();
    store_install_button_get_type ();
    store_rating_label_get_type ();
    store_screenshot_view_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreAppPage *
store_app_page_new (void)
{
    return g_object_new (store_app_page_get_type (), NULL);
}

void
store_app_page_set_cache (StoreAppPage *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_APP_PAGE (self));
    g_set_object (&self->cache, cache);
    store_image_set_cache (self->icon_image, cache);
    // FIXME: Should apply to children
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

    gtk_label_set_label (self->title_label, store_app_get_title (app));
    gtk_label_set_label (self->license_label, store_app_get_license (app));
    gtk_label_set_label (self->publisher_label, store_app_get_publisher (app));
    gtk_widget_set_visible (GTK_WIDGET (self->publisher_validated_image), store_app_get_publisher_validated (app));
    gtk_label_set_label (self->summary_label, store_app_get_summary (app));
    gtk_label_set_label (self->description_label, store_app_get_description (app));
    store_image_set_uri (self->icon_image, NULL); // FIXME: Hack to reset icon
    if (store_app_get_icon (app) != NULL)
        store_image_set_uri (self->icon_image, store_media_get_uri (store_app_get_icon (app)));

    if (store_app_get_contact (app) != NULL) {
        /* Link shown below app description to contact app publisher. */
        const gchar *contact_label = _("Contact");
        g_autofree gchar *link_text = g_markup_printf_escaped ("<a href=\"%s\">%s</a>", store_app_get_contact (app), contact_label);
        gtk_label_set_label (self->contact_label, link_text);
        gtk_widget_show (GTK_WIDGET (self->contact_label));
    }
    else
        gtk_widget_hide (GTK_WIDGET (self->contact_label));

    store_channel_combo_set_app (self->channel_combo, app);

    store_install_button_set_app (self->install_button, app);

    gtk_widget_hide (GTK_WIDGET (self->reviews_box));

    /* Load cached reviews */
    if (self->cache != NULL) {
        g_autoptr(JsonNode) reviews_cache = store_cache_lookup_json (self->cache, "reviews", store_app_get_name (app), FALSE, NULL, NULL);
        if (reviews_cache != NULL) {
            g_autoptr(GPtrArray) reviews = g_ptr_array_new_with_free_func (g_object_unref);
            JsonArray *array = json_node_get_array (reviews_cache);
            for (guint i = 0; i < json_array_get_length (array); i++) {
                JsonNode *node = json_array_get_element (array, i);
                g_ptr_array_add (reviews, store_odrs_review_new_from_json (node));
            }
            set_reviews (self, reviews);
        }
    }

    g_autoptr(StoreOdrsClient) odrs_client = store_odrs_client_new ();
    store_odrs_client_get_reviews_async (odrs_client, store_app_get_appstream_id (app), NULL, 0, NULL, reviews_cb, self);
    g_steal_pointer (&odrs_client); // FIXME leaks for testing, remove when async call keeps reference

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
