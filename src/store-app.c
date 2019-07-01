/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>

#include "store-app.h"

typedef struct
{
    gchar *appstream_id;
    GPtrArray *channels;
    gchar *contact;
    gchar *description;
    StoreMedia *icon;
    gboolean installed;
    gchar *license;
    gchar *name;
    gchar *publisher;
    gboolean publisher_validated;
    GPtrArray *screenshots;
    gint64 one_star_review_count;
    gint64 two_star_review_count;
    gint64 three_star_review_count;
    gint64 four_star_review_count;
    gint64 five_star_review_count;
    gchar *summary;
    gchar *title;
} StoreAppPrivate;

enum
{
    PROP_0,
    PROP_CHANNELS,
    PROP_CONTACT,
    PROP_DESCRIPTION,
    PROP_ICON,
    PROP_INSTALLED,
    PROP_LICENSE,
    PROP_NAME,
    PROP_PUBLISHER,
    PROP_PUBLISHER_VALIDATED,
    PROP_RATINGS_AVERAGE,
    PROP_RATINGS_TOTAL,
    PROP_SCREENSHOTS,
    PROP_ONE_STAR_REVIEW_COUNT,
    PROP_TWO_STAR_REVIEW_COUNT,
    PROP_THREE_STAR_REVIEW_COUNT,
    PROP_FOUR_STAR_REVIEW_COUNT,
    PROP_FIVE_STAR_REVIEW_COUNT,
    PROP_SUMMARY,
    PROP_TITLE,
    PROP_LAST
};

G_DEFINE_TYPE_WITH_PRIVATE (StoreApp, store_app, G_TYPE_OBJECT)

static void
store_app_dispose (GObject *object)
{
    StoreApp *self = STORE_APP (object);
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_clear_pointer (&priv->appstream_id, g_free);
    g_clear_pointer (&priv->channels, g_ptr_array_unref);
    g_clear_pointer (&priv->contact, g_free);
    g_clear_pointer (&priv->description, g_free);
    g_clear_object (&priv->icon);
    g_clear_pointer (&priv->license, g_free);
    g_clear_pointer (&priv->name, g_free);
    g_clear_pointer (&priv->publisher, g_free);
    g_clear_pointer (&priv->screenshots, g_ptr_array_unref);
    g_clear_pointer (&priv->summary, g_free);
    g_clear_pointer (&priv->title, g_free);

    G_OBJECT_CLASS (store_app_parent_class)->dispose (object);
}

static void
store_app_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    StoreApp *self = STORE_APP (object);
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    switch (prop_id)
    {
    case PROP_CHANNELS:
        g_value_set_boxed (value, priv->channels);
        break;
    case PROP_CONTACT:
        g_value_set_string (value, priv->contact);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, priv->description);
        break;
    case PROP_ICON:
        g_value_set_object (value, priv->icon);
        break;
    case PROP_INSTALLED:
        g_value_set_boolean (value, priv->installed);
        break;
    case PROP_LICENSE:
        g_value_set_string (value, priv->license);
        break;
    case PROP_NAME:
        g_value_set_string (value, priv->name);
        break;
    case PROP_PUBLISHER:
        g_value_set_string (value, priv->publisher);
        break;
    case PROP_PUBLISHER_VALIDATED:
        g_value_set_boolean (value, priv->publisher_validated);
        break;
    case PROP_RATINGS_AVERAGE:
        g_value_set_int (value, store_app_get_ratings_average (self));
        break;
    case PROP_RATINGS_TOTAL:
        g_value_set_int (value, store_app_get_ratings_total (self));
        break;
    case PROP_SCREENSHOTS:
        g_value_set_boxed (value, priv->screenshots);
        break;
    case PROP_ONE_STAR_REVIEW_COUNT:
        g_value_set_int64 (value, priv->one_star_review_count);
        break;
    case PROP_TWO_STAR_REVIEW_COUNT:
        g_value_set_int64 (value, priv->two_star_review_count);
        break;
    case PROP_THREE_STAR_REVIEW_COUNT:
        g_value_set_int64 (value, priv->three_star_review_count);
        break;
    case PROP_FOUR_STAR_REVIEW_COUNT:
        g_value_set_int64 (value, priv->four_star_review_count);
        break;
    case PROP_FIVE_STAR_REVIEW_COUNT:
        g_value_set_int64 (value, priv->five_star_review_count);
        break;
    case PROP_SUMMARY:
        g_value_set_string (value, priv->summary);
        break;
    case PROP_TITLE:
        g_value_set_string (value, priv->title);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
