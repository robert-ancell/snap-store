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

#include "store-media.h"
#include "store-model.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreImage, store_image, STORE, IMAGE, GtkDrawingArea)

StoreImage *store_image_new       (void);

void        store_image_set_media (StoreImage *image, StoreMedia *media);

void        store_image_set_model (StoreImage *image, StoreModel *model);

void        store_image_set_size  (StoreImage *image, guint width, guint height);

void        store_image_set_uri   (StoreImage *image, const gchar *uri);

G_END_DECLS
