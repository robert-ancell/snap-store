/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <libsoup/soup.h>

#include "store-application.h"
#include "store-cache.h"
#include "store-category.h"
#include "store-odrs-client.h"
#include "store-snap-pool.h"
#include "store-window.h"

struct _StoreApplication
{
    GtkApplication parent_instance;

    StoreWindow *window;

    StoreCache *cache;
    GCancellable *cancellable;
    GPtrArray *categories;
    GtkCssProvider *css_provider;
    StoreOdrsClient *odrs_client;
    StoreSnapPool *snap_pool;
};

G_DEFINE_TYPE (StoreApplication, store_application, GTK_TYPE_APPLICATION)

typedef struct
{
    StoreApplication *self;
    gchar *section_name;
} FindSectionData;

static FindSectionData *
find_section_data_new (StoreApplication *self, const gchar *section_name)
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

static void
store_application_dispose (GObject *object)
{
    StoreApplication *self = STORE_APPLICATION (object);

    g_clear_object (&self->cache);
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    g_clear_pointer (&self->categories, g_ptr_array_unref);
    g_clear_object (&self->css_provider);
    g_clear_object (&self->odrs_client);
    g_clear_object (&self->snap_pool);

    G_OBJECT_CLASS (store_application_parent_class)->dispose (object);
}

static void
theme_changed_cb (StoreApplication *self)
{
    gtk_css_provider_load_from_resource (self->css_provider, "/io/snapcraft/Store/gtk-style.css");
}

static int
store_application_command_line (GApplication *application, GApplicationCommandLine *command_line)
{
    StoreApplication *self = STORE_APPLICATION (application);

    GVariantDict *options = g_application_command_line_get_options_dict (command_line);

    if (g_variant_dict_contains (options, "no-cache"))
        g_clear_object (&self->cache);

    if (g_variant_dict_contains (options, "version")) {
        g_print ("snap-store " VERSION "\n");
        return 0;
    }

    int args_length;
    g_auto(GStrv) args = g_application_command_line_get_arguments (command_line, &args_length);
    if (args_length >= 2) {
        const gchar *arg = args[1];
        const gchar *name = NULL;

        g_autoptr(SoupURI) uri = soup_uri_new (arg);
        if (SOUP_URI_IS_VALID (uri)) {
            if (g_strcmp0 (soup_uri_get_scheme (uri), "snap") == 0) {
                name = soup_uri_get_host (uri);
            }
            else
                g_warning ("Unsupported URI: %s", arg); // FIXME: Show in GUI
        }
        else {
            // FIXME: Validate characters
            name = arg;
        }

        if (name != NULL) {
            g_autoptr(StoreSnapApp) app = store_snap_pool_get_snap (self->snap_pool, name);
            store_window_show_app (self->window, STORE_APP (app));
        }
    }

    gtk_window_present (GTK_WINDOW (self->window));

    return -1;
}

static void
ratings_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreApplication *self = user_data;

    g_autoptr(GError) error = NULL;
    if (!store_odrs_client_update_ratings_finish (STORE_ODRS_CLIENT (object), result, &error)) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to get ratings: %s", error->message);
        // FIXME: Retry?
        return;
    }

    /* Update existing apps */
    g_autoptr(GPtrArray) snaps = store_snap_pool_get_snaps (self->snap_pool);
    for (guint i = 0; i < snaps->len; i++) {
        StoreSnapApp *snap = g_ptr_array_index (snaps, i);
        gint64 *ratings = store_odrs_client_get_ratings (self->odrs_client, store_app_get_appstream_id (STORE_APP (snap)));
        store_app_set_review_count_one_star (STORE_APP (snap), ratings != NULL ? ratings[0] : 0);
        store_app_set_review_count_two_star (STORE_APP (snap), ratings != NULL ? ratings[1] : 0);
        store_app_set_review_count_three_star (STORE_APP (snap), ratings != NULL ? ratings[2] : 0);
        store_app_set_review_count_four_star (STORE_APP (snap), ratings != NULL ? ratings[3] : 0);
        store_app_set_review_count_five_star (STORE_APP (snap), ratings != NULL ? ratings[4] : 0);
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

static GPtrArray *
load_cached_category_apps (StoreApplication *self, const gchar *section)
{
    g_autoptr(GPtrArray) apps = g_ptr_array_new_with_free_func (g_object_unref);

    g_autoptr(JsonNode) sections_cache = store_cache_lookup_json (self->cache, "sections", section, FALSE, NULL, NULL);
    if (sections_cache == NULL)
        return g_steal_pointer (&apps);

    JsonArray *array = json_node_get_array (sections_cache);
    for (guint j = 0; j < json_array_get_length (array); j++) {
        JsonNode *node = json_array_get_element (array, j);

        const gchar *name = json_node_get_string (node);
        g_autoptr(StoreSnapApp) app = store_snap_pool_get_snap (self->snap_pool, name);
        store_app_set_name (STORE_APP (app), name);
        //FIXMEset_review_counts (self, STORE_APP (app));
        store_app_update_from_cache (STORE_APP (app), self->cache);
        g_ptr_array_add (apps, g_object_ref (app));
    }

    return g_steal_pointer (&apps);
}

static GPtrArray *
load_cached_categories (StoreApplication *self)
{
    g_autoptr(GPtrArray) categories = g_ptr_array_new_with_free_func (g_object_unref);

    if (self->cache == NULL)
        return g_steal_pointer (&categories);

    g_autoptr(JsonNode) sections_cache = store_cache_lookup_json (self->cache, "sections", "_index", FALSE, NULL, NULL);
    if (sections_cache == NULL)
        return g_steal_pointer (&categories);

    JsonArray *array = json_node_get_array (sections_cache);
    g_autoptr(GPtrArray) section_array = g_ptr_array_new ();
    for (guint i = 0; i < json_array_get_length (array); i++) {
        JsonNode *node = json_array_get_element (array, i);
        const gchar *section = json_node_get_string (node);

        StoreCategory *category = store_category_new ();
        g_ptr_array_add (categories, category);
        store_category_set_name (category, section);
        store_category_set_title (category, get_section_title (section));

        g_autoptr(GPtrArray) apps = load_cached_category_apps (self, section);
        store_category_set_apps (category, apps);
    }

    return g_steal_pointer (&categories);
}

StoreCategory *
find_category (StoreApplication *self, const gchar *section_name)
{
    for (guint i = 0; i < self->categories->len; i++) {
        StoreCategory *category = g_ptr_array_index (self->categories, i);
        if (g_strcmp0 (store_category_get_name (category), section_name) == 0)
            return category;
    }

    return NULL;
}

static void
get_category_snaps_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(FindSectionData) data = user_data;
    StoreApplication *self = data->self;

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
        //FIXMEset_review_counts (self, STORE_APP (app));
        if (self->cache != NULL)
            store_app_save_to_cache (STORE_APP (app), self->cache);
        g_ptr_array_add (apps, g_steal_pointer (&app));
    }

    StoreCategory *category = find_category (self, data->section_name);
    if (category != NULL)
        store_category_set_apps (category, apps);

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

