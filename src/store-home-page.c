/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <glib/gi18n.h>
#include <snapd-glib/snapd-glib.h>

#include "store-home-page.h"

#include "store-snap-app.h"
#include "store-cache.h"
#include "store-category-view.h"

struct _StoreHomePage
{
    GtkBox parent_instance;

    GtkBox *category_box;
    StoreCategoryView *installed_view;
    GtkEntry *search_entry;
    StoreCategoryView *search_results_view;

    StoreCache *cache;
    GCancellable *cancellable;
    GCancellable *search_cancellable;
    GSource *search_timeout;
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
search_results_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreHomePage *self = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_find_finish (SNAPD_CLIENT (object), result, NULL, &error);
    if (snaps == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to find snaps: %s", error->message);
        return;
    }

    g_autoptr(GPtrArray) apps = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < snaps->len; i++) {
        SnapdSnap *snap = g_ptr_array_index (snaps, i);
        g_autoptr(StoreSnapApp) app = store_snap_app_new ();
        store_snap_app_update_from_search (app, snap);
        store_app_save_to_cache (STORE_APP (app), self->cache);
        g_ptr_array_add (apps, g_steal_pointer (&app));
    }
    store_category_view_set_apps (self->search_results_view, apps);

    gtk_widget_hide (GTK_WIDGET (self->category_box));
    gtk_widget_show (GTK_WIDGET (self->search_results_view));
}

static void
search_cb (StoreHomePage *self)
{
    const gchar *query = gtk_entry_get_text (self->search_entry);

    if (query[0] == '\0') {
        gtk_widget_show (GTK_WIDGET (self->category_box));
        gtk_widget_hide (GTK_WIDGET (self->search_results_view));
        return;
    }

    g_autoptr(SnapdClient) client = snapd_client_new ();
    g_cancellable_cancel (self->search_cancellable);
    g_clear_object (&self->search_cancellable);
    self->search_cancellable = g_cancellable_new ();
    snapd_client_find_async (client, SNAPD_FIND_FLAGS_SCOPE_WIDE, query, self->search_cancellable, search_results_cb, self);
}

static gboolean
search_timeout_cb (gpointer user_data)
{
    StoreHomePage *self = user_data;

    search_cb (self);

    return G_SOURCE_REMOVE;
}

static void
search_changed_cb (StoreHomePage *self)
{
    if (self->search_timeout)
        g_source_destroy (self->search_timeout);
    g_clear_pointer (&self->search_timeout, g_source_unref);
    self->search_timeout = g_timeout_source_new (200);
    g_source_set_callback (self->search_timeout, search_timeout_cb, self, NULL);
    g_source_attach (self->search_timeout, g_main_context_default ());
}

static void
store_home_page_dispose (GObject *object)
{
    StoreHomePage *self = STORE_HOME_PAGE (object);
    g_clear_object (&self->cache);
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_cancellable_cancel (self->search_cancellable);
    g_clear_object (&self->search_cancellable);
    if (self->search_timeout)
        g_source_destroy (self->search_timeout);
    g_clear_pointer (&self->search_timeout, g_source_unref);

    G_OBJECT_CLASS (store_home_page_parent_class)->dispose (object);
}

static void
store_home_page_class_init (StoreHomePageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_home_page_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-home-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, category_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, search_entry);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, search_results_view);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), app_activated_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), search_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), search_changed_cb);

    signals[SIGNAL_APP_ACTIVATED] = g_signal_new ("app-activated",
                                                  G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                                  G_SIGNAL_RUN_LAST,
                                                  0,
                                                  NULL, NULL,
                                                  NULL,
                                                  G_TYPE_NONE,
                                                  1, store_app_get_type ());
}

typedef struct
{
    StoreHomePage *self;
    gchar *section_name;
} FindSectionData;

static FindSectionData *
find_section_data_new (StoreHomePage *self, const gchar *section_name)
{
    FindSectionData *data = g_new0 (FindSectionData, 1);
    data->self = self;
    data->section_name = g_strdup (section_name);
    return data;
}

static void
find_section_data_free (FindSectionData *data)
{
    g_free (data->section_name);
    g_free (data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (FindSectionData, find_section_data_free)

static StoreCategoryView *
find_store_category_view (StoreHomePage *self, const gchar *section_name)
{
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->category_box));
    for (GList *link = children; link != NULL; link = link->next) {
        GtkWidget *child = link->data;
        if (STORE_IS_CATEGORY_VIEW (child) &&
            g_strcmp0 (store_category_view_get_name (STORE_CATEGORY_VIEW (child)), section_name) == 0)
            return STORE_CATEGORY_VIEW (child);
    }

    return NULL;
}

