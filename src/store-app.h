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

G_DECLARE_FINAL_TYPE (StoreApp, store_app, STORE, APP, GObject)

StoreApp    *store_app_new             (const gchar *name);

const gchar *store_app_get_description (StoreApp *app);

const gchar *store_app_get_name        (StoreApp *app);

const gchar *store_app_get_publisher   (StoreApp *app);

const gchar *store_app_get_summary     (StoreApp *app);

const gchar *store_app_get_title       (StoreApp *app);

G_END_DECLS
