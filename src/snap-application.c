/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "snap-application.h"
#include "snap-application-window.h"

struct _SnapApplication
{
    GtkApplication parent_instance;

    SnapApplicationWindow *window;
};

G_DEFINE_TYPE (SnapApplication, snap_application, GTK_TYPE_APPLICATION)

static void
snap_application_startup (GApplication *application)
{
    SnapApplication *self = SNAP_APPLICATION (application);

    G_APPLICATION_CLASS (snap_application_parent_class)->startup (application);

    self->window = snap_application_window_new (self);
}

static void
snap_application_activate (GApplication *application)
{
    SnapApplication *self = SNAP_APPLICATION (application);
    gtk_window_present (GTK_WINDOW (self->window));
}

static void
snap_application_class_init (SnapApplicationClass *klass)
{
    G_APPLICATION_CLASS (klass)->startup = snap_application_startup;
    G_APPLICATION_CLASS (klass)->activate = snap_application_activate;
}

static void
snap_application_init (SnapApplication *self G_GNUC_UNUSED)
{
}

SnapApplication *
snap_application_new (void)
{
    return g_object_new (snap_application_get_type (), NULL);
}