static void
set_category_apps (StoreHomePage *self, const gchar *section_name, GPtrArray *apps)
{
    StoreCategoryView *view = find_store_category_view (self, section_name);
    if (view == NULL)
        return;

    guint start = 0;
    if (apps->len >= 1) {
        StoreSnapApp *hero = g_ptr_array_index (apps, 0);
        if (store_app_get_icon (STORE_APP (hero)) != NULL) {
            store_category_view_set_hero (view, STORE_APP (hero));
            start = 1;
        }
    }
    g_autoptr(GPtrArray) featured_apps = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = start; i < apps->len && i < start + 9; i++) {
        StoreSnapApp *app = g_ptr_array_index (apps, i);
        g_ptr_array_add (featured_apps, g_object_ref (app));
    }
    store_category_view_set_apps (view, featured_apps);
}

static void
get_category_snaps_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(FindSectionData) data = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_find_section_finish (SNAPD_CLIENT (object), result, NULL, &error);
    if (snaps == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to find snaps in category: %s", error->message);
        return;
    }

    g_autoptr(GPtrArray) apps = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < snaps->len; i++) {
        SnapdSnap *snap = g_ptr_array_index (snaps, i);
        g_autoptr(StoreSnapApp) app = store_snap_app_new ();
        store_snap_app_update_from_search (app, snap);
        store_app_save_to_cache (STORE_APP (app), data->self->cache);
        g_ptr_array_add (apps, g_steal_pointer (&app));
    }
    set_category_apps (data->self, data->section_name, apps);

    /* Save in cache */
    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (guint i = 0; i < snaps->len; i++) {
        SnapdSnap *snap = g_ptr_array_index (snaps, i);
        json_builder_add_string_value (builder, snapd_snap_get_name (snap));
    }
    json_builder_end_array (builder);
    g_autoptr(JsonNode) root = json_builder_get_root (builder);
    store_cache_insert_json (data->self->cache, "sections", data->section_name, FALSE, root);
}

static const gchar *
get_section_title (const gchar *name)
{
    if (strcmp (name, "development") == 0)
        /* Title for Development snap category */
        return _("Development");
    if (strcmp (name, "games") == 0)
        /* Title for Games snap category */
        return _("Games");
    if (strcmp (name, "social") == 0)
        /* Title for Social snap category */
        return _("Social");
    if (strcmp (name, "productivity") == 0)
        /* Title for Productivity snap category */
        return _("Productivity");
    if (strcmp (name, "utilities") == 0)
        /* Title for Utilities snap category */
        return _("Utilities");
    if (strcmp (name, "photo-and-video") == 0)
        /* Title for Photo and Video snap category */
        return _("Photo and Video");
    if (strcmp (name, "server-and-cloud") == 0)
        /* Title for Server and Cloud snap category */
        return _("Server and Cloud");
    if (strcmp (name, "security") == 0)
        /* Title for Security snap category */
        return _("Security");
    if (strcmp (name, "") == 0)
        /* Title for Security snap category */
        return _("Security");
    if (strcmp (name, "featured") == 0)
        /* Title for Featured snap category */
        return _("Featured");
    if (strcmp (name, "devices-and-iot") == 0)
        /* Title for Devices and IoT snap category */
        return _("Devices and IoT");
    if (strcmp (name, "music-and-audio") == 0)
        /* Title for Music and Audio snap category */
        return _("Music and Audio");
    if (strcmp (name, "entertainment") == 0)
        /* Title for Entertainment snap category */
        return _("Entertainment");
    if (strcmp (name, "art-and-design") == 0)
        /* Title for Art and Design snap category */
        return _("Art and Design");
    if (strcmp (name, "finance") == 0)
        /* Title for Finance snap category */
        return _("Finance");
    if (strcmp (name, "news-and-weather") == 0)
        /* Title for News and Weather snap category */
        return _("News and Weather");
    if (strcmp (name, "science") == 0)
        /* Title for Science snap category */
        return _("Science");
    if (strcmp (name, "health-and-fitness") == 0)
        /* Title for Health and Fitness snap category */
        return _("Health and Fitness");
    if (strcmp (name, "education") == 0)
        /* Title for Education snap category */
        return _("Education");
    if (strcmp (name, "books-and-reference") == 0)
        /* Title for Books and Reference snap category */
        return _("Books and Reference");
    if (strcmp (name, "personalisation") == 0)
        /* Title for Personalisation snap category */
        return _("Personalisation");
    return name;
}

