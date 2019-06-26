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

#include "store-app.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreInstallButton, store_install_button, STORE, INSTALL_BUTTON, GtkButton)

void store_install_button_set_app (StoreInstallButton *install_button, StoreApp *app);

G_END_DECLS
