/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-window.h"

#include "store-app-page.h"
#include "store-home-page.h"

struct _StoreWindow
{
    GtkApplicationWindow parent_instance;

    StoreAppPage *app_page;
    GtkButton *back_button;
    StoreHomePage *home_page;
    GtkStack *stack;
};

G_DEFINE_TYPE (StoreWindow, store_window, GTK_TYPE_APPLICATION_WINDOW)

static void
app_activated_cb (StoreWindow *self, StoreApp *app)
{
    store_app_page_set_app (self->app_page, app);
    gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->app_page));
    gtk_widget_show (GTK_WIDGET (self->back_button));
}

static void
back_button_clicked_cb (StoreWindow *self)
{
    gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->home_page));
    gtk_widget_hide (GTK_WIDGET (self->back_button));
}

static void
store_window_dispose (GObject *object)
{
    StoreWindow *self = STORE_WINDOW (object);
}

static void
store_window_class_init (StoreWindowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_window_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/store-window.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, app_page);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, back_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, home_page);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, stack);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), app_activated_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), back_button_clicked_cb);
}

static void
store_window_init (StoreWindow *self)
{
    store_app_page_get_type ();
    store_home_page_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreWindow *
store_window_new (StoreApplication *application)
{
    return g_object_new (store_window_get_type (),
                         "application", application,
                         NULL);
}
