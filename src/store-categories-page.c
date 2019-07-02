/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <glib/gi18n.h>

#include "store-app.h"
#include "store-categories-page.h"

struct _StoreCategoriesPage
{
    GtkBox parent_instance;

    StoreCache *cache;
};

G_DEFINE_TYPE (StoreCategoriesPage, store_categories_page, GTK_TYPE_BOX)

enum
{
    SIGNAL_APP_ACTIVATED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
store_categories_page_dispose (GObject *object)
{
    StoreCategoriesPage *self = STORE_CATEGORIES_PAGE (object);

    g_clear_object (&self->cache);

    G_OBJECT_CLASS (store_categories_page_parent_class)->dispose (object);
}

static void
store_categories_page_class_init (StoreCategoriesPageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_categories_page_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-categories-page.ui");

    signals[SIGNAL_APP_ACTIVATED] = g_signal_new ("app-activated",
                                                  G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                                                  G_SIGNAL_RUN_LAST,
                                                  0,
                                                  NULL, NULL,
                                                  NULL,
                                                  G_TYPE_NONE,
                                                  1, store_app_get_type ());
}

static void
store_categories_page_init (StoreCategoriesPage *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

void
store_categories_page_set_cache (StoreCategoriesPage *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_CATEGORIES_PAGE (self));
    g_set_object (&self->cache, cache);
    // FIXME: Should apply to children
}
