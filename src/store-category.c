/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-category.h"

struct _StoreCategory
{
    GObject parent_instance;

    GPtrArray *apps;
    gchar *name;
    gchar *summary;
    gchar *title;
};

enum
{
    PROP_0,
    PROP_APPS,
    PROP_NAME,
    PROP_SUMMARY,
    PROP_TITLE,
    PROP_LAST
};

G_DEFINE_TYPE (StoreCategory, store_category, G_TYPE_OBJECT)

static void
store_category_dispose (GObject *object)
{
    StoreCategory *self = STORE_CATEGORY (object);

    g_clear_pointer (&self->apps, g_ptr_array_unref);
    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->summary, g_free);
    g_clear_pointer (&self->title, g_free);

    G_OBJECT_CLASS (store_category_parent_class)->dispose (object);
}

static void
store_category_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    StoreCategory *self = STORE_CATEGORY (object);

    switch (prop_id)
    {
    case PROP_APPS:
        g_value_set_boxed (value, self->apps);
        break;
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    case PROP_SUMMARY:
        g_value_set_string (value, self->summary);
        break;
    case PROP_TITLE:
        g_value_set_string (value, self->title);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_category_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreCategory *self = STORE_CATEGORY (object);

    switch (prop_id)
    {
    case PROP_APPS:
        store_category_set_apps (self, g_value_get_boxed (value));
        break;
    case PROP_NAME:
        store_category_set_name (self, g_value_get_string (value));
        break;
    case PROP_SUMMARY:
        store_category_set_summary (self, g_value_get_string (value));
        break;
    case PROP_TITLE:
        store_category_set_title (self, g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_category_class_init (StoreCategoryClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_category_dispose;
    G_OBJECT_CLASS (klass)->get_property = store_category_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_category_set_property;

    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_APPS,
                                     g_param_spec_boxed ("apps", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_NAME,
                                     g_param_spec_string ("name", NULL, NULL, NULL, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_SUMMARY,
                                     g_param_spec_string ("summary", NULL, NULL, NULL, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_TITLE,
                                     g_param_spec_string ("title", NULL, NULL, NULL, G_PARAM_READWRITE));
}

static void
store_category_init (StoreCategory *self G_GNUC_UNUSED)
{
}

StoreCategory *
store_category_new (void)
{
    return g_object_new (store_category_get_type (), NULL);
}

void
store_category_set_apps (StoreCategory *self, GPtrArray *apps)
{
    g_return_if_fail (STORE_IS_CATEGORY (self));

    if (self->apps == apps)
        return;
    // FIXME: Also check for arrays with same contents

    g_clear_pointer (&self->apps, g_ptr_array_unref);
    if (apps != NULL)
        self->apps = g_ptr_array_ref (apps);

    g_object_notify (G_OBJECT (self), "apps");
}

GPtrArray *
store_category_get_apps (StoreCategory *self)
{
    g_return_val_if_fail (STORE_IS_CATEGORY (self), NULL);

    return self->apps;
}

void
store_category_set_name (StoreCategory *self, const gchar *name)
{
    g_return_if_fail (STORE_IS_CATEGORY (self));

    g_clear_pointer (&self->name, g_free);
    self->name = g_strdup (name);

    g_object_notify (G_OBJECT (self), "name");
}

const gchar *
store_category_get_name (StoreCategory *self)
{
    g_return_val_if_fail (STORE_IS_CATEGORY (self), NULL);

    return self->name;
}

void
store_category_set_summary (StoreCategory *self, const gchar *summary)
{
    g_return_if_fail (STORE_IS_CATEGORY (self));

    g_clear_pointer (&self->summary, g_free);
    self->summary = g_strdup (summary);

    g_object_notify (G_OBJECT (self), "summary");
}

const gchar *
store_category_get_summary (StoreCategory *self)
{
    g_return_val_if_fail (STORE_IS_CATEGORY (self), NULL);

    return self->summary;
}

void
store_category_set_title (StoreCategory *self, const gchar *title)
{
    g_return_if_fail (STORE_IS_CATEGORY (self));

    g_clear_pointer (&self->title, g_free);
    self->title = g_strdup (title);

    g_object_notify (G_OBJECT (self), "title");
}

const gchar *
store_category_get_title (StoreCategory *self)
{
    g_return_val_if_fail (STORE_IS_CATEGORY (self), NULL);

    return self->title;
}
