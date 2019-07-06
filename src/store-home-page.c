/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <glib/gi18n.h>

#include "store-home-page.h"

#include "store-app-grid.h"
#include "store-banner-tile.h"
#include "store-category-list.h"

struct _StoreHomePage
{
    StorePage parent_instance;

    StoreBannerTile *banner_tile;
    StoreBannerTile *banner1_tile;
    StoreBannerTile *banner2_tile;
    GtkBox *category_box;
    StoreCategoryList *category_list1;
    StoreCategoryList *category_list2;
    StoreCategoryList *category_list3;
    StoreCategoryList *category_list4;
    StoreAppGrid *editors_picks_grid;
    GtkEntry *search_entry;
    StoreAppGrid *search_results_grid;
    GtkBox *small_banner_box;

    StoreCategory *featured_category;
    GCancellable *search_cancellable;
    GSource *search_timeout;
};

enum
{
    PROP_0,
    PROP_CATEGORIES,
    PROP_LAST
};

G_DEFINE_TYPE (StoreHomePage, store_home_page, store_page_get_type ())

enum
{
    SIGNAL_APP_ACTIVATED,
    SIGNAL_CATEGORY_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
banner_tile_activated_cb (StoreHomePage *self, StoreBannerTile *banner_tile)
{
    g_signal_emit (self, signals[SIGNAL_APP_ACTIVATED], 0, store_banner_tile_get_app (banner_tile));
}

static void
category_list_activated_cb (StoreHomePage *self, StoreCategoryList *category_list)
{
    g_signal_emit (self, signals[SIGNAL_CATEGORY_ACTIVATED], 0, store_category_list_get_category (category_list));
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
    g_autoptr(GPtrArray) apps = store_model_search_finish (STORE_MODEL (object), result, &error);
    if (apps == NULL) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            return;
        g_warning ("Failed to find snaps: %s", error->message);
        return;
    }

    store_app_grid_set_apps (self->search_results_grid, apps);

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

    g_cancellable_cancel (self->search_cancellable);
    g_clear_object (&self->search_cancellable);
    self->search_cancellable = g_cancellable_new ();
    store_model_search_async (store_page_get_model (STORE_PAGE (self)), query, self->search_cancellable, search_results_cb, self);
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
see_more_editors_picks_cb (StoreHomePage *self)
{
    g_signal_emit (self, signals[SIGNAL_CATEGORY_ACTIVATED], 0, self->featured_category);
}

static void
store_home_page_dispose (GObject *object)
{
    StoreHomePage *self = STORE_HOME_PAGE (object);

    g_cancellable_cancel (self->search_cancellable);
    g_clear_object (&self->search_cancellable);
    g_clear_object (&self->featured_category);
    if (self->search_timeout)
        g_source_destroy (self->search_timeout);
    g_clear_pointer (&self->search_timeout, g_source_unref);

    G_OBJECT_CLASS (store_home_page_parent_class)->dispose (object);
}

static void
store_home_page_get_property (GObject *object, guint prop_id, GValue *value G_GNUC_UNUSED, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
store_home_page_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreHomePage *self = STORE_HOME_PAGE (object);

    switch (prop_id)
    {
    case PROP_CATEGORIES:
        store_home_page_set_categories (self, g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_home_page_set_model (StorePage *page, StoreModel *model)
{
    StoreHomePage *self = STORE_HOME_PAGE (page);

    store_app_grid_set_model (self->editors_picks_grid, model);
    store_app_grid_set_model (self->search_results_grid, model);
    store_banner_tile_set_model (self->banner_tile, model);
    store_banner_tile_set_model (self->banner1_tile, model);
    store_banner_tile_set_model (self->banner2_tile, model);
    store_category_list_set_model (self->category_list1, model);
    store_category_list_set_model (self->category_list2, model);
    store_category_list_set_model (self->category_list3, model);
    store_category_list_set_model (self->category_list4, model);

    g_object_bind_property (model, "categories", self, "categories", G_BINDING_SYNC_CREATE);

    STORE_PAGE_CLASS (store_home_page_parent_class)->set_model (page, model);
}

static void
store_home_page_class_init (StoreHomePageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_home_page_dispose;
    G_OBJECT_CLASS (klass)->get_property = store_home_page_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_home_page_set_property;
    STORE_PAGE_CLASS (klass)->set_model = store_home_page_set_model;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_CATEGORIES,
                                     g_param_spec_boxed ("categories", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_WRITABLE));

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
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), banner_tile_activated_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), category_list_activated_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), search_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), search_changed_cb);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), see_more_editors_picks_cb);

    signals[SIGNAL_APP_ACTIVATED] = g_signal_new ("app-activated",
                                                  G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                                  G_SIGNAL_RUN_LAST,
                                                  0,
                                                  NULL, NULL,
                                                  NULL,
                                                  G_TYPE_NONE,
                                                  1, store_app_get_type ());

    signals[SIGNAL_CATEGORY_ACTIVATED] = g_signal_new ("category-activated",
                                                       G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                                       G_SIGNAL_RUN_LAST,
                                                       0,
                                                       NULL, NULL,
                                                       NULL,
                                                       G_TYPE_NONE,
                                                       1, store_category_get_type ());
}

static void
store_home_page_init (StoreHomePage *self)
{
    store_app_grid_get_type ();
    store_banner_tile_get_type ();
    store_category_list_get_type ();
    store_page_get_type ();
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_home_page_set_categories (StoreHomePage *self, GPtrArray *categories)
{
    g_return_if_fail (STORE_IS_HOME_PAGE (self));

    StoreCategoryList *category_lists[] = { self->category_list1, self->category_list2, self->category_list3, self->category_list4 };

    guint n = 0;
    g_clear_object (&self->featured_category);
    for (guint i = 0; i < categories->len; i++) {
        StoreCategory *category = g_ptr_array_index (categories, i);

        if (g_strcmp0 (store_category_get_name (category), "featured") == 0) {
            g_set_object (&self->featured_category, category);
            g_autoptr(GPtrArray) featured_apps = g_ptr_array_new_with_free_func (g_object_unref);
            GPtrArray *apps = store_category_get_apps (category);
            for (guint i = 0; i < apps->len && i < 6; i++) {
                StoreSnapApp *app = g_ptr_array_index (apps, i);
                g_ptr_array_add (featured_apps, g_object_ref (app));
            }
            store_app_grid_set_apps (self->editors_picks_grid, featured_apps);
            continue;
        }

        if (n < 4) {
            gtk_widget_show (GTK_WIDGET (category_lists[n]));
            store_category_list_set_category (category_lists[n], category);
            n++;
        }
    }
    for (; n < 4; n++)
        gtk_widget_hide (GTK_WIDGET (category_lists[n]));
}

void
store_home_page_load (StoreHomePage *self)
{
    store_model_update_categories_async (store_page_get_model (STORE_PAGE (self)), NULL, NULL, NULL);

    // FIXME: Hardcoded
    g_autoptr(StoreSnapApp) app = store_model_get_snap (store_page_get_model (STORE_PAGE (self)), "telemetrytv");
    store_banner_tile_set_app (self->banner_tile, STORE_APP (app));
    g_autoptr(StoreSnapApp) app1 = store_model_get_snap (store_page_get_model (STORE_PAGE (self)), "supertuxkart");
    store_banner_tile_set_app (self->banner1_tile, STORE_APP (app1));
    g_autoptr(StoreSnapApp) app2 = store_model_get_snap (store_page_get_model (STORE_PAGE (self)), "fluffychat");
    store_banner_tile_set_app (self->banner2_tile, STORE_APP (app2));
}
