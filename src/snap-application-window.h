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

#include "snap-application.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapApplicationWindow, snap_application_window, SNAP, APPLICATION_WINDOW, GtkApplicationWindow)

SnapApplicationWindow *snap_application_window_new (SnapApplication *application);

G_END_DECLS
