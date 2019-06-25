/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <snapd-glib/snapd-glib.h>

#include "store-app.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreSnapApp, store_snap_app, STORE, SNAP_APP, StoreApp)

StoreSnapApp *store_snap_app_new                (void);

void          store_snap_app_update_from_search (StoreSnapApp *app, SnapdSnap *snap);

G_END_DECLS
