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

#include "store-application.h"
#include "store-cache.h"
#include "store-snap-pool.h"
#include "store-window.h"

struct _StoreApplication
{
    GtkApplication parent_instance;

    StoreWindow *window;

    StoreCache *cache;
    GtkCssProvider *css_provider;
    StoreSnapPool *snap_pool;
};

G_DEFINE_TYPE (StoreApplication, store_application, GTK_TYPE_APPLICATION)

static void
store_application_dispose (GObject *object)
{
    StoreApplication *self = STORE_APPLICATION (object);

    g_clear_object (&self->cache);
    g_clear_object (&self->css_provider);
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
        g_autoptr(StoreSnapApp) app = store_snap_pool_get_snap (self->snap_pool, args[1]);
        store_window_show_app (self->window, STORE_APP (app));
    }

    gtk_window_present (GTK_WINDOW (self->window));

    return -1;
}

static void
store_application_startup (GApplication *application)
{
    StoreApplication *self = STORE_APPLICATION (application);

    G_APPLICATION_CLASS (store_application_parent_class)->startup (application);

    self->window = store_window_new (self);
    store_window_set_cache (self->window, self->cache);
    store_window_set_snap_pool (self->window, self->snap_pool);
    store_window_load (self->window);

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
