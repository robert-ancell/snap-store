/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-page.h"

typedef struct
{
    StoreModel *model;
} StorePagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (StorePage, store_page, GTK_TYPE_BIN)

static void
store_page_dispose (GObject *object)
{
    StorePage *self = STORE_PAGE (object);
    StorePagePrivate *priv = store_page_get_instance_private (self);

    g_clear_object (&priv->model);

    G_OBJECT_CLASS (store_page_parent_class)->dispose (object);
}

static void
store_page_real_set_model (StorePage *self, StoreModel *model)
{
    StorePagePrivate *priv = store_page_get_instance_private (self);

    g_set_object (&priv->model, model);
}

static void
store_page_class_init (StorePageClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_page_dispose;

    klass->set_model = store_page_real_set_model;
}

static void
store_page_init (StorePage *self G_GNUC_UNUSED)
{
}

void
store_page_set_model (StorePage *self, StoreModel *model)
{
    g_return_if_fail (STORE_IS_PAGE (self));

    STORE_PAGE_GET_CLASS (self)->set_model (self, model);
}

StoreModel *
store_page_get_model (StorePage *self)
{
    StorePagePrivate *priv = store_page_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_PAGE (self), NULL);

    return priv->model;
}
