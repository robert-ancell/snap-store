/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreMedia, store_media, STORE, MEDIA, GObject)

StoreMedia  *store_media_new        (void);

void         store_media_set_height (StoreMedia *media, guint height);

guint        store_media_get_height (StoreMedia *media);

void         store_media_set_width  (StoreMedia *media, guint width);

guint        store_media_get_width  (StoreMedia *media);

void         store_media_set_url    (StoreMedia *media, const gchar *url);

const gchar *store_media_get_url    (StoreMedia *media);

G_END_DECLS
