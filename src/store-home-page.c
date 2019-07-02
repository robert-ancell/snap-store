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

#include "store-banner-tile.h"
#include "store-category-grid.h"
#include "store-category-list.h"

struct _StoreHomePage
{
    GtkBox parent_instance;

    StoreBannerTile *banner_tile;
    StoreBannerTile *banner1_tile;
    StoreBannerTile *banner2_tile;
    GtkBox *category_box;
    StoreCategoryList *category_list1;
    StoreCategoryList *category_list2;
    StoreCategoryList *category_list3;
    StoreCategoryList *category_list4;
    StoreCategoryGrid *editors_picks_grid;
    GtkEntry *search_entry;
    StoreCategoryGrid *search_results_grid;
    GtkBox *small_banner_box;

    StoreCache *cache;
    GCancellable *cancellable;
    StoreOdrsClient *odrs_client;
    GCancellable *search_cancellable;
    GSource *search_timeout;
    StoreSnapPool *snap_pool;
};

G_DEFINE_TYPE (StoreHomePage, store_home_page, GTK_TYPE_BOX)

enum
{
    SIGNAL_APP_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
set_review_counts (StoreHomePage *self, StoreApp *app)
{
    if (self->odrs_client == NULL)
        return;

    gint64 *ratings = NULL;
    if (store_app_get_appstream_id (app) != NULL)
        ratings = store_odrs_client_get_ratings (self->odrs_client, store_app_get_appstream_id (app));

    store_app_set_review_count_one_star (app, ratings != NULL ? ratings[0] : 0);
    store_app_set_review_count_two_star (app, ratings != NULL ? ratings[1] : 0);
    store_app_set_review_count_three_star (app, ratings != NULL ? ratings[2] : 0);
    store_app_set_review_count_four_star (app, ratings != NULL ? ratings[3] : 0);
    store_app_set_review_count_five_star (app, ratings != NULL ? ratings[4] : 0);
}

static void
banner_activated_cb (StoreHomePage *self)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, store_banner_tile_get_app (self->banner_tile));
}

static void
banner1_activated_cb (StoreHomePage *self)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, store_banner_tile_get_app (self->banner1_tile));
}

static void
banner2_activated_cb (StoreHomePage *self)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, store_banner_tile_get_app (self->banner2_tile));
}

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
        g_autoptr(StoreSnapApp) app = store_snap_pool_get_snap (self->snap_pool, snapd_snap_get_name (snap));
        store_snap_app_update_from_search (app, snap);
        set_review_counts (self, STORE_APP (app));
        if (self->cache != NULL)
            store_app_save_to_cache (STORE_APP (app), self->cache);
        g_ptr_array_add (apps, g_steal_pointer (&app));
    }
    store_category_grid_set_apps (self->search_results_grid, apps);

    gtk_widget_hide (GTK_WIDGET (self->category_box));
    gtk_widget_hide (GTK_WIDGET (self->editors_picks_grid));
    gtk_widget_show (GTK_WIDGET (self->search_results_grid));
    gtk_widget_hide (GTK_WIDGET (self->small_banner_box));
}

static void
search_cb (StoreHomePage *self)
{
    const gchar *query = gtk_entry_get_text (self->search_entry);

    if (query[0] == '\0') {
        gtk_widget_show (GTK_WIDGET (self->category_box));
        gtk_widget_show (GTK_WIDGET (self->editors_picks_grid));
        gtk_widget_hide (GTK_WIDGET (self->search_results_grid));
        gtk_widget_show (GTK_WIDGET (self->small_banner_box));
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
    g_clear_object (&self->odrs_client);
    g_cancellable_cancel (self->search_cancellable);
    g_clear_object (&self->search_cancellable);
    if (self->search_timeout)
        g_source_destroy (self->search_timeout);
    g_clear_pointer (&self->search_timeout, g_source_unref);
    g_clear_object (&self->snap_pool);

    G_OBJECT_CLASS (store_home_page_parent_class)->dispose (object);
}

static void
store_home_page_class_init (StoreHomePageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_home_page_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-home-page.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, banner_tile);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, banner1_tile);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, banner2_tile);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, category_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, category_list1);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, category_list2);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, category_list3);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, category_list4);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, editors_picks_grid);   
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, search_entry);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, search_results_grid);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreHomePage, small_banner_box);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), app_activated_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), banner_activated_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), banner1_activated_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), banner2_activated_cb);
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

