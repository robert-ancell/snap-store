/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <snapd-glib/snapd-glib.h>

#include "store-home-page.h"

#include "store-app.h"
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
app_activated_cb (StoreHomePage *self, StoreApp *app)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, app);
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
                                                  1, store_app_get_type ());
}

static GPtrArray *
make_apps (const gchar *name_list)
{
    g_autoptr(GPtrArray) result = g_ptr_array_new_with_free_func (g_object_unref);
    g_auto(GStrv) names = g_strsplit (name_list, ";", -1);
    for (int i = 0; names[i] != NULL; i++)
        g_ptr_array_add (result, store_app_new (names[i]));
    return g_steal_pointer (&result);
}

static void
store_home_page_init (StoreHomePage *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    g_autoptr(SnapdClient) client = snapd_client_new ();
    g_auto(GStrv) sections = snapd_client_get_sections_sync (client, NULL, NULL); // FIXME async
    for (int i = 0; sections[i] != NULL; i++) {
        StoreCategoryView *view = store_category_view_new (sections[i]);
        g_signal_connect_object (view, "app-activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
        g_autoptr(GPtrArray) snaps = snapd_client_find_section_sync (client, SNAPD_FIND_FLAGS_SCOPE_WIDE, sections[i], NULL, NULL, NULL, NULL); // FIXME async
        if (snaps->len >= 1) {
            SnapdSnap *snap = g_ptr_array_index (snaps, 0);
            g_autoptr(StoreApp) hero = store_app_new (snapd_snap_get_name (snap));
            store_category_view_set_hero (view, hero);
        }
        g_autoptr(GPtrArray) apps = g_ptr_array_new_with_free_func (g_object_unref);
        for (guint j = 1; j < snaps->len && j < 10; j++) {
            SnapdSnap *snap = g_ptr_array_index (snaps, j);
            g_ptr_array_add (apps, store_app_new (snapd_snap_get_name (snap)));
        }
        store_category_view_set_apps (view, apps);
        gtk_widget_show (GTK_WIDGET (view));
        gtk_container_add (GTK_CONTAINER (self->category_box), GTK_WIDGET (view));
    }
}

StoreHomePage *
store_home_page_new (void)
{
    return g_object_new (store_home_page_get_type (), NULL);
}
