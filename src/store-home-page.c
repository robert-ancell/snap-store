/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-home-page.h"

#include "store-category-view.h"

struct _StoreHomePage
{
    GtkBox parent_instance;

    GtkBox *category_box;
};

G_DEFINE_TYPE (StoreHomePage, store_home_page, GTK_TYPE_BOX)

enum
{
    SIGNAL_APP_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
app_activated_cb (StoreHomePage *self, const gchar *name)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, name);
}

static void
store_home_page_dispose (GObject *object)
{
    StoreHomePage *self = STORE_HOME_PAGE (object);
}

static void
store_home_page_class_init (StoreHomePageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_home_page_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/store-home-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, category_box);

    signals[SIGNAL_APP_ACTIVATED] = g_signal_new ("app-activated",
                                                  G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                                  G_SIGNAL_RUN_LAST,
                                                  0,
                                                  NULL, NULL,
                                                  NULL,
                                                  G_TYPE_NONE,
                                                  1, G_TYPE_STRING);
}

static void
store_home_page_init (StoreHomePage *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    StoreCategoryView *view = store_category_view_new ("featured");
    g_signal_connect_object (view, "app-activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
    store_category_view_set_hero (view, "spotify");
    g_auto(GStrv) featured_apps = g_strsplit ("riot-web;pick-colour-picker;wing7;rider;emacs;toontown;eric-ide;natron;hiri", ";", -1);
    store_category_view_set_apps (view, featured_apps);
    gtk_widget_show (GTK_WIDGET (view));
    gtk_container_add (GTK_CONTAINER (self->category_box), GTK_WIDGET (view));

    view = store_category_view_new ("development");
    g_signal_connect_object (view, "app-activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
    store_category_view_set_hero (view, "sublime-text");
    g_auto(GStrv) development_apps = g_strsplit ("pycharm-community;postman;atom;notepad-plus-plus;android-studio;phpstorm;eclipse;pycharm-professional;gitkraken", ";", -1);
    store_category_view_set_apps (view, development_apps);
    gtk_widget_show (GTK_WIDGET (view));
    gtk_container_add (GTK_CONTAINER (self->category_box), GTK_WIDGET (view));
}

StoreHomePage *
store_home_page_new (void)
{
    return g_object_new (store_home_page_get_type (), NULL);
}
