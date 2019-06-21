/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-app-page.h"

#include "store-category-view.h"

struct _StoreAppPage
{
    GtkBox parent_instance;

    GtkLabel *description_label;
    GtkLabel *details_title_label;
    GtkImage *icon_image;
    GtkButton *install_button;
    GtkLabel *publisher_label;
    GtkBox *screenshots_box;
    GtkLabel *summary_label;
    GtkLabel *title_label;

    StoreApp *app;
};

G_DEFINE_TYPE (StoreAppPage, store_app_page, GTK_TYPE_BOX)

static void
store_app_page_dispose (GObject *object)
{
    StoreAppPage *self = STORE_APP_PAGE (object);
    g_clear_object (&self->app);
}

static void
store_app_page_class_init (StoreAppPageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_page_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/store-app-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, description_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, details_title_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, icon_image);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, install_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, publisher_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, screenshots_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, summary_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreAppPage, title_label);
}

static void
store_app_page_init (StoreAppPage *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreAppPage *
store_app_page_new (void)
{
    return g_object_new (store_app_page_get_type (), NULL);
}

void
store_app_page_set_app (StoreAppPage *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_APP_PAGE (self));

    if (self->app == app)
        return;

    g_clear_object (&self->app);
    if (app != NULL)
        self->app = g_object_ref (app);

    gtk_label_set_label (self->title_label, store_app_get_title (app));
    gtk_label_set_label (self->publisher_label, store_app_get_publisher (app));
    gtk_label_set_label (self->summary_label, store_app_get_summary (app));
    gtk_label_set_label (self->description_label, store_app_get_description (app));
    g_autofree gchar *details_title = g_strdup_printf ("Details for %s", store_app_get_title (app)); // FIXME: translatable
    gtk_label_set_label (self->details_title_label, details_title);
    gtk_image_set_from_resource (self->icon_image, "/com/ubuntu/SnapStore/default-snap-icon.svg");
}

StoreApp *
store_app_page_get_app (StoreAppPage *self)
{
    g_return_val_if_fail (STORE_IS_APP_PAGE (self), NULL);
    return self->app;
}
