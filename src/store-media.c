/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-media.h"

struct _StoreMedia
{
    GObject parent_instance;

    gchar *url;
};

G_DEFINE_TYPE (StoreMedia, store_media, G_TYPE_OBJECT)

static void
store_media_dispose (GObject *object)
{
    StoreMedia *self = STORE_MEDIA (object);
    g_clear_pointer (&self->url, g_free);
}

static void
store_media_class_init (StoreMediaClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_media_dispose;
}

static void
store_media_init (StoreMedia *self)
{
    self->url = g_strdup ("");
}

StoreMedia *
store_media_new (void)
{
    return g_object_new (store_media_get_type (), NULL);
}

void
store_media_set_url (StoreMedia *self, const gchar *url)
{
    g_return_if_fail (STORE_IS_MEDIA (self));
    g_clear_pointer (&self->url, g_free);
    self->url = g_strdup (url);
}

const gchar *
store_media_get_url (StoreMedia *self)
{
    g_return_val_if_fail (STORE_IS_MEDIA (self), NULL);
    return self->url;
}