static void
get_sections_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    StoreApplication *self = user_data;

    g_autoptr(GError) error = NULL;
    g_auto(GStrv) sections = snapd_client_get_sections_finish (SNAPD_CLIENT (object), result, &error);
    if (sections == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to get sections: %s", error->message);
        return;
    }

    g_clear_pointer (&self->categories, g_ptr_array_unref);
    self->categories = g_ptr_array_new_with_free_func (g_object_unref);
    for (int i = 0; sections[i] != NULL; i++) {
        StoreCategory *category = store_category_new ();
        g_ptr_array_add (self->categories, category);
        store_category_set_name (category, sections[i]);
        store_category_set_title (category, get_section_title (sections[i]));

        g_autoptr(GPtrArray) apps = load_cached_category_apps (self, sections[i]);
        store_category_set_apps (category, apps);

        g_autoptr(SnapdClient) client = snapd_client_new ();
        snapd_client_find_section_async (client, SNAPD_FIND_FLAGS_SCOPE_WIDE, sections[i], NULL, self->cancellable, get_category_snaps_cb, find_section_data_new (self, sections[i]));
    }

    store_window_set_categories (self->window, self->categories);

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
store_application_startup (GApplication *application)
{
    StoreApplication *self = STORE_APPLICATION (application);

    G_APPLICATION_CLASS (store_application_parent_class)->startup (application);

    self->window = store_window_new (self);
    store_window_set_cache (self->window, self->cache);
    store_window_set_odrs_client (self->window, self->odrs_client);
    store_window_set_snap_pool (self->window, self->snap_pool);
    store_window_load (self->window);

    /* Load cached sections */
    self->categories = load_cached_categories (self);
    store_window_set_categories (self->window, self->categories);
    g_autoptr(SnapdClient) client = snapd_client_new ();
    snapd_client_get_sections_async (client, self->cancellable, get_sections_cb, self);

    store_odrs_client_update_ratings_async (self->odrs_client, self->cancellable, ratings_cb, self);

    self->css_provider = gtk_css_provider_new ();
    gtk_style_context_add_provider_for_screen (gdk_screen_get_default (), GTK_STYLE_PROVIDER (self->css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_signal_connect_object (gtk_settings_get_default (), "notify::gtk-theme-name", G_CALLBACK (theme_changed_cb), self, G_CONNECT_SWAPPED);
    theme_changed_cb (self);
}

static void
store_application_activate (GApplication *application)
{
    StoreApplication *self = STORE_APPLICATION (application);
    gtk_window_present (GTK_WINDOW (self->window));
}

static void
store_application_class_init (StoreApplicationClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_application_dispose;
    G_APPLICATION_CLASS (klass)->command_line = store_application_command_line;
    G_APPLICATION_CLASS (klass)->startup = store_application_startup;
    G_APPLICATION_CLASS (klass)->activate = store_application_activate;
}

static void
store_application_init (StoreApplication *self)
{
    const GOptionEntry options[] = {
        { "version", 0, 0, G_OPTION_ARG_NONE, NULL,
           /* Help text for --version command line option */
           _("Show version number"), NULL },
        { "no-cache", 0, 0, G_OPTION_ARG_NONE, NULL,
           /* Help text for --no-cache command line option */
           _("Disable caching"), NULL },
        { NULL }
    };

    g_application_add_main_option_entries (G_APPLICATION (self), options);

    self->cache = store_cache_new ();
    self->cancellable = g_cancellable_new ();
    self->odrs_client = store_odrs_client_new ();
    self->snap_pool = store_snap_pool_new ();
}

StoreApplication *
store_application_new (void)
{
    return g_object_new (store_application_get_type (),
                         "application-id", "io.snapcraft.Store",
                         "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                         NULL);
}
