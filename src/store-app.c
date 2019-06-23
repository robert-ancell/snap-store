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

    gchar *appstream_id;
    gchar *description;
    StoreMedia *icon;
    gchar *name;
    gchar *publisher;
    gchar *summary;
    gchar *title;
};

G_DEFINE_TYPE (StoreApp, store_app, G_TYPE_OBJECT)

static void
store_app_dispose (GObject *object)
{
    StoreApp *self = STORE_APP (object);
    g_clear_pointer (&self->appstream_id, g_free);
    g_clear_pointer (&self->description, g_free);
    g_clear_object (&self->icon);
    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->publisher, g_free);
    g_clear_pointer (&self->summary, g_free);
    g_clear_pointer (&self->title, g_free);
}

static void
store_app_class_init (StoreAppClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_dispose;
}

static void
store_app_init (StoreApp *self)
{
    self->appstream_id = g_strdup ("");
    self->description = g_strdup ("");
    self->publisher = g_strdup ("");
    self->summary = g_strdup ("");
    self->title = g_strdup ("");
}

StoreApp *
store_app_new (const gchar *name)
{
    StoreApp *self = g_object_new (store_app_get_type (), NULL);
    self->name = g_strdup (name);
    return self;
}

void
store_app_set_appstream_id (StoreApp *self, const gchar *appstream_id)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->appstream_id, g_free);
    self->appstream_id = g_strdup (appstream_id);
}

const gchar *
store_app_get_appstream_id (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->appstream_id;
}

void
store_app_set_description (StoreApp *self, const gchar *description)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->description, g_free);
    self->description = g_strdup (description);
}

const gchar *
store_app_get_description (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->description;
}

void
store_app_set_icon (StoreApp *self, StoreMedia *icon)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_object (&self->icon);
    if (icon != NULL)
        self->icon = g_object_ref (icon);
}

StoreMedia *
store_app_get_icon (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->icon;
}

const gchar *
store_app_get_name (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->name;
}

void
store_app_set_publisher (StoreApp *self, const gchar *publisher)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->publisher, g_free);
    self->publisher = g_strdup (publisher);
}

const gchar *
store_app_get_publisher (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->publisher;
}

void
store_app_set_summary (StoreApp *self, const gchar *summary)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->summary, g_free);
    self->summary = g_strdup (summary);
}

const gchar *
store_app_get_summary (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->summary;
}

void
store_app_set_title (StoreApp *self, const gchar *title)
{
    g_return_if_fail (STORE_IS_APP (self));
    g_clear_pointer (&self->title, g_free);
    self->title = g_strdup (title);
}

const gchar *
store_app_get_title (StoreApp *self)
{
    g_return_val_if_fail (STORE_IS_APP (self), NULL);
    return self->title;
}
