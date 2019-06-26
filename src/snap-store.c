/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <glib/gi18n.h>
#include <locale.h>

#include <config.h>
#include "store-application.h"

int main (int argc, char **argv)
{
    setlocale (LC_ALL, "");

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    g_autoptr(StoreApplication) app = store_application_new ();
    return g_application_run (G_APPLICATION (app), argc, argv);
}