store_app_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    StoreApp *self = STORE_APP (object);

    switch (prop_id)
    {
    case PROP_CHANNELS:
        store_app_set_channels (self, g_value_get_boxed (value));
        break;
    case PROP_CONTACT:
        store_app_set_contact (self, g_value_get_string (value));
        break;
    case PROP_DESCRIPTION:
        store_app_set_description (self, g_value_get_string (value));
        break;
    case PROP_ICON:
        store_app_set_icon (self, g_value_get_object (value));
        break;
    case PROP_INSTALLED:
        store_app_set_installed (self, g_value_get_boolean (value));
        break;
    case PROP_LICENSE:
        store_app_set_license (self, g_value_get_string (value));
        break;
    case PROP_NAME:
        store_app_set_name (self, g_value_get_string (value));
        break;
    case PROP_PUBLISHER:
        store_app_set_publisher (self, g_value_get_string (value));
        break;
    case PROP_PUBLISHER_VALIDATED:
        store_app_set_publisher_validated (self, g_value_get_boolean (value));
        break;
    case PROP_SCREENSHOTS:
        store_app_set_screenshots (self, g_value_get_boxed (value));
        break;
    case PROP_ONE_STAR_REVIEW_COUNT:
        store_app_set_one_star_review_count (self, g_value_get_int64 (value));
        break;
    case PROP_TWO_STAR_REVIEW_COUNT:
        store_app_set_two_star_review_count (self, g_value_get_int64 (value));
        break;
    case PROP_THREE_STAR_REVIEW_COUNT:
        store_app_set_three_star_review_count (self, g_value_get_int64 (value));
        break;
    case PROP_FOUR_STAR_REVIEW_COUNT:
        store_app_set_four_star_review_count (self, g_value_get_int64 (value));
        break;
    case PROP_FIVE_STAR_REVIEW_COUNT:
        store_app_set_five_star_review_count (self, g_value_get_int64 (value));
        break;
    case PROP_SUMMARY:
        store_app_set_summary (self, g_value_get_string (value));
        break;
    case PROP_TITLE:
        store_app_set_title (self, g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
install_string_property (StoreAppClass *klass, guint property_id, const gchar *name)
{
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     property_id,
                                     g_param_spec_string (name, NULL, NULL, NULL, G_PARAM_READWRITE));
}

static void
install_array_property (StoreAppClass *klass, guint property_id, const gchar *name)
{
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     property_id,
                                     g_param_spec_boxed (name, NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_READWRITE));
}

static void
install_object_property (StoreAppClass *klass, guint property_id, const gchar *name, GType type)
{
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     property_id,
                                     g_param_spec_object (name, NULL, NULL, type, G_PARAM_READWRITE));
}

static void
install_boolean_property (StoreAppClass *klass, guint property_id, const gchar *name)
{
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     property_id,
                                     g_param_spec_boolean (name, NULL, NULL, FALSE, G_PARAM_READWRITE));
}

static void
store_app_class_init (StoreAppClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = store_app_dispose;
    G_OBJECT_CLASS (klass)->get_property = store_app_get_property;
    G_OBJECT_CLASS (klass)->set_property = store_app_set_property;

    install_array_property (klass, PROP_CHANNELS, "channels");
    install_string_property (klass, PROP_CONTACT, "contact");
    install_string_property (klass, PROP_DESCRIPTION, "description");
    install_object_property (klass, PROP_ICON, "icon", store_media_get_type ());
    install_boolean_property (klass, PROP_INSTALLED, "installed");
    install_string_property (klass, PROP_LICENSE, "license");
    install_string_property (klass, PROP_NAME, "name");
    install_string_property (klass, PROP_PUBLISHER, "publisher");
    install_boolean_property (klass, PROP_PUBLISHER_VALIDATED, "publisher-validated");
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_RATINGS_AVERAGE,
                                     g_param_spec_int ("ratings-average", NULL, NULL, G_MININT, G_MAXINT, 0, G_PARAM_READABLE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_RATINGS_TOTAL,
                                     g_param_spec_int ("ratings-total", NULL, NULL, G_MININT, G_MAXINT, 0, G_PARAM_READABLE));
    install_array_property (klass, PROP_SCREENSHOTS, "screenshots");
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_ONE_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("one-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_TWO_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("two-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_THREE_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("three-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_FOUR_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("four-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (klass),
                                     PROP_FIVE_STAR_REVIEW_COUNT,
                                     g_param_spec_int64 ("five-star-review-count", NULL, NULL, G_MININT64, G_MAXINT64, 0, G_PARAM_READWRITE));
    install_string_property (klass, PROP_SUMMARY, "summary");
    install_string_property (klass, PROP_TITLE, "title");
}

static void
store_app_init (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    priv->channels = g_ptr_array_new ();
    priv->screenshots = g_ptr_array_new ();
}

void
store_app_install_async (StoreApp *self, StoreChannel *channel, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_APP (self));
    STORE_APP_GET_CLASS (self)->install_async (self, channel, cancellable, callback, callback_data);
}

