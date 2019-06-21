/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-app.h"

struct _StoreApp
{
    GObject parent_instance;

    gchar *name;
};

G_DEFINE_TYPE (StoreApp, store_app, G_TYPE_OBJECT)

static void
store_app_dispose (GObject *object)
{
    StoreApp *self = STORE_APP (object);
    g_clear_pointer (&self->name, g_free);
}

static void
store_app_class_init (StoreAppClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_dispose;
}

static void
store_app_init (StoreApp *self)
{
}

StoreApp *
store_app_new (const gchar *name)
{
    StoreApp *self = g_object_new (store_app_get_type (), NULL);
    self->name = g_strdup (name);
    return self;
}

const gchar *
store_app_get_description (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return "Lorem Ipsum\nLorem Ipsum\nLorem Ipsum\nLorem Ipsum...";
}

const gchar *
store_app_get_name (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->name;
}

const gchar *
store_app_get_publisher (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return "Publisher";
}

const gchar *
store_app_get_summary (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return "Lorem Ipsum";
}

const gchar *
store_app_get_title (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->name;
}
