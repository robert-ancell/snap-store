/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <gtk/gtk.h>
#include <libsoup/soup.h>

#include "store-cache.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreImage, store_image, STORE, IMAGE, GtkImage)

StoreImage *store_image_new         (void);

void        store_image_set_cache   (StoreImage *image, StoreCache *cache);

void        store_image_set_session (StoreImage *image, SoupSession *session);

void        store_image_set_size    (StoreImage *image, guint width, guint height);

void        store_image_set_uri     (StoreImage *image, const gchar *uri);

G_END_DECLS