gboolean
store_app_install_finish (StoreApp *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_APP (self), FALSE);
    return STORE_APP_GET_CLASS (self)->install_finish (self, result, error);
}

void
store_app_refresh_async (StoreApp *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_APP (self));
    STORE_APP_GET_CLASS (self)->refresh_async (self, cancellable, callback, callback_data);
}

gboolean
store_app_refresh_finish (StoreApp *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_APP (self), FALSE);
    return STORE_APP_GET_CLASS (self)->refresh_finish (self, result, error);
}

void
store_app_remove_async (StoreApp *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer callback_data)
{
    g_return_if_fail (STORE_IS_APP (self));
    STORE_APP_GET_CLASS (self)->remove_async (self, cancellable, callback, callback_data);
}

gboolean
store_app_remove_finish (StoreApp *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (STORE_IS_APP (self), FALSE);
    return STORE_APP_GET_CLASS (self)->remove_finish (self, result, error);
}

void
store_app_save_to_cache (StoreApp *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_APP (self));
    STORE_APP_GET_CLASS (self)->save_to_cache (self, cache);
}

void
store_app_update_from_cache (StoreApp *self, StoreCache *cache)
{
    g_return_if_fail (STORE_IS_APP (self));
    STORE_APP_GET_CLASS (self)->update_from_cache (self, cache);
}

void
store_app_set_appstream_id (StoreApp *self, const gchar *appstream_id)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->appstream_id, g_free);
    priv->appstream_id = g_strdup (appstream_id);
}

const gchar *
store_app_get_appstream_id (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->appstream_id;
}

void
store_app_set_channels (StoreApp *self, GPtrArray *channels)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->channels, g_ptr_array_unref);
    if (channels != NULL)
        priv->channels = g_ptr_array_ref (channels);

    g_object_notify (G_OBJECT (self), "channels");
}

GPtrArray *
store_app_get_channels (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->channels;
}

void
store_app_set_contact (StoreApp *self, const gchar *contact)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->contact, g_free);
    priv->contact = g_strdup (contact);

    g_object_notify (G_OBJECT (self), "contact");
}

const gchar *
store_app_get_contact (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->contact;
}

void
store_app_set_description (StoreApp *self, const gchar *description)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->description, g_free);
    priv->description = g_strdup (description);

    g_object_notify (G_OBJECT (self), "description");
}

const gchar *
store_app_get_description (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->description;
}

void
store_app_set_icon (StoreApp *self, StoreMedia *icon)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_object (&priv->icon);
    if (icon != NULL)
        priv->icon = g_object_ref (icon);

    g_object_notify (G_OBJECT (self), "icon");
}

StoreMedia *
store_app_get_icon (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->icon;
}

void
store_app_set_installed (StoreApp *self, gboolean installed)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    priv->installed = installed;

    g_object_notify (G_OBJECT (self), "installed");
}

gboolean
store_app_get_installed (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), FALSE);

    return priv->installed;
}

void
store_app_set_license (StoreApp *self, const gchar *license)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->license, g_free);
    priv->license = g_strdup (license);

    g_object_notify (G_OBJECT (self), "license");
}

const gchar *
store_app_get_license (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->license;
}

void
store_app_set_name (StoreApp *self, const gchar *name)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->name, g_free);
    priv->name = g_strdup (name);

    g_object_notify (G_OBJECT (self), "name");
}

const gchar *
store_app_get_name (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->name;
}

void
store_app_set_publisher (StoreApp *self, const gchar *publisher)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->publisher, g_free);
    priv->publisher = g_strdup (publisher);

    g_object_notify (G_OBJECT (self), "publisher");
}

const gchar *
store_app_get_publisher (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->publisher;
}

void
store_app_set_publisher_validated (StoreApp *self, gboolean validated)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    priv->publisher_validated = validated;
}

gboolean
store_app_get_publisher_validated (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), FALSE);

    return priv->publisher_validated;
}

