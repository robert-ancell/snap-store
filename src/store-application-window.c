/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-application-window.h"

#include "store-app-page.h"
#include "store-home-page.h"

struct _StoreApplicationWindow
{
    GtkApplicationWindow parent_instance;

    StoreAppPage *app_page;
    StoreHomePage *home_page;
    GtkStack *stack;
};

G_DEFINE_TYPE (StoreApplicationWindow, store_application_window, GTK_TYPE_APPLICATION_WINDOW)

static void
store_application_window_dispose (GObject *object)
{
    StoreApplicationWindow *self = STORE_APPLICATION_WINDOW (object);
}

static void
store_application_window_class_init (StoreApplicationWindowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_application_window_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/store-application-window.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreApplicationWindow, app_page);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreApplicationWindow, home_page);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreApplicationWindow, stack);
}

static void
store_application_window_init (StoreApplicationWindow *self)
{
    store_app_page_get_type ();
    store_home_page_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

StoreApplicationWindow *
store_application_window_new (StoreApplication *application)
{
    return g_object_new (store_application_window_get_type (),
                         "application", application,
                         NULL);
}
