/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-channel.h"

struct _StoreChannel
{
    GObject parent_instance;

    gchar *name;
    gchar *version;
};

G_DEFINE_TYPE (StoreChannel, store_channel, G_TYPE_OBJECT)

static void
store_channel_dispose (GObject *object)
{
    StoreChannel *self = STORE_CHANNEL (object);

    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->version, g_free);

    G_OBJECT_CLASS (store_channel_parent_class)->dispose (object);
}

static void
store_channel_class_init (StoreChannelClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_channel_dispose;
}

static void
store_channel_init (StoreChannel *self)
{
    self->name = g_strdup ("");
    self->version = g_strdup ("");
}

StoreChannel *
store_channel_new (void)
{
    return g_object_new (store_channel_get_type (), NULL);
}

StoreChannel *
store_channel_new_from_json (JsonNode *node)
{
    StoreChannel *self = store_channel_new ();

    JsonObject *object = json_node_get_object (node);
    store_channel_set_name (self, json_object_get_string_member (object, "name"));
    store_channel_set_version (self, json_object_get_string_member (object, "version"));

    return self;
}

JsonNode *
store_channel_to_json (StoreChannel *self)
{
    g_return_val_if_fail (STORE_IS_CHANNEL (self), NULL);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "name");
    json_builder_add_string_value (builder, self->name);
    json_builder_set_member_name (builder, "version");
    json_builder_add_string_value (builder, self->version);
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

void
store_channel_set_name (StoreChannel *self, const gchar *name)
{
    g_return_if_fail (STORE_IS_CHANNEL (self));
    g_clear_pointer (&self->name, g_free);
    self->name = g_strdup (name);
}

const gchar *
store_channel_get_name (StoreChannel *self)
{
    g_return_val_if_fail (STORE_IS_CHANNEL (self), NULL);
    return self->name;
}

void
store_channel_set_version (StoreChannel *self, const gchar *version)
{
    g_return_if_fail (STORE_IS_CHANNEL (self));
    g_clear_pointer (&self->version, g_free);
    self->version = g_strdup (version);
}

const gchar *
store_channel_get_version (StoreChannel *self)
{
    g_return_val_if_fail (STORE_IS_CHANNEL (self), NULL);
    return self->version;
}
