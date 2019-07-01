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
#include "store-odrs-client.h"
#include "store-snap-pool.h"
#include "store-window.h"

struct _StoreApplication
{
    GtkApplication parent_instance;

    StoreWindow *window;

    StoreCache *cache;
    GtkCssProvider *css_provider;
    StoreOdrsClient *odrs_client;
    StoreSnapPool *snap_pool;
};

G_DEFINE_TYPE (StoreApplication, store_application, GTK_TYPE_APPLICATION)

static void
store_application_dispose (GObject *object)
{
    StoreApplication *self = STORE_APPLICATION (object);

    g_clear_object (&self->cache);
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
        store_app_set_one_star_review_count (STORE_APP (snap), ratings != NULL ? ratings[0] : 0);
        store_app_set_two_star_review_count (STORE_APP (snap), ratings != NULL ? ratings[1] : 0);
        store_app_set_three_star_review_count (STORE_APP (snap), ratings != NULL ? ratings[2] : 0);
        store_app_set_four_star_review_count (STORE_APP (snap), ratings != NULL ? ratings[3] : 0);
        store_app_set_five_star_review_count (STORE_APP (snap), ratings != NULL ? ratings[4] : 0);
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

    store_odrs_client_update_ratings_async (self->odrs_client, NULL, ratings_cb, self);

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
