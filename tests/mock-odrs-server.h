/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <libsoup/soup.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (MockOdrsServer, mock_odrs_server, MOCK, ODRS_SERVER, SoupServer)

typedef struct _MockApp MockApp;
typedef struct _MockReview MockReview;

MockOdrsServer *mock_odrs_server_new         (void);

void            mock_odrs_server_set_port    (MockOdrsServer *server, guint port);

guint           mock_odrs_server_get_port    (MockOdrsServer *server);

MockApp        *mock_odrs_server_add_app     (MockOdrsServer *server, const gchar *id);

MockApp        *mock_odrs_server_find_app    (MockOdrsServer *server, const gchar *id);

MockReview     *mock_app_add_review          (MockApp *app);

MockReview     *mock_app_find_review         (MockApp *app, gint64 id);

void            mock_review_set_locale       (MockReview *review, const gchar *locale);

void            mock_review_set_distro       (MockReview *review, const gchar *distro);

void            mock_review_set_version      (MockReview *review, const gchar *version);

void            mock_review_set_user_display (MockReview *review, const gchar *user_display);

void            mock_review_set_summary      (MockReview *review, const gchar *summary);

void            mock_review_set_description  (MockReview *review, const gchar *description);

void            mock_review_set_rating       (MockReview *review, gint64 rating);

G_END_DECLS