static gdouble
pnormaldist (gdouble qn)
{
    static gdouble b[11] = {
        1.570796288,      0.03706987906,   -0.8364353589e-3,
       -0.2250947176e-3,  0.6841218299e-5,  0.5824238515e-5,
       -0.104527497e-5,   0.8360937017e-7, -0.3231081277e-8,
        0.3657763036e-10, 0.6936233982e-12 };
    gdouble w1, w3;
    guint i;

    if (qn < 0 || qn > 1)
        return 0; // This is an error case
    if (qn == 0.5)
        return 0;

    w1 = qn;
    if (qn > 0.5)
        w1 = 1.0 - w1;
    w3 = -log (4.0 * w1 * (1.0 - w1));
    w1 = b[0];
    for (i = 1; i < 11; i++)
        w1 = w1 + (b[i] * pow (w3, i));

    if (qn > 0.5)
        return sqrt (w1 * w3);
    else
        return -sqrt (w1 * w3);
}

static gdouble
wilson_score (gdouble value, gdouble n, gdouble power)
{
    if (value == 0)
        return 0;

    gdouble z = pnormaldist (1 - power / 2);
    gdouble phat = value / n;
    return (phat + z * z / (2 * n) - z * sqrt ((phat * (1 - phat) + z * z / (4 * n)) / n)) / (1 + z * z / n);
}

gint
store_app_get_ratings_average (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), -1);

    gdouble total = store_app_get_ratings_total (self);
    if (total == 0)
         return -1;

    gdouble val = wilson_score (priv->one_star_review_count, total, 0.2) * -2 +
                  wilson_score (priv->two_star_review_count, total, 0.2) * -1 +
                  wilson_score (priv->four_star_review_count, total, 0.2) *  1 +
                  wilson_score (priv->five_star_review_count, total, 0.2) *  2;

    return ceil (20 * (val + 3));
}

gint
store_app_get_ratings_total (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), -1);

    return priv->one_star_review_count + priv->two_star_review_count + priv->three_star_review_count + priv->four_star_review_count + priv->five_star_review_count;
}

void
store_app_set_screenshots (StoreApp *self, GPtrArray *screenshots)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->screenshots, g_ptr_array_unref);
    if (screenshots != NULL)
        priv->screenshots = g_ptr_array_ref (screenshots);

    g_object_notify (G_OBJECT (self), "screenshots");
}

GPtrArray *
store_app_get_screenshots (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->screenshots;
}

void
store_app_set_one_star_review_count (StoreApp *self, gint64 count)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    priv->one_star_review_count = count;

    g_object_notify (G_OBJECT (self), "one-star-review-count");
    g_object_notify (G_OBJECT (self), "ratings-average");
    g_object_notify (G_OBJECT (self), "ratings-total");
}

void
store_app_set_two_star_review_count (StoreApp *self, gint64 count)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    priv->two_star_review_count = count;

    g_object_notify (G_OBJECT (self), "two-star-review-count");
    g_object_notify (G_OBJECT (self), "ratings-average");
    g_object_notify (G_OBJECT (self), "ratings-total");
}

void
store_app_set_three_star_review_count (StoreApp *self, gint64 count)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    priv->three_star_review_count = count;

    g_object_notify (G_OBJECT (self), "three-star-review-count");
    g_object_notify (G_OBJECT (self), "ratings-average");
    g_object_notify (G_OBJECT (self), "ratings-total");
}

void
store_app_set_four_star_review_count (StoreApp *self, gint64 count)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    priv->four_star_review_count = count;

    g_object_notify (G_OBJECT (self), "four-star-review-count");
    g_object_notify (G_OBJECT (self), "ratings-average");
    g_object_notify (G_OBJECT (self), "ratings-total");
}

void
store_app_set_five_star_review_count (StoreApp *self, gint64 count)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    priv->five_star_review_count = count;

    g_object_notify (G_OBJECT (self), "five-star-review-count");
    g_object_notify (G_OBJECT (self), "ratings-average");
    g_object_notify (G_OBJECT (self), "ratings-total");
}

void
store_app_set_summary (StoreApp *self, const gchar *summary)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->summary, g_free);
    priv->summary = g_strdup (summary);

    g_object_notify (G_OBJECT (self), "summary");
}

const gchar *
store_app_get_summary (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->summary;
}

void
store_app_set_title (StoreApp *self, const gchar *title)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_if_fail (STORE_IS_APP (self));

    g_clear_pointer (&priv->title, g_free);
    priv->title = g_strdup (title);

    g_object_notify (G_OBJECT (self), "title");
}

const gchar *
store_app_get_title (StoreApp *self)
{
    StoreAppPrivate *priv = store_app_get_instance_private (self);

    g_return_val_if_fail (STORE_IS_APP (self), NULL);

    return priv->title;
}
