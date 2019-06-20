/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "snap-application-window.h"

struct _SnapApplicationWindow
{
    GtkApplicationWindow parent_instance;
};

G_DEFINE_TYPE (SnapApplicationWindow, snap_application_window, GTK_TYPE_APPLICATION_WINDOW)

static void
snap_application_window_dispose (GObject *object)
{
    SnapApplicationWindow *self = SNAP_APPLICATION_WINDOW (object);
}

static void
snap_application_window_class_init (SnapApplicationWindowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = snap_application_window_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), "/com/ubuntu/SnapStore/snap-application-window.ui");
}

static void
snap_application_window_init (SnapApplicationWindow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

SnapApplicationWindow *
snap_application_window_new (SnapApplication *application)
{
    return g_object_new (snap_application_window_get_type (),
                         "application", application,
                         NULL);
}
