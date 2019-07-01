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

#include "store-channel.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreChannelCombo, store_channel_combo, STORE, CHANNEL_COMBO, GtkComboBox)

void store_channel_combo_set_channels (StoreChannelCombo *combo, GPtrArray *channels);

G_END_DECLS
