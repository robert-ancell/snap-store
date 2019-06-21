/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "store-application.h"

int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    g_autoptr(StoreApplication) app = store_application_new ();
    return g_application_run (G_APPLICATION (app), argc, argv);
}
