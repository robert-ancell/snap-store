/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-install-button.h"

struct _StoreInstallButton
{
    GtkButton parent_instance;

    GtkLabel *install_label;
    GtkStack *label_stack;
    GtkLabel *remove_label;

    StoreApp *app;
    gulong installed_handler;
};

G_DEFINE_TYPE (StoreInstallButton, store_install_button, GTK_TYPE_BUTTON)

static void
clicked_cb (StoreInstallButton *self)
{
    if (store_app_get_installed (self->app))
        store_app_remove_async (self->app, NULL, NULL, NULL);
    else
        store_app_install_async (self->app, NULL, NULL, NULL, NULL);
}

static void
update_installed (StoreInstallButton *self)
{
    if (store_app_get_installed (self->app))
        gtk_stack_set_visible_child (self->label_stack, GTK_WIDGET (self->remove_label));
    else
        gtk_stack_set_visible_child (self->label_stack, GTK_WIDGET (self->install_label));
}

static void
store_install_button_dispose (GObject *object)
{
    StoreInstallButton *self = STORE_INSTALL_BUTTON (object);

    g_clear_object (&self->app);

    G_OBJECT_CLASS (store_install_button_parent_class)->dispose (object);
}

static void
store_install_button_class_init (StoreInstallButtonClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_install_button_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-install-button.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreInstallButton, install_label);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreInstallButton, label_stack);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreInstallButton, remove_label);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), clicked_cb);
}

static void
store_install_button_init (StoreInstallButton *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_install_button_set_app (StoreInstallButton *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_INSTALL_BUTTON (self));

    if (self->installed_handler != 0) {
        g_signal_handler_disconnect (self->app, self->installed_handler);
        self->installed_handler = 0;
    }

    g_set_object (&self->app, app);

    if (app != NULL)
        self->installed_handler = g_signal_connect_object (app, "notify::installed", G_CALLBACK (update_installed), self, G_CONNECT_SWAPPED);
    update_installed (self);
}