static StoreCategoryList *
find_store_category_list (StoreHomePage *self, const gchar *section_name)
{
    g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->category_box));
    for (GList *link = children; link != NULL; link = link->next) {
        GtkWidget *child = link->data;
        if (STORE_IS_CATEGORY_LIST (child) &&
            g_strcmp0 (store_category_list_get_name (STORE_CATEGORY_LIST (child)), section_name) == 0)
            return STORE_CATEGORY_LIST (child);
    }

    return NULL;
}

static void
set_category_apps (StoreHomePage *self, const gchar *section_name, GPtrArray *apps)
{
    if (g_strcmp0 (section_name, "featured") == 0) {
        g_autoptr(GPtrArray) featured_apps = g_ptr_array_new_with_free_func (g_object_unref);
        for (guint i = 0; i < apps->len && i < 6; i++) {
            StoreSnapApp *app = g_ptr_array_index (apps, i);
            g_ptr_array_add (featured_apps, g_object_ref (app));
        }
        store_category_grid_set_apps (self->editors_picks_grid, featured_apps);
        return;
    }

    StoreCategoryList *list = find_store_category_list (self, section_name);
    if (list == NULL)
        return;

    g_autoptr(GPtrArray) featured_apps = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < apps->len && i < 5; i++) {
        StoreSnapApp *app = g_ptr_array_index (apps, i);
        g_ptr_array_add (featured_apps, g_object_ref (app));
    }
    store_category_list_set_apps (list, featured_apps);
}

static void
get_category_snaps_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(FindSectionData) data = user_data;
    StoreHomePage *self = data->self;

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
        g_autoptr(StoreSnapApp) app = store_snap_pool_get_snap (self->snap_pool, snapd_snap_get_name (snap));
        store_snap_app_update_from_search (app, snap);
        set_review_counts (self, STORE_APP (app));
        if (self->cache != NULL)
            store_app_save_to_cache (STORE_APP (app), self->cache);
        g_ptr_array_add (apps, g_steal_pointer (&app));
    }
    set_category_apps (self, data->section_name, apps);

    /* Save in cache */
    if (self->cache != NULL) {
        g_autoptr(JsonBuilder) builder = json_builder_new ();
        json_builder_begin_array (builder);
        for (guint i = 0; i < snaps->len; i++) {
            SnapdSnap *snap = g_ptr_array_index (snaps, i);
            json_builder_add_string_value (builder, snapd_snap_get_name (snap));
        }
        json_builder_end_array (builder);
        g_autoptr(JsonNode) root = json_builder_get_root (builder);
        store_cache_insert_json (self->cache, "sections", data->section_name, FALSE, root, NULL, NULL);
    }
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
    int n = 0;
    for (int i = 0; sections[i] != NULL && n < 4; i++) {
        if (g_strcmp0 (sections[i], "featured") == 0)
            continue;

        g_autoptr(GList) children = gtk_container_get_children (GTK_CONTAINER (self->category_box));
        StoreCategoryList *list = g_list_nth_data (children, n);
        store_category_list_set_name (list, sections[i]);
        store_category_list_set_title (list, get_section_title (sections[i]));
        if (populate) { // FIXME: Hack to stop the following occuring for cached values
            g_autoptr(SnapdClient) client = snapd_client_new ();
            snapd_client_find_section_async (client, SNAPD_FIND_FLAGS_SCOPE_WIDE, sections[i], NULL, self->cancellable, get_category_snaps_cb, find_section_data_new (self, sections[i]));
        }
        n++;
    }

    // FIXME: Hide if not enough
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
    if (self->cache != NULL) {
        g_autoptr(JsonBuilder) builder = json_builder_new ();
        json_builder_begin_array (builder);
        for (int i = 0; sections[i] != NULL; i++)
            json_builder_add_string_value (builder, sections[i]);
        json_builder_end_array (builder);
        g_autoptr(JsonNode) root = json_builder_get_root (builder);
        store_cache_insert_json (self->cache, "sections", "_index", FALSE, root, NULL, NULL);
    }
}

