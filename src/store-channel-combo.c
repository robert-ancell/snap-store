/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-channel-combo.h"

struct _StoreChannelCombo
{
    GtkComboBox parent_instance;

    GtkCellRendererText *renderer;
    GtkListStore *channel_model;

    StoreApp *app;
    gulong channels_handler;
};

G_DEFINE_TYPE (StoreChannelCombo, store_channel_combo, GTK_TYPE_COMBO_BOX)

static void
update_channels (StoreChannelCombo *self)
{
    gtk_list_store_clear (self->channel_model);

    if (self->app == NULL)
        return;

    GPtrArray *channels = store_app_get_channels (self->app);
    for (guint i = 0; i < channels->len; i++) {
        StoreChannel *channel = g_ptr_array_index (channels, i);
        GtkTreeIter iter;
        gtk_list_store_append (self->channel_model, &iter);
        g_autofree gchar *label = g_strdup_printf ("%s %s", store_channel_get_name (channel), store_channel_get_version (channel));
        gtk_list_store_set (self->channel_model, &iter, 0, label, -1);
    }
}

static void
store_channel_combo_dispose (GObject *object)
{
    StoreChannelCombo *self = STORE_CHANNEL_COMBO (object);

    g_clear_object (&self->app);

    G_OBJECT_CLASS (store_channel_combo_parent_class)->dispose (object);
}

static void
store_channel_combo_class_init (StoreChannelComboClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_channel_combo_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/io/snapcraft/Store/store-channel-combo.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreChannelCombo, channel_model);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), StoreChannelCombo, renderer);
}

static void
store_channel_combo_init (StoreChannelCombo *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (self), GTK_CELL_RENDERER (self->renderer), "text", 0);
}

void
store_channel_combo_set_app (StoreChannelCombo *self, StoreApp *app)
{
    g_return_if_fail (STORE_IS_CHANNEL_COMBO (self));

    if (self->app == app)
        return;

    if (self->channels_handler != 0) {
        g_signal_handler_disconnect (self->app, self->channels_handler);
        self->channels_handler = 0;
    }

    g_set_object (&self->app, app);

    if (app != NULL)
        self->channels_handler = g_signal_connect_object (app, "notify::channels", G_CALLBACK (update_channels), self, G_CONNECT_SWAPPED);
    update_channels (self);
}
