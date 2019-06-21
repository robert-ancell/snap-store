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
    StoreCategoryView *installed_view;

    GCancellable *cancellable;
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
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
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

static StoreApp *
snap_to_app (SnapdSnap *snap)
{
    g_autoptr(StoreApp) app = store_app_new (snapd_snap_get_name (snap));
    if (snapd_snap_get_title (snap) != NULL)
        store_app_set_title (app, snapd_snap_get_title (snap));
    else
        store_app_set_title (app, snapd_snap_get_name (snap));
    if (snapd_snap_get_publisher_display_name (snap) != NULL)
        store_app_set_publisher (app, snapd_snap_get_publisher_display_name (snap));
    else
        store_app_set_publisher (app, snapd_snap_get_publisher_username (snap));
    store_app_set_summary (app, snapd_snap_get_summary (snap));
    store_app_set_description (app, snapd_snap_get_description (snap));

    GPtrArray *media = snapd_snap_get_media (snap);
    for (guint i = 0; i < media->len; i++) {
        SnapdMedia *m = g_ptr_array_index (media, i);
        if (g_strcmp0 (snapd_media_get_media_type (m), "icon") == 0) {
            store_app_set_icon (app, snapd_media_get_url (m));
            break;
        }
    }

    g_autofree gchar *appstream_id = g_strdup_printf ("io.snapcraft.%s-%s", snapd_snap_get_name (snap), snapd_snap_get_id (snap));
    store_app_set_appstream_id (app, appstream_id);

    return g_steal_pointer (&app);
}

static void
get_category_snaps_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreCategoryView *view = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_find_section_finish (SNAPD_CLIENT (object), result, NULL, &error);
    if (snaps == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to find snaps in category: %s", error->message);
        return;
    }

    guint start = 0;
    if (snaps->len >= 1) {
        SnapdSnap *snap = g_ptr_array_index (snaps, 0);
        g_autoptr(StoreApp) hero = snap_to_app (snap);
        if (store_app_get_icon (hero) != NULL && g_strcmp0 (store_app_get_icon (hero), "") != 0) {
            store_category_view_set_hero (view, hero);
            start = 1;
        }
    }
    g_autoptr(GPtrArray) apps = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = start; i < snaps->len && i < start + 9; i++) {
        SnapdSnap *snap = g_ptr_array_index (snaps, i);
        g_ptr_array_add (apps, snap_to_app (snap));
    }
    store_category_view_set_apps (view, apps);
}

static const gchar *
get_section_title (const gchar *name)
{
    // FIXME: translatable
    if (strcmp (name, "development") == 0)
        return "Development";
    if (strcmp (name, "games") == 0)
        return "Games";
    if (strcmp (name, "social") == 0)
        return "Social";
    if (strcmp (name, "productivity") == 0)
        return "Productivity";
    if (strcmp (name, "utilities") == 0)
        return "Utilities";
    if (strcmp (name, "photo-and-video") == 0)
        return "Photo and Video";
    if (strcmp (name, "server-and-cloud") == 0)
        return "Server and Cloud";
    if (strcmp (name, "security") == 0)
        return "Security";
    if (strcmp (name, "") == 0)
        return "Security";
    if (strcmp (name, "featured") == 0)
        return "Featured";
    if (strcmp (name, "devices-and-iot") == 0)
        return "Devices and IoT";
    if (strcmp (name, "music-and-audio") == 0)
        return "Music and Audio";
    if (strcmp (name, "entertainment") == 0)
        return "Entertainment";
    if (strcmp (name, "art-and-design") == 0)
        return "Art and Design";
    if (strcmp (name, "finance") == 0)
        return "Finance";
    if (strcmp (name, "news-and-weather") == 0)
        return "News and Weather";
    if (strcmp (name, "science") == 0)
        return "Science";
    if (strcmp (name, "health-and-fitness") == 0)
        return "Health and Fitness";
    if (strcmp (name, "education") == 0)
        return "Education";
    if (strcmp (name, "books-and-reference") == 0)
        return "Books and Reference";
    return name;
}

static void
get_categories_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreHomePage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_auto(GStrv) sections = snapd_client_get_sections_finish (SNAPD_CLIENT (object), result, &error);
    if (sections == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to get sections: %s", error->message);
        return;
    }

    for (int i = 0; sections[i] != NULL; i++) {
        StoreCategoryView *view = store_category_view_new (get_section_title (sections[i]));
        g_signal_connect_object (view, "app-activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
        g_autoptr(SnapdClient) client = snapd_client_new ();
        snapd_client_find_section_async (client, SNAPD_FIND_FLAGS_SCOPE_WIDE, sections[i], NULL, self->cancellable, get_category_snaps_cb, view);
        gtk_widget_show (GTK_WIDGET (view));
        gtk_container_add (GTK_CONTAINER (self->category_box), GTK_WIDGET (view));
    }
}

static void
get_snaps_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreHomePage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_get_snaps_finish (SNAPD_CLIENT (object), result, &error);
    if (snaps == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to get installed snaps: %s", error->message);
        return;
    }

    g_autoptr(GPtrArray) apps = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < snaps->len; i++) {
        SnapdSnap *snap = g_ptr_array_index (snaps, i);
        g_ptr_array_add (apps, snap_to_app (snap));
    }
    store_category_view_set_apps (self->installed_view, apps);
    gtk_widget_show (GTK_WIDGET (self->installed_view));
}

static void
store_home_page_init (StoreHomePage *self)
{
    self->cancellable = g_cancellable_new ();

    gtk_widget_init_template (GTK_WIDGET (self));

    self->installed_view = store_category_view_new ("Installed"); // FIXME: translatable
    g_signal_connect_object (self->installed_view, "app-activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
    gtk_box_pack_end (self->category_box, GTK_WIDGET (self->installed_view), FALSE, FALSE, 0);

    g_autoptr(SnapdClient) client = snapd_client_new ();
    snapd_client_get_sections_async (client, self->cancellable, get_categories_cb, self);

    g_autoptr(SnapdClient) client2 = snapd_client_new ();
    snapd_client_get_snaps_async (client2, SNAPD_GET_SNAPS_FLAGS_NONE, NULL, self->cancellable, get_snaps_cb, self);
}

StoreHomePage *
store_home_page_new (void)
{
    return g_object_new (store_home_page_get_type (), NULL);
}
