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
#include <json-glib/json-glib.h>

#include "store-media.h"

G_BEGIN_DECLS

G_DECLARE_DERIVABLE_TYPE (StoreApp, store_app, STORE, APP, GObject)

struct _StoreAppClass
{
    GObjectClass parent_class;

    void     (*refresh_async)  (StoreApp *app, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);
    gboolean (*refresh_finish) (StoreApp *app, GAsyncResult *result, GError **error);
};

StoreApp    *store_app_new                     (void);

StoreApp    *store_app_new_from_json           (JsonNode *node);

JsonNode    *store_app_to_json                 (StoreApp *app);

void         store_app_refresh_async           (StoreApp *app, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data);

gboolean     store_app_refresh_finish          (StoreApp *app, GAsyncResult *result, GError **error);

void         store_app_set_appstream_id        (StoreApp *app, const gchar *appstream_id);

const gchar *store_app_get_appstream_id        (StoreApp *app);

void         store_app_set_description         (StoreApp *app, const gchar *description);

const gchar *store_app_get_description         (StoreApp *app);

void         store_app_set_icon                (StoreApp *app, StoreMedia *icon);

StoreMedia  *store_app_get_icon                (StoreApp *app);

void         store_app_set_name                (StoreApp *app, const gchar *name);

const gchar *store_app_get_name                (StoreApp *app);

void         store_app_set_publisher           (StoreApp *app, const gchar *publisher);

const gchar *store_app_get_publisher           (StoreApp *app);

void         store_app_set_publisher_validated (StoreApp *app, gboolean validated);

gboolean     store_app_get_publisher_validated (StoreApp *app);

void         store_app_set_screenshots         (StoreApp *app, GPtrArray *screenshots);

GPtrArray   *store_app_get_screenshots         (StoreApp *app);

void         store_app_set_summary             (StoreApp *app, const gchar *summary);

const gchar *store_app_get_summary             (StoreApp *app);

void         store_app_set_title               (StoreApp *app, const gchar *title);

const gchar *store_app_get_title               (StoreApp *app);

G_END_DECLS