static void
set_sections (StoreHomePage *self, GStrv sections, gboolean populate)
{
    /* Add or update existing categories */
    for (int i = 0; sections[i] != NULL; i++) {
        StoreCategoryView *view = find_store_category_view (self, sections[i]); // FIXME: Only look ahead from current position
        if (view != NULL)
            gtk_box_reorder_child (self->category_box, GTK_WIDGET (view), i);
        else {
            view = store_category_view_new ();
            gtk_widget_show (GTK_WIDGET (view));
            store_category_view_set_name (view, sections[i]);
            store_category_view_set_title (view, get_section_title (sections[i]));
            g_signal_connect_object (view, "app-activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
            gtk_container_add_with_properties (GTK_CONTAINER (self->category_box), GTK_WIDGET (view), "position", i, NULL);
        }
        if (populate) { // FIXME: Hack to stop the following occuring for cached values
            g_autoptr(SnapdClient) client = snapd_client_new ();
            snapd_client_find_section_async (client, SNAPD_FIND_FLAGS_SCOPE_WIDE, sections[i], NULL, self->cancellable, get_category_snaps_cb, find_section_data_new (self, sections[i]));
        }
    }

    /* Remove remaining unused categories */
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->category_box));
    for (GList *link = g_list_nth (children, g_strv_length (sections)); link != NULL; link = link->next) {
        GtkWidget *child = link->data;
        if (child != GTK_WIDGET (self->installed_view))
            gtk_container_remove (GTK_CONTAINER (self->category_box), child);
    }
}

static void
get_sections_cb (GObject *object, GAsyncResult *result, gpointer user_data)
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

    set_sections (self, sections, TRUE);

    /* Save in cache */
    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (int i = 0; sections[i] != NULL; i++)
        json_builder_add_string_value (builder, sections[i]);
    json_builder_end_array (builder);
    g_autoptr(JsonNode) root = json_builder_get_root (builder);
    store_cache_insert_json (self->cache, "sections", "_index", FALSE, root);
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
        g_autoptr(StoreSnapApp) app = store_snap_app_new ();
        store_app_set_installed (STORE_APP (app), TRUE);
        store_snap_app_update_from_search (app, snap);
        g_ptr_array_add (apps, g_steal_pointer (&app));
    }
    store_category_view_set_apps (self->installed_view, apps);
    gtk_widget_show (GTK_WIDGET (self->installed_view));
}

static void
store_home_page_init (StoreHomePage *self)
{
    self->cache = store_cache_new ();
    self->cancellable = g_cancellable_new ();

    store_category_view_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));

    self->installed_view = store_category_view_new (); // FIXME: Move into .ui
    store_category_view_set_title (self->installed_view,
                                   /* Title for category showing installed snaps */
                                   _("Installed"));
    g_signal_connect_object (self->installed_view, "app-activated", G_CALLBACK (app_activated_cb), self, G_CONNECT_SWAPPED);
    gtk_box_pack_end (self->category_box, GTK_WIDGET (self->installed_view), FALSE, FALSE, 0);

    g_autoptr(SnapdClient) client = snapd_client_new ();
    snapd_client_get_sections_async (client, self->cancellable, get_sections_cb, self);

    g_autoptr(SnapdClient) client2 = snapd_client_new ();
    snapd_client_get_snaps_async (client2, SNAPD_GET_SNAPS_FLAGS_NONE, NULL, self->cancellable, get_snaps_cb, self);

    /* Load cached sections */
    g_autoptr(JsonNode) sections_cache = store_cache_lookup_json (self->cache, "sections", "_index", FALSE);
    if (sections_cache != NULL) {
        JsonArray *array = json_node_get_array (sections_cache);
        g_autoptr(GPtrArray) section_array = g_ptr_array_new ();
        for (guint i = 0; i < json_array_get_length (array); i++) {
            JsonNode *node = json_array_get_element (array, i);
            g_ptr_array_add (section_array, (gpointer) json_node_get_string (node));
        }
        g_ptr_array_add (section_array, NULL);
        GStrv sections = (GStrv) section_array->pdata;
        set_sections (self, sections, FALSE);

        for (int i = 0; sections[i] != NULL; i++) {
            g_autoptr(JsonNode) sections_cache = store_cache_lookup_json (self->cache, "sections", sections[i], FALSE);
            if (sections_cache != NULL) {
                JsonArray *array = json_node_get_array (sections_cache);
                g_autoptr(GPtrArray) apps = g_ptr_array_new_with_free_func (g_object_unref);
                for (guint j = 0; j < json_array_get_length (array); j++) {
                    JsonNode *node = json_array_get_element (array, j);

                    const gchar *name = json_node_get_string (node);
                    g_autoptr(StoreSnapApp) app = store_snap_app_new ();
                    store_app_set_name (STORE_APP (app), name);
                    store_app_update_from_cache (STORE_APP (app), self->cache);
                    g_ptr_array_add (apps, g_object_ref (app));
                }

                set_category_apps (self, sections[i], apps);
            }
        }
    }
}

StoreHomePage *
store_home_page_new (void)
{
    return g_object_new (store_home_page_get_type (), NULL);
}
