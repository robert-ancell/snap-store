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

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StoreChannel, store_channel, STORE, CHANNEL, GObject)

StoreChannel *store_channel_new              (void);

StoreChannel *store_channel_new_from_json    (JsonNode *node);

JsonNode     *store_channel_to_json          (StoreChannel *channel);

void          store_channel_set_name         (StoreChannel *channel, const gchar *name);

const gchar  *store_channel_get_name         (StoreChannel *channel);

void          store_channel_set_release_date (StoreChannel *channel, GDateTime *release_date);

GDateTime    *store_channel_get_release_date (StoreChannel *channel);

void          store_channel_set_size         (StoreChannel *channel, gint64 size);

gint64        store_channel_get_size         (StoreChannel *channel);

void          store_channel_set_version      (StoreChannel *channel, const gchar *version);

const gchar  *store_channel_get_version      (StoreChannel *channel);

G_END_DECLS
