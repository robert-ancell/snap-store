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
#include "store-category-home-page.h"
#include "store-category-page.h"
#include "store-home-page.h"
#include "store-installed-page.h"

struct _StoreWindow
{
    GtkApplicationWindow parent_instance;

    StoreAppPage *app_page;
    GtkButton *back_button;
    GtkToggleButton *categories_button;
    StoreCategoryHomePage *category_home_page;
    StoreCategoryPage *category_page;
    GtkToggleButton *home_button;
    StoreHomePage *home_page;
    GtkToggleButton *installed_button;
    StoreInstalledPage *installed_page;
    GtkStack *stack;

    StoreModel *model;
    GList *page_stack;
};

G_DEFINE_TYPE (StoreWindow, store_window, GTK_TYPE_APPLICATION_WINDOW)

static void
app_activated_cb (StoreWindow *self, StoreApp *app)
{
    store_window_show_app (self, app);
}

static void
category_activated_cb (StoreWindow *self, StoreCategory *category)
{
    store_window_show_category (self, category);
}

static void
back_button_clicked_cb (StoreWindow *self)
{
    GtkWidget *page = g_list_nth_data (self->page_stack, 0);
    if (page == NULL)
        page = GTK_WIDGET (self->home_page);
    self->page_stack = g_list_delete_link (self->page_stack, g_list_first (self->page_stack));
    gtk_stack_set_visible_child (self->stack, page);
    gtk_widget_set_visible (GTK_WIDGET (self->back_button), self->page_stack != NULL);
}

static void
page_toggled_cb (StoreWindow *self, GtkToggleButton *button)
{
    if (!gtk_toggle_button_get_active (button))
        return;

    if (button == self->home_button)
        gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->home_page));
    else if (button == self->categories_button)
        gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->category_home_page));
    else if (button == self->installed_button)
        gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->installed_page));
    g_clear_pointer (&self->page_stack, g_list_free);
    gtk_widget_hide (GTK_WIDGET (self->back_button));

    if (button != self->home_button)
        gtk_toggle_button_set_active (self->home_button, FALSE);
    if (button != self->categories_button)
        gtk_toggle_button_set_active (self->categories_button, FALSE);
    if (button != self->installed_button)
        gtk_toggle_button_set_active (self->installed_button, FALSE);
}

static void
store_window_dispose (GObject *object)
{
    StoreWindow *self = STORE_WINDOW (object);

    g_clear_object (&self->model);
    g_clear_pointer (&self->page_stack, g_list_free);

    G_OBJECT_CLASS (store_window_parent_class)->dispose (object);
}

static void
store_window_class_init (StoreWindowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_window_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-window.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, app_page);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, back_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, categories_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, category_home_page);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, category_page);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, home_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, home_page);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, installed_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, installed_page);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreWindow, stack);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), app_activated_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), back_button_clicked_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), category_activated_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), page_toggled_cb);
}

static void
store_window_init (StoreWindow *self)
{
    store_app_page_get_type ();
    store_category_home_page_get_type ();
    store_category_page_get_type ();
    store_home_page_get_type ();
    store_installed_page_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));

    gtk_window_set_default_size (GTK_WINDOW (self), 800, 600); // FIXME: Temp
}

StoreWindow *
store_window_new (StoreApplication *application)
{
    return g_object_new (store_window_get_type (),
                         "application", application,
                         NULL);
}

void
store_window_set_model (StoreWindow *self, StoreModel *model)
{
    g_return_if_fail (STORE_IS_WINDOW (self));

    g_set_object (&self->model, model);

    store_page_set_model (STORE_PAGE (self->app_page), model);
    store_page_set_model (STORE_PAGE (self->category_home_page), model);
    store_page_set_model (STORE_PAGE (self->category_page), model);
    store_page_set_model (STORE_PAGE (self->home_page), model);
    store_page_set_model (STORE_PAGE (self->installed_page), model);
}

void
store_window_load (StoreWindow *self)
{
    g_return_if_fail (STORE_IS_WINDOW (self));

    store_home_page_load (self->home_page);
    store_installed_page_load (self->installed_page);
}

void
store_window_show_app (StoreWindow *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_WINDOW (self));

    self->page_stack = g_list_prepend (self->page_stack, gtk_stack_get_visible_child (self->stack));

    store_app_page_set_app (self->app_page, app);
    gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->app_page)); // FIXME: Buttons
    gtk_widget_show (GTK_WIDGET (self->back_button));
}

void
store_window_show_category (StoreWindow *self, StoreCategory *category)
{
    g_return_if_fail (STORE_IS_WINDOW (self));

    self->page_stack = g_list_prepend (self->page_stack, gtk_stack_get_visible_child (self->stack));

    store_category_page_set_category (self->category_page, category);
    gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->category_page)); // FIXME: Buttons
    gtk_widget_show (GTK_WIDGET (self->back_button));
}
