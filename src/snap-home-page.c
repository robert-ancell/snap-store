/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "snap-home-page.h"

#include "snap-category-view.h"

struct _SnapHomePage
{
    GtkBox parent_instance;

    GtkBox *category_box;
};

G_DEFINE_TYPE (SnapHomePage, snap_home_page, GTK_TYPE_BOX)

static void
snap_home_page_dispose (GObject *object)
{
    SnapHomePage *self = SNAP_HOME_PAGE (object);
}

static void
snap_home_page_class_init (SnapHomePageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = snap_home_page_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/snap-home-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), SnapHomePage, category_box);
}

static void
snap_home_page_init (SnapHomePage *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    SnapCategoryView *view = snap_category_view_new ("featured");
    snap_category_view_set_hero (view, "spotify");
    g_auto(GStrv) featured_apps = g_strsplit ("riot-web;pick-colour-picker;wing7;rider;emacs;toontown;eric-ide;natron;hiri", ";", -1);
    snap_category_view_set_apps (view, featured_apps);
    gtk_widget_show (GTK_WIDGET (view));
    gtk_container_add (GTK_CONTAINER (self->category_box), GTK_WIDGET (view));

    view = snap_category_view_new ("development");
    snap_category_view_set_hero (view, "sublime-text");
    g_auto(GStrv) development_apps = g_strsplit ("pycharm-community;postman;atom;notepad-plus-plus;android-studio;phpstorm;eclipse;pycharm-professional;gitkraken", ";", -1);
    snap_category_view_set_apps (view, development_apps);
    gtk_widget_show (GTK_WIDGET (view));
    gtk_container_add (GTK_CONTAINER (self->category_box), GTK_WIDGET (view));
}

SnapHomePage *
snap_home_page_new (void)
{
    return g_object_new (snap_home_page_get_type (), NULL);
}