static void
store_home_page_init (StoreHomePage *self)
{
    self->cancellable = g_cancellable_new ();

    store_banner_tile_get_type ();
    store_category_list_get_type ();
    store_category_grid_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));

    store_category_grid_set_title (self->editors_picks_grid,
                                   /* Title for category editors picks */
                                   _("Editors picks")); // FIXME: Make a property in .ui
}

void
store_home_page_set_cache (StoreHomePage *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_HOME_PAGE (self));
    g_set_object (&self->cache, cache);
    store_banner_tile_set_cache (self->banner_tile, cache);
    store_banner_tile_set_cache (self->banner1_tile, cache);
    store_banner_tile_set_cache (self->banner2_tile, cache);
    store_category_list_set_cache (self->category_list1, cache);
    store_category_list_set_cache (self->category_list2, cache);
    store_category_list_set_cache (self->category_list3, cache);
    store_category_list_set_cache (self->category_list4, cache);
    store_category_grid_set_cache (self->editors_picks_grid, cache);
    store_category_grid_set_cache (self->search_results_grid, cache);
}

void
store_home_page_set_odrs_client (StoreHomePage *self, StoreOdrsClient *odrs_client)
{
    g_return_if_fail (STORE_IS_HOME_PAGE (self));
    g_set_object (&self->odrs_client, odrs_client);
}

void
store_home_page_set_snap_pool (StoreHomePage *self, StoreSnapPool *pool)
{
    g_return_if_fail (STORE_IS_HOME_PAGE (self));
    g_set_object (&self->snap_pool, pool);
}

void
store_home_page_load (StoreHomePage *self)
{
    g_autoptr(SnapdClient) client = snapd_client_new ();
    snapd_client_get_sections_async (client, self->cancellable, get_sections_cb, self);

    // FIXME: Hardcoded
    g_autoptr(StoreSnapApp) app = store_snap_pool_get_snap (self->snap_pool, "telemetrytv");
    store_app_update_from_cache (STORE_APP (app), self->cache);
    store_banner_tile_set_app (self->banner_tile, STORE_APP (app));
    g_autoptr(StoreSnapApp) app1 = store_snap_pool_get_snap (self->snap_pool, "supertuxkart");
    store_app_update_from_cache (STORE_APP (app1), self->cache);
    store_banner_tile_set_app (self->banner1_tile, STORE_APP (app1));
    g_autoptr(StoreSnapApp) app2 = store_snap_pool_get_snap (self->snap_pool, "fluffychat");
    store_app_update_from_cache (STORE_APP (app2), self->cache);
    store_banner_tile_set_app (self->banner2_tile, STORE_APP (app2));

    /* Load cached sections */
    if (self->cache != NULL) {
        g_autoptr(JsonNode) sections_cache = store_cache_lookup_json (self->cache, "sections", "_index", FALSE, NULL, NULL);
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
                g_autoptr(JsonNode) sections_cache = store_cache_lookup_json (self->cache, "sections", sections[i], FALSE, NULL, NULL);
                if (sections_cache != NULL) {
                    JsonArray *array = json_node_get_array (sections_cache);
                    g_autoptr(GPtrArray) apps = g_ptr_array_new_with_free_func (g_object_unref);
                    for (guint j = 0; j < json_array_get_length (array); j++) {
                        JsonNode *node = json_array_get_element (array, j);

                        const gchar *name = json_node_get_string (node);
                        g_autoptr(StoreSnapApp) app = store_snap_pool_get_snap (self->snap_pool, name);
                        store_app_set_name (STORE_APP (app), name);
                        set_review_counts (self, STORE_APP (app));
                        store_app_update_from_cache (STORE_APP (app), self->cache);
                        g_ptr_array_add (apps, g_object_ref (app));
                    }

                    set_category_apps (self, sections[i], apps);
                }
            }
        }
    }
}
