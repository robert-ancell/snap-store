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
};

enum
{
    PROP_0,
    PROP_CHANNELS,
    PROP_LAST
};

G_DEFINE_TYPE (StoreChannelCombo, store_channel_combo, GTK_TYPE_COMBO_BOX)

static void
store_channel_combo_get_property (GObject *object, guint prop_id, GValue *value G_GNUC_UNUSED, GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
store_channel_combo_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreChannelCombo *self = STORE_CHANNEL_COMBO (object);

    switch (prop_id)
    {
    case PROP_CHANNELS:
        store_channel_combo_set_channels (self, g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_channel_combo_class_init (StoreChannelComboClass *klass)
{
    G_OBJECT_CLASS (klass)->get_property = store_channel_combo_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_channel_combo_set_property;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_CHANNELS,
                                     g_param_spec_boxed ("channels", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_WRITABLE));

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
store_channel_combo_set_channels (StoreChannelCombo *self, GPtrArray *channels)
{
    g_return_if_fail (STORE_IS_CHANNEL_COMBO (self));

    gtk_list_store_clear (self->channel_model);

    for (guint i = 0; i < channels->len; i++) {
        StoreChannel *channel = g_ptr_array_index (channels, i);
        GtkTreeIter iter;
        gtk_list_store_append (self->channel_model, &iter);
        g_autofree gchar *label = g_strdup_printf ("%s %s", store_channel_get_name (channel), store_channel_get_version (channel));
        gtk_list_store_set (self->channel_model, &iter, 0, label, -1);
    }
}
