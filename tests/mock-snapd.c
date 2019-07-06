/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <glib/gstdio.h>
#include <gio/gunixsocketaddress.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "mock-snapd.h"

struct _MockSnapd
{
    GObject parent_instance;

    GMutex mutex;
    GCond condition;

    GThread *thread;
    GError **thread_init_error;
    GMainLoop *loop;
    GMainContext *context;

    gchar *dir_path;
    gchar *socket_path;
    gboolean close_on_request;
    gboolean decline_auth;
    GList *interfaces;
    GList *snaps;
    gchar *build_id;
    gchar *confinement;
    GHashTable *sandbox_features;
    gchar *store;
    gchar *maintenance_kind;
    gchar *maintenance_message;
    gboolean managed;
    gboolean on_classic;
    gchar *refresh_hold;
    gchar *refresh_last;
    gchar *refresh_next;
    gchar *refresh_schedule;
    gchar *refresh_timer;
    GList *store_sections;
    GList *store_snaps;
    GList *established_connections;
    GList *undesired_connections;
    GList *assertions;
    int change_index;
    GList *changes;
    gchar *spawn_time;
    gchar *ready_time;
    SoupMessageHeaders *last_request_headers;
};

G_DEFINE_TYPE (MockSnapd, mock_snapd, G_TYPE_OBJECT)

struct _MockApp
{
    gchar *name;
    gchar *common_id;
    gchar *daemon;
    gchar *desktop_file;
    gboolean enabled;
    gboolean active;
};

struct _MockChange
{
    gchar *id;
    gchar *kind;
    gchar *summary;
    gchar *spawn_time;
    gchar *ready_time;
    int task_index;
    GList *tasks;
    JsonNode *data;
};

struct _MockChannel
{
    gchar *risk;
    gchar *branch;
    gchar *confinement;
    gchar *epoch;
    gchar *released_at;
    gchar *revision;
    int size;
    gchar *version;
};

struct _MockMedia
{
    gchar *type;
    gchar *url;
    int width;
    int height;
};

struct _MockInterface
{
    gchar *name;
    gchar *summary;
    gchar *doc_url;
};

struct _MockPlug
{
    MockSnap *snap;
    gchar *name;
    MockInterface *interface;
    GHashTable *attributes;
    gchar *label;
};

struct _MockSlot
{
    MockSnap *snap;
    gchar *name;
    MockInterface *interface;
    GHashTable *attributes;
    gchar *label;
};

struct _MockConnection
{
    MockPlug *plug;
    MockSlot *slot;
    gboolean manual;
    gboolean gadget;
};

struct _MockSnap
{
    GList *apps;
    gchar *base;
    gchar *broken;
    gchar *channel;
    GHashTable *configuration;
    gchar *confinement;
    gchar *contact;
    gchar *description;
    gboolean devmode;
    int download_size;
    gchar *icon;
    gchar *icon_mime_type;
    GBytes *icon_data;
    gchar *id;
    gchar *install_date;
    int installed_size;
    gboolean jailmode;
    gchar *license;
    GList *media;
    gchar *mounted_from;
    gchar *name;
    gboolean is_private;
    gchar *publisher_display_name;
    gchar *publisher_id;
    gchar *publisher_username;
    gchar *publisher_validation;
    gchar *revision;
    gchar *status;
    gchar *summary;
    gchar *title;
    gchar *tracking_channel;
    GList *tracks;
    gboolean trymode;
    gchar *type;
    gchar *version;
    GList *store_sections;
    GList *plugs;
    GList *slots_;
    gboolean disabled;
    gboolean dangerous;
    gchar *snap_data;
    gchar *snap_path;
    gchar *error;
    gboolean restart_required;
    gboolean preferred;
    gboolean scope_is_wide;
};

struct _MockTask
{
    gchar *id;
    gchar *kind;
    gchar *summary;
    gchar *status;
    gchar *progress_label;
    int progress_done;
    int progress_total;
    gchar *spawn_time;
    gchar *ready_time;
    MockSnap *snap;
    gchar *snap_name;
    gchar *error;
};

struct _MockTrack
{
    gchar *name;
    GList *channels;
};

static void
mock_app_free (MockApp *app)
{
    g_free (app->name);
    g_free (app->common_id);
    g_free (app->daemon);
    g_free (app->desktop_file);
    g_slice_free (MockApp, app);
}

static void
mock_channel_free (MockChannel *channel)
{
    g_free (channel->risk);
    g_free (channel->branch);
    g_free (channel->confinement);
    g_free (channel->epoch);
    g_free (channel->released_at);
    g_free (channel->revision);
    g_free (channel->version);
    g_slice_free (MockChannel, channel);
}

static void
mock_track_free (MockTrack *track)
{
    g_free (track->name);
    g_list_free_full (track->channels, (GDestroyNotify) mock_channel_free);
    g_slice_free (MockTrack, track);
}

static void
mock_media_free (MockMedia *media)
{
    g_free (media->type);
    g_free (media->url);
    g_slice_free (MockMedia, media);
}

static void
mock_interface_free (MockInterface *interface)
{
    g_free (interface->name);
    g_free (interface->summary);
    g_free (interface->doc_url);
    g_slice_free (MockInterface, interface);
}

static void
mock_plug_free (MockPlug *plug)
{
    g_free (plug->name);
    g_hash_table_unref (plug->attributes);
    g_free (plug->label);
    g_slice_free (MockPlug, plug);
}

static void
mock_slot_free (MockSlot *slot)
{
    g_free (slot->name);
    g_hash_table_unref (slot->attributes);
    g_free (slot->label);
    g_slice_free (MockSlot, slot);
}

static void
mock_connection_free (MockConnection *connection)
{
    g_slice_free (MockConnection, connection);
}

static void
mock_snap_free (MockSnap *snap)
{
    g_list_free_full (snap->apps, (GDestroyNotify) mock_app_free);
    g_free (snap->base);
    g_free (snap->broken);
    g_free (snap->channel);
    g_hash_table_unref (snap->configuration);
    g_free (snap->confinement);
    g_free (snap->contact);
    g_free (snap->description);
    g_free (snap->icon);
    g_free (snap->icon_mime_type);
    g_bytes_unref (snap->icon_data);
    g_free (snap->id);
    g_free (snap->install_date);
    g_free (snap->license);
    g_list_free_full (snap->media, (GDestroyNotify) mock_media_free);
    g_free (snap->mounted_from);
    g_free (snap->name);
    g_free (snap->publisher_display_name);
    g_free (snap->publisher_id);
    g_free (snap->publisher_username);
    g_free (snap->publisher_validation);
    g_free (snap->revision);
    g_free (snap->status);
    g_free (snap->summary);
    g_free (snap->title);
    g_free (snap->tracking_channel);
    g_list_free_full (snap->tracks, (GDestroyNotify) mock_track_free);
    g_free (snap->type);
    g_free (snap->version);
    g_list_free_full (snap->store_sections, g_free);
    g_list_free_full (snap->plugs, (GDestroyNotify) mock_plug_free);
    g_list_free_full (snap->slots_, (GDestroyNotify) mock_slot_free);
    g_free (snap->snap_data);
    g_free (snap->snap_path);
    g_free (snap->error);
    g_slice_free (MockSnap, snap);
}

MockSnapd *
mock_snapd_new (void)
{
    return g_object_new (MOCK_TYPE_SNAPD, NULL);
}

const gchar *
mock_snapd_get_socket_path (MockSnapd *snapd)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);
    return snapd->socket_path;
}

void
mock_snapd_set_close_on_request (MockSnapd *snapd, gboolean close_on_request)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));
    snapd->close_on_request = close_on_request;
}

void
mock_snapd_set_decline_auth (MockSnapd *snapd, gboolean decline_auth)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));
    snapd->decline_auth = decline_auth;
}

void
mock_snapd_set_maintenance (MockSnapd *snapd, const gchar *kind, const gchar *message)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));
    g_free (snapd->maintenance_kind);
    snapd->maintenance_kind = g_strdup (kind);
    g_free (snapd->maintenance_message);
    snapd->maintenance_message = g_strdup (message);
}

void
mock_snapd_set_build_id (MockSnapd *snapd, const gchar *build_id)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->build_id);
    snapd->build_id = g_strdup (build_id);
}

void
mock_snapd_set_confinement (MockSnapd *snapd, const gchar *confinement)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->confinement);
    snapd->confinement = g_strdup (confinement);
}

void
mock_snapd_add_sandbox_feature (MockSnapd *snapd, const gchar *backend, const gchar *feature)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    GPtrArray *backend_features = g_hash_table_lookup (snapd->sandbox_features, backend);
    if (backend_features == NULL) {
        backend_features = g_ptr_array_new_with_free_func (g_free);
        g_hash_table_insert (snapd->sandbox_features, g_strdup (backend), backend_features);
    }
    g_ptr_array_add (backend_features, g_strdup (feature));
}

void
mock_snapd_set_store (MockSnapd *snapd, const gchar *name)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->store);
    snapd->store = g_strdup (name);
}

void
mock_snapd_set_managed (MockSnapd *snapd, gboolean managed)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    snapd->managed = managed;
}

void
mock_snapd_set_on_classic (MockSnapd *snapd, gboolean on_classic)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    snapd->on_classic = on_classic;
}

void
mock_snapd_set_refresh_hold (MockSnapd *snapd, const gchar *refresh_hold)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_hold);
    snapd->refresh_hold = g_strdup (refresh_hold);
}

void
mock_snapd_set_refresh_last (MockSnapd *snapd, const gchar *refresh_last)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_last);
    snapd->refresh_last = g_strdup (refresh_last);
}

void
mock_snapd_set_refresh_next (MockSnapd *snapd, const gchar *refresh_next)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_next);
    snapd->refresh_next = g_strdup (refresh_next);
}

void
mock_snapd_set_refresh_schedule (MockSnapd *snapd, const gchar *schedule)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_schedule);
    snapd->refresh_schedule = g_strdup (schedule);
}

void
mock_snapd_set_refresh_timer (MockSnapd *snapd, const gchar *timer)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_timer);
    snapd->refresh_timer = g_strdup (timer);
}

void
mock_snapd_set_spawn_time (MockSnapd *snapd, const gchar *spawn_time)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->spawn_time);
    snapd->spawn_time = g_strdup (spawn_time);
}

void
mock_snapd_set_ready_time (MockSnapd *snapd, const gchar *ready_time)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->ready_time);
    snapd->ready_time = g_strdup (ready_time);
}

static MockChange *
add_change (MockSnapd *snapd)
{
    MockChange *change = g_slice_new0 (MockChange);
    snapd->change_index++;
    change->id = g_strdup_printf ("%d", snapd->change_index);
    change->kind = g_strdup ("KIND");
    change->summary = g_strdup ("SUMMARY");
    change->task_index = snapd->change_index * 100;
    snapd->changes = g_list_append (snapd->changes, change);

    return change;
}

MockChange *
mock_snapd_add_change (MockSnapd *snapd)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    return add_change (snapd);
}

MockTask *
mock_change_add_task (MockChange *change, const gchar *kind)
{
    MockTask *task = g_slice_new0 (MockTask);
    task->id = g_strdup_printf ("%d", change->task_index);
    change->task_index++;
    task->kind = g_strdup (kind);
    task->summary = g_strdup ("SUMMARY");
    task->status = g_strdup ("Do");
    task->progress_label = g_strdup ("LABEL");
    task->progress_done = 0;
    task->progress_total = 1;
    change->tasks = g_list_append (change->tasks, task);

    return task;
}

void
mock_task_set_snap_name (MockTask *task, const gchar *snap_name)
{
    task->snap_name = g_strdup (snap_name);
}

void
mock_task_set_status (MockTask *task, const gchar *status)
{
    g_free (task->status);
    task->status = g_strdup (status);
}

void
mock_task_set_progress (MockTask *task, int done, int total)
{
    task->progress_done = done;
    task->progress_total = total;
}

void
mock_task_set_spawn_time (MockTask *task, const gchar *spawn_time)
{
    g_free (task->spawn_time);
    task->spawn_time = g_strdup (spawn_time);
}

void
mock_task_set_ready_time (MockTask *task, const gchar *ready_time)
{
    g_free (task->ready_time);
    task->ready_time = g_strdup (ready_time);
}

void
mock_change_set_spawn_time (MockChange *change, const gchar *spawn_time)
{
    g_free (change->spawn_time);
    change->spawn_time = g_strdup (spawn_time);
}

void
mock_change_set_ready_time (MockChange *change, const gchar *ready_time)
{
    g_free (change->ready_time);
    change->ready_time = g_strdup (ready_time);
}

static MockSnap *
mock_snap_new (const gchar *name)
{
    MockSnap *snap = g_slice_new0 (MockSnap);
    snap->configuration = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    snap->confinement = g_strdup ("strict");
    snap->publisher_display_name = g_strdup ("PUBLISHER-DISPLAY-NAME");
    snap->publisher_id = g_strdup ("PUBLISHER-ID");
    snap->publisher_username = g_strdup ("PUBLISHER-USERNAME");
    snap->icon = g_strdup ("ICON");
    snap->id = g_strdup ("ID");
    snap->name = g_strdup (name);
    snap->revision = g_strdup ("REVISION");
    snap->status = g_strdup ("active");
    snap->type = g_strdup ("app");
    snap->version = g_strdup ("VERSION");

    return snap;
}

static MockInterface *
mock_interface_new (const gchar *name)
{
    MockInterface *interface = g_slice_new0 (MockInterface);
    interface->name = g_strdup (name);
    interface->summary = g_strdup ("SUMMARY");
    interface->doc_url = g_strdup ("URL");

    return interface;
}

MockInterface *
mock_snapd_add_interface (MockSnapd *snapd, const gchar *name)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    MockInterface *interface = mock_interface_new (name);
    snapd->interfaces = g_list_append (snapd->interfaces, interface);

    return interface;
}

void
mock_interface_set_summary (MockInterface *interface, const gchar *summary)
{
    g_free (interface->summary);
    interface->summary = g_strdup (summary);
}

void
mock_interface_set_doc_url (MockInterface *interface, const gchar *url)
{
    g_free (interface->doc_url);
    interface->doc_url = g_strdup (url);
}

MockSnap *
mock_snapd_add_snap (MockSnapd *snapd, const gchar *name)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    MockSnap *snap = mock_snap_new (name);
    snapd->snaps = g_list_append (snapd->snaps, snap);

    return snap;
}

static MockSnap *
find_snap (MockSnapd *snapd, const gchar *name)
{
    for (GList *link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        if (strcmp (snap->name, name) == 0)
            return snap;
    }

    return NULL;
}

MockSnap *
mock_snapd_find_snap (MockSnapd *snapd, const gchar *name)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    return find_snap (snapd, name);
}

void
mock_snapd_add_store_section (MockSnapd *snapd, const gchar *name)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    snapd->store_sections = g_list_append (snapd->store_sections, g_strdup (name));
}

MockSnap *
mock_snapd_add_store_snap (MockSnapd *snapd, const gchar *name)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    MockSnap *snap = mock_snap_new (name);
    snap->download_size = 65535;
    snapd->store_snaps = g_list_append (snapd->store_snaps, snap);

    mock_snap_add_track (snap, "latest");

    return snap;
}

static MockSnap *
find_store_snap_by_name (MockSnapd *snapd, const gchar *name, const gchar *channel,  const gchar *revision)
{
    for (GList *link = snapd->store_snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        if (strcmp (snap->name, name) == 0 &&
            (channel == NULL || g_strcmp0 (snap->channel, channel) == 0) &&
            (revision == NULL || g_strcmp0 (snap->revision, revision) == 0))
            return snap;
    }

    return NULL;
}

MockApp *
mock_snap_add_app (MockSnap *snap, const gchar *name)
{
    MockApp *app = g_slice_new0 (MockApp);
    app->name = g_strdup (name);
    snap->apps = g_list_append (snap->apps, app);

    return app;
}

MockApp *
mock_snap_find_app (MockSnap *snap, const gchar *name)
{
    for (GList *link = snap->apps; link; link = link->next) {
        MockApp *app = link->data;
        if (strcmp (app->name, name) == 0)
            return app;
    }

    return NULL;
}

void
mock_app_set_active (MockApp *app, gboolean active)
{
    app->active = active;
}

void
mock_app_set_enabled (MockApp *app, gboolean enabled)
{
    app->enabled = enabled;
}

void
mock_app_set_common_id (MockApp *app, const gchar *id)
{
    g_free (app->common_id);
    app->common_id = g_strdup (id);
}

void
mock_app_set_daemon (MockApp *app, const gchar *daemon)
{
    g_free (app->daemon);
    app->daemon = g_strdup (daemon);
}

void
mock_app_set_desktop_file (MockApp *app, const gchar *desktop_file)
{
    g_free (app->desktop_file);
    app->desktop_file = g_strdup (desktop_file);
}

void
mock_snap_set_base (MockSnap *snap, const gchar *base)
{
    g_free (snap->base);
    snap->base = g_strdup (base);
}

void
mock_snap_set_broken (MockSnap *snap, const gchar *broken)
{
    g_free (snap->broken);
    snap->broken = g_strdup (broken);
}

void
mock_snap_set_channel (MockSnap *snap, const gchar *channel)
{
    g_free (snap->channel);
    snap->channel = g_strdup (channel);
}

const gchar *
mock_snap_get_channel (MockSnap *snap)
{
    return snap->channel;
}

void
mock_snap_set_conf (MockSnap *snap, const gchar *name, const gchar *value)
{
    g_hash_table_insert (snap->configuration, g_strdup (name), g_strdup (value));
}

gsize
mock_snap_get_conf_count (MockSnap *snap)
{
    return g_hash_table_size (snap->configuration);
}

const gchar *
mock_snap_get_conf (MockSnap *snap, const gchar *name)
{
    return g_hash_table_lookup (snap->configuration, name);
}

MockTrack *
mock_snap_add_track (MockSnap *snap, const gchar *name)
{
    for (GList *link = snap->tracks; link; link = link->next) {
        MockTrack *track = link->data;
        if (g_strcmp0 (track->name, name) == 0)
            return track;
    }

    MockTrack *track = g_slice_new0 (MockTrack);
    track->name = g_strdup (name);
    snap->tracks = g_list_append (snap->tracks, track);

    return track;
}

MockChannel *
mock_track_add_channel (MockTrack *track, const gchar *risk, const gchar *branch)
{
    MockChannel *channel = g_slice_new0 (MockChannel);
    channel->risk = g_strdup (risk);
    channel->branch = g_strdup (branch);
    channel->confinement = g_strdup ("strict");
    channel->epoch = g_strdup ("0");
    channel->revision = g_strdup ("REVISION");
    channel->size = 65535;
    channel->version = g_strdup ("VERSION");
    track->channels = g_list_append (track->channels, channel);

    return channel;
}

void
mock_channel_set_branch (MockChannel *channel, const gchar *branch)
{
    g_free (channel->branch);
    channel->branch = g_strdup (branch);
}

void
mock_channel_set_confinement (MockChannel *channel, const gchar *confinement)
{
    g_free (channel->confinement);
    channel->confinement = g_strdup (confinement);
}

void
mock_channel_set_epoch (MockChannel *channel, const gchar *epoch)
{
    g_free (channel->epoch);
    channel->epoch = g_strdup (epoch);
}

void
mock_channel_set_released_at (MockChannel *channel, const gchar *released_at)
{
    g_free (channel->released_at);
    channel->released_at = g_strdup (released_at);
}

void
mock_channel_set_revision (MockChannel *channel, const gchar *revision)
{
    g_free (channel->revision);
    channel->revision = g_strdup (revision);
}

const gchar *
mock_snap_get_revision (MockSnap *snap)
{
    return snap->revision;
}

void
mock_channel_set_size (MockChannel *channel, int size)
{
    channel->size = size;
}

void
mock_channel_set_version (MockChannel *channel, const gchar *version)
{
    g_free (channel->version);
    channel->version = g_strdup (version);
}

void
mock_snap_set_confinement (MockSnap *snap, const gchar *confinement)
{
    g_free (snap->confinement);
    snap->confinement = g_strdup (confinement);
}

const gchar *
mock_snap_get_confinement (MockSnap *snap)
{
    return snap->confinement;
}

void
mock_snap_set_contact (MockSnap *snap, const gchar *contact)
{
    g_free (snap->contact);
    snap->contact = g_strdup (contact);
}

gboolean
mock_snap_get_dangerous (MockSnap *snap)
{
    return snap->dangerous;
}

const gchar *
mock_snap_get_data (MockSnap *snap)
{
    return snap->snap_data;
}

void
mock_snap_set_description (MockSnap *snap, const gchar *description)
{
    g_free (snap->description);
    snap->description = g_strdup (description);
}

void
mock_snap_set_devmode (MockSnap *snap, gboolean devmode)
{
    snap->devmode = devmode;
}

gboolean
mock_snap_get_devmode (MockSnap *snap)
{
    return snap->devmode;
}

void
mock_snap_set_disabled (MockSnap *snap, gboolean disabled)
{
    snap->disabled = disabled;
}

gboolean
mock_snap_get_disabled (MockSnap *snap)
{
    return snap->disabled;
}

void
mock_snap_set_download_size (MockSnap *snap, int download_size)
{
    snap->download_size = download_size;
}

void
mock_snap_set_error (MockSnap *snap, const gchar *error)
{
    g_free (snap->error);
    snap->error = g_strdup (error);
}

void
mock_snap_set_icon (MockSnap *snap, const gchar *icon)
{
    g_free (snap->icon);
    snap->icon = g_strdup (icon);
}

void
mock_snap_set_icon_data (MockSnap *snap, const gchar *mime_type, GBytes *data)
{
    g_free (snap->icon_mime_type);
    snap->icon_mime_type = g_strdup (mime_type);
    g_bytes_unref (snap->icon_data);
    snap->icon_data = g_bytes_ref (data);
}

void
mock_snap_set_id (MockSnap *snap, const gchar *id)
{
    g_free (snap->id);
    snap->id = g_strdup (id);
}

void
mock_snap_set_install_date (MockSnap *snap, const gchar *install_date)
{
    g_free (snap->install_date);
    snap->install_date = g_strdup (install_date);
}

void
mock_snap_set_installed_size (MockSnap *snap, int installed_size)
{
    snap->installed_size = installed_size;
}

void
mock_snap_set_jailmode (MockSnap *snap, gboolean jailmode)
{
    snap->jailmode = jailmode;
}

gboolean
mock_snap_get_jailmode (MockSnap *snap)
{
    return snap->jailmode;
}

void
mock_snap_set_license (MockSnap *snap, const gchar *license)
{
    g_free (snap->license);
    snap->license = g_strdup (license);
}

void
mock_snap_set_mounted_from (MockSnap *snap, const gchar *mounted_from)
{
    g_free (snap->mounted_from);
    snap->mounted_from = g_strdup (mounted_from);
}

const gchar *
mock_snap_get_path (MockSnap *snap)
{
    return snap->snap_path;
}

gboolean
mock_snap_get_preferred (MockSnap *snap)
{
    return snap->preferred;
}

void
mock_snap_set_publisher_display_name (MockSnap *snap, const gchar *display_name)
{
    g_free (snap->publisher_display_name);
    snap->publisher_display_name = g_strdup (display_name);
}

void
mock_snap_set_publisher_id (MockSnap *snap, const gchar *id)
{
    g_free (snap->publisher_id);
    snap->publisher_id = g_strdup (id);
}

void
mock_snap_set_publisher_username (MockSnap *snap, const gchar *username)
{
    g_free (snap->publisher_username);
    snap->publisher_username = g_strdup (username);
}

void
mock_snap_set_publisher_validation (MockSnap *snap, const gchar *validation)
{
    g_free (snap->publisher_validation);
    snap->publisher_validation = g_strdup (validation);
}

void
mock_snap_set_restart_required (MockSnap *snap, gboolean restart_required)
{
    snap->restart_required = restart_required;
}

void
mock_snap_set_revision (MockSnap *snap, const gchar *revision)
{
    g_free (snap->revision);
    snap->revision = g_strdup (revision);
}

void
mock_snap_set_scope_is_wide (MockSnap *snap, gboolean scope_is_wide)
{
    snap->scope_is_wide = scope_is_wide;
}

MockMedia *
mock_snap_add_media (MockSnap *snap, const gchar *type, const gchar *url, int width, int height)
{
    MockMedia *media = g_slice_new0 (MockMedia);
    media->type = g_strdup (type);
    media->url = g_strdup (url);
    media->width = width;
    media->height = height;
    snap->media = g_list_append (snap->media, media);

    return media;
}

void
mock_snap_set_status (MockSnap *snap, const gchar *status)
{
    g_free (snap->status);
    snap->status = g_strdup (status);
}

void
mock_snap_set_summary (MockSnap *snap, const gchar *summary)
{
    g_free (snap->summary);
    snap->summary = g_strdup (summary);
}

void
mock_snap_set_title (MockSnap *snap, const gchar *title)
{
    g_free (snap->title);
    snap->title = g_strdup (title);
}

void
mock_snap_set_tracking_channel (MockSnap *snap, const gchar *channel)
{
    g_free (snap->tracking_channel);
    snap->tracking_channel = g_strdup (channel);
}

const gchar *
mock_snap_get_tracking_channel (MockSnap *snap)
{
    return snap->tracking_channel;
}

void
mock_snap_set_trymode (MockSnap *snap, gboolean trymode)
{
    snap->trymode = trymode;
}

void
mock_snap_set_type (MockSnap *snap, const gchar *type)
{
    g_free (snap->type);
    snap->type = g_strdup (type);
}

void
mock_snap_set_version (MockSnap *snap, const gchar *version)
{
    g_free (snap->version);
    snap->version = g_strdup (version);
}

void
mock_snap_add_store_section (MockSnap *snap, const gchar *name)
{
    snap->store_sections = g_list_append (snap->store_sections, g_strdup (name));
}

MockPlug *
mock_snap_add_plug (MockSnap *snap, MockInterface *interface, const gchar *name)
{
    MockPlug *plug = g_slice_new0 (MockPlug);
    plug->snap = snap;
    plug->name = g_strdup (name);
    plug->interface = interface;
    plug->attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    plug->label = g_strdup ("LABEL");
    snap->plugs = g_list_append (snap->plugs, plug);

    return plug;
}

MockPlug *
mock_snap_find_plug (MockSnap *snap, const gchar *name)
{
    for (GList *link = snap->plugs; link; link = link->next) {
        MockPlug *plug = link->data;
        if (strcmp (plug->name, name) == 0)
            return plug;
    }

    return NULL;
}

void
mock_plug_add_attribute (MockPlug *plug, const gchar *name, const gchar *value)
{
    g_hash_table_insert (plug->attributes, g_strdup (name), g_strdup (value));
}

MockSlot *
mock_snap_add_slot (MockSnap *snap, MockInterface *interface, const gchar *name)
{
    MockSlot *slot = g_slice_new0 (MockSlot);
    slot->snap = snap;
    slot->name = g_strdup (name);
    slot->interface = interface;
    slot->attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    slot->label = g_strdup ("LABEL");
    snap->slots_ = g_list_append (snap->slots_, slot);

    return slot;
}

MockSlot *
mock_snap_find_slot (MockSnap *snap, const gchar *name)
{
    for (GList *link = snap->slots_; link; link = link->next) {
        MockSlot *slot = link->data;
        if (strcmp (slot->name, name) == 0)
            return slot;
    }

    return NULL;
}

void
mock_slot_add_attribute (MockSlot *slot, const gchar *name, const gchar *value)
{
    g_hash_table_insert (slot->attributes, g_strdup (name), g_strdup (value));
}

void
mock_snapd_connect (MockSnapd *snapd, MockPlug *plug, MockSlot *slot, gboolean manual, gboolean gadget)
{
    g_return_if_fail (plug != NULL);

    if (slot != NULL) {
        /* Remove existing connections */
        g_autoptr(GList) old_established_connections = g_steal_pointer (&snapd->established_connections);
        for (GList *link = old_established_connections; link; link = link->next) {
            MockConnection *connection = link->data;
            if (connection->plug == plug)
                continue;
            snapd->established_connections = g_list_append (snapd->established_connections, connection);
        }
        g_autoptr(GList) old_undesired_connections = g_steal_pointer (&snapd->undesired_connections);
        for (GList *link = old_undesired_connections; link; link = link->next) {
            MockConnection *connection = link->data;
            if (connection->plug == plug)
                continue;
            snapd->undesired_connections = g_list_append (snapd->undesired_connections, connection);
        }

        MockConnection *connection = g_slice_new0 (MockConnection);
        connection->plug = plug;
        connection->slot = slot;
        connection->manual = manual;
        connection->gadget = gadget;
        snapd->established_connections = g_list_append (snapd->established_connections, connection);
    }
    else {
        /* Remove existing connections */
        MockSlot *autoconnected_slot = NULL;
        g_autoptr(GList) old_established_connections = g_steal_pointer (&snapd->established_connections);
        for (GList *link = old_established_connections; link; link = link->next) {
            MockConnection *connection = link->data;
            if (connection->plug == plug) {
                if (!connection->manual)
                    autoconnected_slot = connection->slot;
                mock_connection_free (connection);
                continue;
            }
            snapd->established_connections = g_list_append (snapd->established_connections, connection);
        }
        g_autoptr(GList) old_undesired_connections = g_steal_pointer (&snapd->undesired_connections);
        for (GList *link = old_undesired_connections; link; link = link->next) {
            MockConnection *connection = link->data;
            if (connection->plug == plug)
                continue;
            snapd->undesired_connections = g_list_append (snapd->undesired_connections, connection);
        }

        /* Mark as undesired if overriding auto connection */
        if (autoconnected_slot != NULL) {
            MockConnection *connection = g_slice_new0 (MockConnection);
            connection->plug = plug;
            connection->slot = autoconnected_slot;
            connection->manual = manual;
            connection->gadget = gadget;
            snapd->undesired_connections = g_list_append (snapd->undesired_connections, connection);
        }
    }
}

MockSlot *
mock_snapd_find_plug_connection (MockSnapd *snapd, MockPlug *plug)
{
    for (GList *link = snapd->established_connections; link; link = link->next) {
        MockConnection *connection = link->data;
        if (connection->plug == plug)
            return connection->slot;
    }

    return NULL;
}

GList *
mock_snapd_find_slot_connections (MockSnapd *snapd, MockSlot *slot)
{
    GList *plugs = NULL;
    for (GList *link = snapd->established_connections; link; link = link->next) {
        MockConnection *connection = link->data;
        if (connection->slot == slot)
            plugs = g_list_append (plugs, connection->plug);
    }

    return plugs;
}

const gchar *
mock_snapd_get_last_user_agent (MockSnapd *snapd)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    if (snapd->last_request_headers == NULL)
        return NULL;

    return soup_message_headers_get_one (snapd->last_request_headers, "User-Agent");
}

const gchar *
mock_snapd_get_last_accept_language (MockSnapd *snapd)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    if (snapd->last_request_headers == NULL)
        return NULL;

    return soup_message_headers_get_one (snapd->last_request_headers, "Accept-Language");
}

const gchar *
mock_snapd_get_last_allow_interaction (MockSnapd *snapd)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    if (snapd->last_request_headers == NULL)
        return NULL;

    return soup_message_headers_get_one (snapd->last_request_headers, "X-Allow-Interaction");
}

static MockChange *
get_change (MockSnapd *snapd, const gchar *id)
{
    for (GList *link = snapd->changes; link; link = link->next) {
        MockChange *change = link->data;
        if (strcmp (change->id, id) == 0)
            return change;
    }

    return NULL;
}

static void
mock_task_free (MockTask *task)
{
    g_free (task->id);
    g_free (task->kind);
    g_free (task->summary);
    g_free (task->status);
    g_free (task->progress_label);
    g_free (task->spawn_time);
    g_free (task->ready_time);
    if (task->snap != NULL)
        mock_snap_free (task->snap);
    g_free (task->snap_name);
    g_free (task->error);
    g_slice_free (MockTask, task);
}

static void
mock_change_free (MockChange *change)
{
    g_free (change->id);
    g_free (change->kind);
    g_free (change->summary);
    g_free (change->spawn_time);
    g_free (change->ready_time);
    g_list_free_full (change->tasks, (GDestroyNotify) mock_task_free);
    if (change->data != NULL)
        json_node_unref (change->data);
    g_slice_free (MockChange, change);
}

static void
send_response (SoupMessage *message, guint status_code, const gchar *content_type, const guint8 *content, gsize content_length)
{
    soup_message_set_status (message, status_code);
    soup_message_headers_set_content_type (message->response_headers,
                                           content_type, NULL);
    soup_message_headers_set_content_length (message->response_headers,
                                             content_length);

    soup_message_body_append (message->response_body, SOUP_MEMORY_COPY,
                              content, content_length);
}

static void
send_json_response (SoupMessage *message, guint status_code, JsonNode *node)
{
    g_autoptr(JsonGenerator) generator = json_generator_new ();
    json_generator_set_root (generator, node);
    gsize data_length;
    g_autofree gchar *data = json_generator_to_data (generator, &data_length);

    send_response (message, status_code, "application/json", (guint8*) data, data_length);
}

static JsonNode *
make_response (MockSnapd *snapd, const gchar *type, guint status_code, JsonNode *result, const gchar *change_id) // FIXME: sources
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, type);
    json_builder_set_member_name (builder, "status-code");
    json_builder_add_int_value (builder, status_code);
    json_builder_set_member_name (builder, "status");
    json_builder_add_string_value (builder, soup_status_get_phrase (status_code));
    json_builder_set_member_name (builder, "result");
    if (result != NULL)
        json_builder_add_value (builder, result);
     else
        json_builder_add_null_value (builder); // FIXME: Snapd sets it to null, but it might be a bug?
    if (change_id != NULL) {
        json_builder_set_member_name (builder, "change");
        json_builder_add_string_value (builder, change_id);
    }
    if (snapd->maintenance_kind != NULL) {
        json_builder_set_member_name (builder, "maintenance");
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "kind");
        json_builder_add_string_value (builder, snapd->maintenance_kind);
        json_builder_set_member_name (builder, "message");
        json_builder_add_string_value (builder, snapd->maintenance_message);
        json_builder_end_object (builder);
    }
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static void
send_sync_response (MockSnapd *snapd, SoupMessage *message, guint status_code, JsonNode *result)
{
    g_autoptr(JsonNode) response = make_response (snapd, "sync", status_code, result, NULL);
    send_json_response (message, status_code, response);
}

static void
send_async_response (MockSnapd *snapd, SoupMessage *message, guint status_code, const gchar *change_id)
{
    g_autoptr(JsonNode) response = make_response (snapd, "async", status_code, NULL, change_id);
    send_json_response (message, status_code, response);
}

static void
send_error_response (MockSnapd *snapd, SoupMessage *message, guint status_code, const gchar *error_message, const gchar *kind)
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "message");
    json_builder_add_string_value (builder, error_message);
    if (kind != NULL) {
        json_builder_set_member_name (builder, "kind");
        json_builder_add_string_value (builder, kind);
    }
    json_builder_end_object (builder);

    g_autoptr(JsonNode) response = make_response (snapd, "error", status_code, json_builder_get_root (builder), NULL);
    send_json_response (message, status_code, response);
}

static void
send_error_bad_request (MockSnapd *snapd, SoupMessage *message, const gchar *error_message, const gchar *kind)
{
    send_error_response (snapd, message, 400, error_message, kind);
}

static void
send_error_forbidden (MockSnapd *snapd, SoupMessage *message, const gchar *error_message, const gchar *kind)
{
    send_error_response (snapd, message, 403, error_message, kind);
}

static void
send_error_not_found (MockSnapd *snapd, SoupMessage *message, const gchar *error_message, const gchar *kind)
{
    send_error_response (snapd, message, 404, error_message, kind);
}

static void
send_error_method_not_allowed (MockSnapd *snapd, SoupMessage *message, const gchar *error_message)
{
    send_error_response (snapd, message, 405, error_message, NULL);
}

static void
handle_system_info (MockSnapd *snapd, SoupMessage *message)
{
    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    if (snapd->build_id) {
        json_builder_set_member_name (builder, "build-id");
        json_builder_add_string_value (builder, snapd->build_id);
    }
    if (snapd->confinement) {
        json_builder_set_member_name (builder, "confinement");
        json_builder_add_string_value (builder, snapd->confinement);
    }
    json_builder_set_member_name (builder, "os-release");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "id");
    json_builder_add_string_value (builder, "OS-ID");
    json_builder_set_member_name (builder, "version-id");
    json_builder_add_string_value (builder, "OS-VERSION");
    json_builder_end_object (builder);
    json_builder_set_member_name (builder, "series");
    json_builder_add_string_value (builder, "SERIES");
    json_builder_set_member_name (builder, "version");
    json_builder_add_string_value (builder, "VERSION");
    json_builder_set_member_name (builder, "managed");
    json_builder_add_boolean_value (builder, snapd->managed);
    json_builder_set_member_name (builder, "on-classic");
    json_builder_add_boolean_value (builder, snapd->on_classic);
    json_builder_set_member_name (builder, "kernel-version");
    json_builder_add_string_value (builder, "KERNEL-VERSION");
    json_builder_set_member_name (builder, "locations");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap-mount-dir");
    json_builder_add_string_value (builder, "/snap");
    json_builder_set_member_name (builder, "snap-bin-dir");
    json_builder_add_string_value (builder, "/snap/bin");
    json_builder_end_object (builder);
    if (g_hash_table_size (snapd->sandbox_features) > 0) {
        json_builder_set_member_name (builder, "sandbox-features");
        json_builder_begin_object (builder);
        GHashTableIter iter;
        gpointer key, value;
        g_hash_table_iter_init (&iter, snapd->sandbox_features);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
            const gchar *backend = key;
            GPtrArray *backend_features = value;

            json_builder_set_member_name (builder, backend);
            json_builder_begin_array (builder);
            for (guint i = 0; i < backend_features->len; i++)
                json_builder_add_string_value (builder, g_ptr_array_index (backend_features, i));
            json_builder_end_array (builder);
        }
        json_builder_end_object (builder);
    }
    if (snapd->store) {
        json_builder_set_member_name (builder, "store");
        json_builder_add_string_value (builder, snapd->store);
    }
    json_builder_set_member_name (builder, "refresh");
    json_builder_begin_object (builder);
    if (snapd->refresh_timer) {
        json_builder_set_member_name (builder, "timer");
        json_builder_add_string_value (builder, snapd->refresh_timer);
    }
    else if (snapd->refresh_schedule) {
        json_builder_set_member_name (builder, "schedule");
        json_builder_add_string_value (builder, snapd->refresh_schedule);
    }
    if (snapd->refresh_last) {
        json_builder_set_member_name (builder, "last");
        json_builder_add_string_value (builder, snapd->refresh_last);
    }
    if (snapd->refresh_next) {
        json_builder_set_member_name (builder, "next");
        json_builder_add_string_value (builder, snapd->refresh_next);
    }
    if (snapd->refresh_hold) {
        json_builder_set_member_name (builder, "hold");
        json_builder_add_string_value (builder, snapd->refresh_hold);
    }
    json_builder_end_object (builder);
    json_builder_end_object (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder));
}

static JsonNode *
get_json (SoupMessage *message)
{
    const gchar *content_type = soup_message_headers_get_content_type (message->request_headers, NULL);
    if (content_type == NULL)
        return NULL;

    g_autoptr(JsonParser) parser = json_parser_new ();
    g_autoptr(GError) error = NULL;
    if (!json_parser_load_from_data (parser, message->request_body->data, message->request_body->length, &error)) {
        g_warning ("Failed to parse request: %s", error->message);
        return NULL;
    }

    return json_node_ref (json_parser_get_root (parser));
}

static JsonNode *
make_app_node (MockApp *app, const gchar *snap_name)
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    if (snap_name != NULL) {
        json_builder_set_member_name (builder, "snap");
        json_builder_add_string_value (builder, snap_name);
    }
    if (app->name != NULL) {
        json_builder_set_member_name (builder, "name");
        json_builder_add_string_value (builder, app->name);
    }
    if (app->common_id != NULL) {
        json_builder_set_member_name (builder, "common-id");
        json_builder_add_string_value (builder, app->common_id);
    }
    if (app->daemon != NULL) {
        json_builder_set_member_name (builder, "daemon");
        json_builder_add_string_value (builder, app->daemon);
    }
    if (app->desktop_file != NULL) {
        json_builder_set_member_name (builder, "desktop-file");
        json_builder_add_string_value (builder, app->desktop_file);
    }
    if (app->active) {
        json_builder_set_member_name (builder, "active");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if (app->enabled) {
        json_builder_set_member_name (builder, "enabled");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static JsonNode *
make_snap_node (MockSnap *snap)
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    if (snap->apps != NULL) {
        json_builder_set_member_name (builder, "apps");
        json_builder_begin_array (builder);
        for (GList *link = snap->apps; link; link = link->next) {
            MockApp *app = link->data;
            json_builder_add_value (builder, make_app_node (app, NULL));
        }
        json_builder_end_array (builder);
    }
    if (snap->broken) {
        json_builder_set_member_name (builder, "broken");
        json_builder_add_string_value (builder, snap->broken);
    }
    if (snap->base) {
        json_builder_set_member_name (builder, "base");
        json_builder_add_string_value (builder, snap->base);
    }
    if (snap->channel) {
        json_builder_set_member_name (builder, "channel");
        json_builder_add_string_value (builder, snap->channel);
    }
    if (snap->tracks != NULL) {
        json_builder_set_member_name (builder, "channels");
        json_builder_begin_object (builder);
        for (GList *link = snap->tracks; link; link = link->next) {
            MockTrack *track = link->data;

            for (GList *channel_link = track->channels; channel_link; channel_link = channel_link->next) {
                MockChannel *channel = channel_link->data;

                g_autofree gchar *key = NULL;
                if (channel->branch != NULL)
                    key = g_strdup_printf ("%s/%s/%s", track->name, channel->risk, channel->branch);
                else
                    key = g_strdup_printf ("%s/%s", track->name, channel->risk);

                g_autofree gchar *name = NULL;
                if (g_strcmp0 (track->name, "latest") == 0) {
                    if (channel->branch != NULL)
                        name = g_strdup_printf ("%s/%s", channel->risk, channel->branch);
                    else
                        name = g_strdup (channel->risk);
                }
                else
                    name = g_strdup (key);

                json_builder_set_member_name (builder, key);
                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "revision");
                json_builder_add_string_value (builder, channel->revision);
                json_builder_set_member_name (builder, "confinement");
                json_builder_add_string_value (builder, channel->confinement);
                json_builder_set_member_name (builder, "version");
                json_builder_add_string_value (builder, channel->version);
                json_builder_set_member_name (builder, "channel");
                json_builder_add_string_value (builder, name);
                json_builder_set_member_name (builder, "epoch");
                json_builder_add_string_value (builder, channel->epoch);
                json_builder_set_member_name (builder, "size");
                json_builder_add_int_value (builder, channel->size);
                if (channel->released_at != NULL) {
                    json_builder_set_member_name (builder, "released-at");
                    json_builder_add_string_value (builder, channel->released_at);
                }
                json_builder_end_object (builder);
            }
        }
        json_builder_end_object (builder);
    }
    if (snap->apps != NULL) {
        int id_count = 0;
        for (GList *link = snap->apps; link; link = link->next) {
            MockApp *app = link->data;
            if (app->common_id == NULL)
                continue;
            if (id_count == 0) {
                json_builder_set_member_name (builder, "common-ids");
                json_builder_begin_array (builder);
            }
            id_count++;
            json_builder_add_string_value (builder, app->common_id);
        }
        if (id_count > 0)
            json_builder_end_array (builder);
    }
    json_builder_set_member_name (builder, "confinement");
    json_builder_add_string_value (builder, snap->confinement);
    if (snap->contact) {
        json_builder_set_member_name (builder, "contact");
        json_builder_add_string_value (builder, snap->contact);
    }
    if (snap->description) {
        json_builder_set_member_name (builder, "description");
        json_builder_add_string_value (builder, snap->description);
    }
    json_builder_set_member_name (builder, "developer");
    json_builder_add_string_value (builder, snap->publisher_username);
    json_builder_set_member_name (builder, "devmode");
    json_builder_add_boolean_value (builder, snap->devmode);
    if (snap->download_size > 0) {
        json_builder_set_member_name (builder, "download-size");
        json_builder_add_int_value (builder, snap->download_size);
    }
    json_builder_set_member_name (builder, "icon");
    json_builder_add_string_value (builder, snap->icon);
    json_builder_set_member_name (builder, "id");
    json_builder_add_string_value (builder, snap->id);
    if (snap->install_date != NULL) {
        json_builder_set_member_name (builder, "install-date");
        json_builder_add_string_value (builder, snap->install_date);
    }
    if (snap->installed_size > 0) {
        json_builder_set_member_name (builder, "installed-size");
        json_builder_add_int_value (builder, snap->installed_size);
    }
    json_builder_set_member_name (builder, "jailmode");
    json_builder_add_boolean_value (builder, snap->jailmode);
    if (snap->license) {
        json_builder_set_member_name (builder, "license");
        json_builder_add_string_value (builder, snap->license);
    }
    if (snap->mounted_from) {
        json_builder_set_member_name (builder, "mounted-from");
        json_builder_add_string_value (builder, snap->mounted_from);
    }
    if (snap->media != NULL) {
        json_builder_set_member_name (builder, "media");
        json_builder_begin_array (builder);
        for (GList *link = snap->media; link; link = link->next) {
            MockMedia *media = link->data;

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "type");
            json_builder_add_string_value (builder, media->type);
            json_builder_set_member_name (builder, "url");
            json_builder_add_string_value (builder, media->url);
            if (media->width > 0 && media->height > 0) {
                json_builder_set_member_name (builder, "width");
                json_builder_add_int_value (builder, media->width);
                json_builder_set_member_name (builder, "height");
                json_builder_add_int_value (builder, media->height);
            }
            json_builder_end_object (builder);
        }
        json_builder_end_array (builder);
    }
    json_builder_set_member_name (builder, "name");
    json_builder_add_string_value (builder, snap->name);
    if (snap->is_private) {
        json_builder_set_member_name (builder, "private");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_set_member_name (builder, "publisher");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "id");
    json_builder_add_string_value (builder, snap->publisher_id);
    json_builder_set_member_name (builder, "username");
    json_builder_add_string_value (builder, snap->publisher_username);
    json_builder_set_member_name (builder, "display-name");
    json_builder_add_string_value (builder, snap->publisher_display_name);
    if (snap->publisher_validation != NULL) {
        json_builder_set_member_name (builder, "validation");
        json_builder_add_string_value (builder, snap->publisher_validation);
    }
    json_builder_end_object (builder);
    json_builder_set_member_name (builder, "resource");
    g_autofree gchar *resource = g_strdup_printf ("/v2/snaps/%s", snap->name);
    json_builder_add_string_value (builder, resource);
    json_builder_set_member_name (builder, "revision");
    json_builder_add_string_value (builder, snap->revision);
    json_builder_set_member_name (builder, "status");
    json_builder_add_string_value (builder, snap->status);
    if (snap->summary) {
        json_builder_set_member_name (builder, "summary");
        json_builder_add_string_value (builder, snap->summary);
    }
    if (snap->title) {
        json_builder_set_member_name (builder, "title");
        json_builder_add_string_value (builder, snap->title);
    }
    if (snap->tracking_channel) {
        json_builder_set_member_name (builder, "tracking-channel");
        json_builder_add_string_value (builder, snap->tracking_channel);
    }
    if (snap->tracks) {
        json_builder_set_member_name (builder, "tracks");
        json_builder_begin_array (builder);
        for (GList *link = snap->tracks; link; link = link->next) {
            MockTrack *track = link->data;
            json_builder_add_string_value (builder, track->name);
        }
        json_builder_end_array (builder);
    }
    json_builder_set_member_name (builder, "trymode");
    json_builder_add_boolean_value (builder, snap->trymode);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, snap->type);
    json_builder_set_member_name (builder, "version");
    json_builder_add_string_value (builder, snap->version);
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static GList *
get_refreshable_snaps (MockSnapd *snapd)
{
    g_autoptr(GList) refreshable_snaps = NULL;
    for (GList *link = snapd->store_snaps; link; link = link->next) {
        MockSnap *store_snap = link->data;
        MockSnap *snap;

        snap = find_snap (snapd, store_snap->name);
        if (snap != NULL && strcmp (store_snap->revision, snap->revision) > 0)
            refreshable_snaps = g_list_append (refreshable_snaps, store_snap);
    }

    return g_steal_pointer (&refreshable_snaps);
}

static gboolean
filter_snaps (GStrv selected_snaps, MockSnap *snap)
{
    /* If no filter selected, then return all snaps */
    if (selected_snaps == NULL || selected_snaps[0] == NULL)
        return TRUE;

    for (int i = 0; selected_snaps[i] != NULL; i++) {
        if (strcmp (selected_snaps[i], snap->name) == 0)
            return TRUE;
    }

    return FALSE;
}

static void
handle_snaps (MockSnapd *snapd, SoupMessage *message, GHashTable *query)
{
    const gchar *content_type = soup_message_headers_get_content_type (message->request_headers, NULL);

    if (strcmp (message->method, "GET") == 0) {
        const gchar *select_param = NULL;
        g_auto(GStrv) selected_snaps = NULL;
        if (query != NULL) {
            const gchar *snaps_param = NULL;

            select_param = g_hash_table_lookup (query, "select");
            snaps_param = g_hash_table_lookup (query, "snaps");
            if (snaps_param != NULL)
                selected_snaps = g_strsplit (snaps_param, ",", -1);
        }

        g_autoptr(JsonBuilder) builder = json_builder_new ();
        json_builder_begin_array (builder);
        for (GList *link = snapd->snaps; link; link = link->next) {
            MockSnap *snap = link->data;

            if (!filter_snaps (selected_snaps, snap))
                continue;

            if ((select_param == NULL || strcmp (select_param, "enabled") == 0) && strcmp (snap->status, "active") != 0)
                continue;

            json_builder_add_value (builder, make_snap_node (snap));
        }
        json_builder_end_array (builder);

        send_sync_response (snapd, message, 200, json_builder_get_root (builder));
    }
    else if (strcmp (message->method, "POST") == 0 && g_strcmp0 (content_type, "application/json") == 0) {
        g_autoptr(JsonNode) request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "unknown content type", NULL);
            return;
        }

        JsonObject *o = json_node_get_object (request);
        const gchar *action = json_object_get_string_member (o, "action");
        if (strcmp (action, "refresh") == 0) {
            g_autoptr(GList) refreshable_snaps = get_refreshable_snaps (snapd);

            g_autoptr(JsonBuilder) builder = json_builder_new ();
            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "snap-names");
            json_builder_begin_array (builder);
            for (GList *link = refreshable_snaps; link; link = link->next) {
                MockSnap *snap = link->data;
                json_builder_add_string_value (builder, snap->name);
            }
            json_builder_end_array (builder);
            json_builder_end_object (builder);

            MockChange *change = add_change (snapd);
            change->data = json_builder_get_root (builder);
            mock_change_add_task (change, "refresh");
            send_async_response (snapd, message, 202, change->id);
        }
        else {
            send_error_bad_request (snapd, message, "unsupported multi-snap operation", NULL);
            return;
        }
    }
    else if (strcmp (message->method, "POST") == 0 && g_str_has_prefix (content_type, "multipart/")) {
        g_autoptr(SoupMultipart) multipart = soup_multipart_new_from_message (message->request_headers, message->request_body);
        if (multipart == NULL) {
            send_error_bad_request (snapd, message, "cannot read POST form", NULL);
            return;
        }

        gboolean classic = FALSE, dangerous = FALSE, devmode = FALSE, jailmode = FALSE;
        g_autofree gchar *action = NULL;
        g_autofree gchar *snap = NULL;
        g_autofree gchar *snap_path = NULL;
        for (int i = 0; i < soup_multipart_get_length (multipart); i++) {
            SoupMessageHeaders *part_headers;
            SoupBuffer *part_body;
            g_autofree gchar *disposition = NULL;
            g_autoptr(GHashTable) params = NULL;

            if (!soup_multipart_get_part (multipart, i, &part_headers, &part_body))
                continue;
            if (!soup_message_headers_get_content_disposition (part_headers, &disposition, &params))
                continue;

            if (strcmp (disposition, "form-data") == 0) {
                const gchar *name = g_hash_table_lookup (params, "name");

                if (g_strcmp0 (name, "action") == 0)
                    action = g_strndup (part_body->data, part_body->length);
                else if (g_strcmp0 (name, "classic") == 0)
                    classic = strncmp (part_body->data, "true", part_body->length) == 0;
                else if (g_strcmp0 (name, "dangerous") == 0)
                    dangerous = strncmp (part_body->data, "true", part_body->length) == 0;
                else if (g_strcmp0 (name, "devmode") == 0)
                    devmode = strncmp (part_body->data, "true", part_body->length) == 0;
                else if (g_strcmp0 (name, "jailmode") == 0)
                    jailmode = strncmp (part_body->data, "true", part_body->length) == 0;
                else if (g_strcmp0 (name, "snap") == 0)
                    snap = g_strndup (part_body->data, part_body->length);
                else if (g_strcmp0 (name, "snap-path") == 0)
                    snap_path = g_strndup (part_body->data, part_body->length);
            }
        }

        if (g_strcmp0 (action, "try") == 0) {
            if (snap_path == NULL) {
                send_error_bad_request (snapd, message, "need 'snap-path' value in form", NULL);
                return;
            }

            if (strcmp (snap_path, "*") == 0) {
                send_error_bad_request (snapd, message, "directory does not contain an unpacked snap", "snap-not-a-snap");
                return;
            }

            MockChange *change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            MockTask *task = mock_change_add_task (change, "try");
            task->snap = mock_snap_new ("try");
            task->snap->trymode = TRUE;
            task->snap->snap_path = g_steal_pointer (&snap_path);

            send_async_response (snapd, message, 202, change->id);
        }
        else {
            if (snap == NULL) {
                send_error_bad_request (snapd, message, "cannot find \"snap\" file field in provided multipart/form-data payload", NULL);
                return;
            }

            MockChange *change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            MockTask *task = mock_change_add_task (change, "install");
            task->snap = mock_snap_new ("sideload");
            if (classic)
                mock_snap_set_confinement (task->snap, "classic");
            task->snap->dangerous = dangerous;
            task->snap->devmode = devmode; // FIXME: Should set confinement to devmode?
            task->snap->jailmode = jailmode;
            g_free (task->snap->snap_data);
            task->snap->snap_data = g_steal_pointer (&snap);

            send_async_response (snapd, message, 202, change->id);
        }
    }
    else {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }
}

static void
handle_snap (MockSnapd *snapd, SoupMessage *message, const gchar *name)
{
    if (strcmp (message->method, "GET") == 0) {
        MockSnap *snap = find_snap (snapd, name);
        if (snap != NULL)
            send_sync_response (snapd, message, 200, make_snap_node (snap));
        else
            send_error_not_found (snapd, message, "cannot find snap", NULL);
    }
    else if (strcmp (message->method, "POST") == 0) {
        g_autoptr(JsonNode) request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "unknown content type", NULL);
            return;
        }

        JsonObject *o = json_node_get_object (request);
        const gchar *action = json_object_get_string_member (o, "action");
        const gchar *channel = NULL, *revision = NULL;
        gboolean classic = FALSE, dangerous = FALSE, devmode = FALSE, jailmode = FALSE;
        if (json_object_has_member (o, "channel"))
            channel = json_object_get_string_member (o, "channel");
        if (json_object_has_member (o, "revision"))
            revision = json_object_get_string_member (o, "revision");
        if (json_object_has_member (o, "classic"))
            classic = json_object_get_boolean_member (o, "classic");
        if (json_object_has_member (o, "dangerous"))
            dangerous = json_object_get_boolean_member (o, "dangerous");
        if (json_object_has_member (o, "devmode"))
            devmode = json_object_get_boolean_member (o, "devmode");
        if (json_object_has_member (o, "jailmode"))
            jailmode = json_object_get_boolean_member (o, "jailmode");

        if (strcmp (action, "install") == 0) {
            if (snapd->decline_auth) {
                send_error_forbidden (snapd, message, "cancelled", "auth-cancelled");
                return;
            }

            MockSnap *snap = find_snap (snapd, name);
            if (snap != NULL) {
                send_error_bad_request (snapd, message, "snap is already installed", "snap-already-installed");
                return;
            }

            snap = find_store_snap_by_name (snapd, name, NULL, NULL);
            if (snap == NULL) {
                send_error_not_found (snapd, message, "cannot install, snap not found", "snap-not-found");
                return;
            }

            snap = find_store_snap_by_name (snapd, name, channel, NULL);
            if (snap == NULL) {
                send_error_not_found (snapd, message, "no snap revision on specified channel", "snap-channel-not-available");
                return;
            }

            snap = find_store_snap_by_name (snapd, name, channel, revision);
            if (snap == NULL) {
                send_error_not_found (snapd, message, "no snap revision available as specified", "snap-revision-not-available");
                return;
            }

            if (strcmp (snap->confinement, "classic") == 0 && !classic) {
                send_error_bad_request (snapd, message, "requires classic confinement", "snap-needs-classic");
                return;
            }
            if (strcmp (snap->confinement, "classic") == 0 && !snapd->on_classic) {
                send_error_bad_request (snapd, message, "requires classic confinement which is only available on classic systems", "snap-needs-classic-system");
                return;
            }
            if (classic && strcmp (snap->confinement, "classic") != 0) {
                send_error_bad_request (snapd, message, "snap not compatible with --classic", "snap-not-classic");
                return;
            }
            if (strcmp (snap->confinement, "devmode") == 0 && !devmode) {
                send_error_bad_request (snapd, message, "requires devmode or confinement override", "snap-needs-devmode");
                return;
            }

            MockChange *change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            MockTask *task = mock_change_add_task (change, "install");
            task->snap = mock_snap_new (name);
            mock_snap_set_confinement (task->snap, snap->confinement);
            mock_snap_set_channel (task->snap, snap->channel);
            mock_snap_set_revision (task->snap, snap->revision);
            task->snap->devmode = devmode;
            task->snap->jailmode = jailmode;
            task->snap->dangerous = dangerous;
            if (snap->error != NULL)
                task->error = g_strdup (snap->error);

            send_async_response (snapd, message, 202, change->id);
        }
        else if (strcmp (action, "refresh") == 0) {
            MockSnap *snap = find_snap (snapd, name);
            if (snap != NULL) {
                /* Find if we have a store snap with a newer revision */
                MockSnap *store_snap = find_store_snap_by_name (snapd, name, channel, NULL);
                if (store_snap == NULL) {
                    send_error_bad_request (snapd, message, "cannot perform operation on local snap", "snap-local");
                }
                else if (strcmp (store_snap->revision, snap->revision) > 0) {
                    mock_snap_set_channel (snap, channel);

                    MockChange *change = add_change (snapd);
                    mock_change_set_spawn_time (change, snapd->spawn_time);
                    mock_change_set_ready_time (change, snapd->ready_time);
                    mock_change_add_task (change, "refresh");
                    send_async_response (snapd, message, 202, change->id);
                }
                else
                    send_error_bad_request (snapd, message, "snap has no updates available", "snap-no-update-available");
            }
            else
                send_error_bad_request (snapd, message, "cannot refresh: cannot find snap", "snap-not-installed");
        }
        else if (strcmp (action, "remove") == 0) {
            if (snapd->decline_auth) {
                send_error_forbidden (snapd, message, "cancelled", "auth-cancelled");
                return;
            }

            MockSnap *snap = find_snap (snapd, name);
            if (snap != NULL) {
                MockChange *change = add_change (snapd);
                mock_change_set_spawn_time (change, snapd->spawn_time);
                mock_change_set_ready_time (change, snapd->ready_time);
                MockTask *task = mock_change_add_task (change, "remove");
                mock_task_set_snap_name (task, name);
                if (snap->error != NULL)
                    task->error = g_strdup (snap->error);

                send_async_response (snapd, message, 202, change->id);
            }
            else
                send_error_bad_request (snapd, message, "snap is not installed", "snap-not-installed");
        }
        else if (strcmp (action, "enable") == 0) {
            MockSnap *snap = find_snap (snapd, name);
            if (snap != NULL) {
                if (!snap->disabled) {
                    send_error_bad_request (snapd, message, "cannot enable: snap is already enabled", NULL);
                    return;
                }
                snap->disabled = FALSE;

                MockChange *change = add_change (snapd);
                mock_change_add_task (change, "enable");
                send_async_response (snapd, message, 202, change->id);
            }
            else
                send_error_bad_request (snapd, message, "cannot enable: cannot find snap", "snap-not-installed");
        }
        else if (strcmp (action, "disable") == 0) {
            MockSnap *snap = find_snap (snapd, name);
            if (snap != NULL) {
                if (snap->disabled) {
                    send_error_bad_request (snapd, message, "cannot disable: snap is already disabled", NULL);
                    return;
                }
                snap->disabled = TRUE;

                MockChange *change = add_change (snapd);
                mock_change_set_spawn_time (change, snapd->spawn_time);
                mock_change_set_ready_time (change, snapd->ready_time);
                mock_change_add_task (change, "disable");
                send_async_response (snapd, message, 202, change->id);
            }
            else
                send_error_bad_request (snapd, message, "cannot disable: cannot find snap", "snap-not-installed");
        }
        else if (strcmp (action, "switch") == 0) {
            MockSnap *snap = find_snap (snapd, name);
            if (snap != NULL) {
                mock_snap_set_tracking_channel (snap, channel);

                MockChange *change = add_change (snapd);
                mock_change_set_spawn_time (change, snapd->spawn_time);
                mock_change_set_ready_time (change, snapd->ready_time);
                mock_change_add_task (change, "switch");
                send_async_response (snapd, message, 202, change->id);
            }
            else
                send_error_bad_request (snapd, message, "cannot switch: cannot find snap", "snap-not-installed");
        }
        else
            send_error_bad_request (snapd, message, "unknown action", NULL);
    }
    else
        send_error_method_not_allowed (snapd, message, "method not allowed");
}

static gint
compare_keys (gconstpointer a, gconstpointer b)
{
    gchar *name_a = *((gchar **) a);
    gchar *name_b = *((gchar **) b);
    return strcmp (name_a, name_b);
}

static void
handle_snap_conf (MockSnapd *snapd, SoupMessage *message, const gchar *name, GHashTable *query)
{
    if (strcmp (message->method, "GET") == 0) {
        MockSnap *snap;
        if (strcmp (name, "system") == 0)
            snap = find_snap (snapd, "core");
        else
            snap = find_snap (snapd, name);

        g_autoptr(GPtrArray) keys = g_ptr_array_new_with_free_func (g_free);
        const gchar *keys_param = NULL;
        if (query != NULL)
            keys_param = g_hash_table_lookup (query, "keys");
        if (keys_param != NULL) {
            g_auto(GStrv) key_names = g_strsplit (keys_param, ",", -1);
            for (int i = 0; key_names[i] != NULL; i++)
                g_ptr_array_add (keys, g_strdup (g_strstrip (key_names[i])));
        }
        if (keys->len == 0 && snap != NULL) {
            g_autofree GStrv key_names = (GStrv) g_hash_table_get_keys_as_array (snap->configuration, NULL);
            for (int i = 0; key_names[i] != NULL; i++)
                g_ptr_array_add (keys, g_strdup (g_strstrip (key_names[i])));
        }
        g_ptr_array_sort (keys, compare_keys);

        g_autoptr(JsonBuilder) builder = json_builder_new ();
        json_builder_begin_object (builder);
        for (guint i = 0; i < keys->len; i++) {
            const gchar *key = g_ptr_array_index (keys, i);

            const gchar *value = NULL;
            if (snap != NULL)
                value = g_hash_table_lookup (snap->configuration, key);

            if (value == NULL) {
                g_autofree gchar *error_message = g_strdup_printf ("snap \"%s\" has no \"%s\" configuration option", name, key);
                send_error_bad_request (snapd, message, error_message, "option-not-found");
                return;
            }

            g_autoptr(JsonParser) parser = json_parser_new ();
            gboolean result = json_parser_load_from_data (parser, value, -1, NULL);
            g_assert (result);

            json_builder_set_member_name (builder, key);
            json_builder_add_value (builder, json_node_ref (json_parser_get_root (parser)));
        }
        json_builder_end_object (builder);

        send_sync_response (snapd, message, 200, json_builder_get_root (builder));
    }
    else if (strcmp (message->method, "PUT") == 0) {
        g_autoptr(JsonNode) request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "cannot decode request body into patch values", NULL);
            return;
        }

        MockSnap *snap;
        if (strcmp (name, "system") == 0)
            snap = find_snap (snapd, "core");
        else
            snap = find_snap (snapd, name);
        if (snap == NULL) {
            send_error_not_found (snapd, message, "snap is not installed", "snap-not-found");
            return;
        }

        JsonObject *o = json_node_get_object (request);

        JsonObjectIter iter;
        const gchar *key;
        JsonNode *value_node;
        json_object_iter_init (&iter, o);
        while (json_object_iter_next (&iter, &key, &value_node)) {
            g_autoptr(JsonGenerator) generator = json_generator_new ();
            json_generator_set_root (generator, value_node);
            g_autofree gchar *value = json_generator_to_data (generator, NULL);
            mock_snap_set_conf (snap, key, value);
        }

        MockChange *change = add_change (snapd);
        send_async_response (snapd, message, 202, change->id);
    }
    else
        send_error_method_not_allowed (snapd, message, "method not allowed");
}

static void
handle_icon (MockSnapd *snapd, SoupMessage *message, const gchar *path)
{
    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    if (!g_str_has_suffix (path, "/icon")) {
        send_error_not_found (snapd, message, "not found", NULL);
        return;
    }
    g_autofree gchar *name = g_strndup (path, strlen (path) - strlen ("/icon"));

    MockSnap *snap = find_snap (snapd, name);
    if (snap == NULL)
        send_error_not_found (snapd, message, "cannot find snap", NULL);
    else if (snap->icon_data == NULL)
        send_error_not_found (snapd, message, "not found", NULL);
    else
        send_response (message, 200, snap->icon_mime_type,
                       (const guint8 *) g_bytes_get_data (snap->icon_data, NULL),
                       g_bytes_get_size (snap->icon_data));
}

static void
make_attributes (GHashTable *attributes, JsonBuilder *builder)
{
    g_autoptr(GPtrArray) keys = g_ptr_array_new_with_free_func (g_free);
    g_autofree GStrv key_names = (GStrv) g_hash_table_get_keys_as_array (attributes, NULL);
    for (int i = 0; key_names[i] != NULL; i++)
        g_ptr_array_add (keys, g_strdup (g_strstrip (key_names[i])));
    g_ptr_array_sort (keys, compare_keys);

    json_builder_begin_object (builder);
    for (guint i = 0; i < keys->len; i++) {
        const gchar *key = g_ptr_array_index (keys, i);
        const gchar *value = g_hash_table_lookup (attributes, key);

        g_autoptr(JsonParser) parser = json_parser_new ();
        gboolean result = json_parser_load_from_data (parser, value, -1, NULL);
        g_assert (result);

        json_builder_set_member_name (builder, key);
        json_builder_add_value (builder, json_node_ref (json_parser_get_root (parser)));
    }
    json_builder_end_object (builder);
}

static void
make_connections (MockSnapd *snapd, JsonBuilder *builder)
{
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "plugs");
    json_builder_begin_array (builder);
    for (GList *link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;

        for (GList *l = snap->plugs; l; l = l->next) {
            MockPlug *plug = l->data;

            g_autoptr(GList) slots = NULL;
            for (GList *l2 = snapd->established_connections; l2; l2 = l2->next) {
                MockConnection *connection = l2->data;
                if (connection->plug == plug)
                    slots = g_list_append (slots, connection->slot);
            }

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "snap");
            json_builder_add_string_value (builder, snap->name);
            json_builder_set_member_name (builder, "plug");
            json_builder_add_string_value (builder, plug->name);
            json_builder_set_member_name (builder, "interface");
            json_builder_add_string_value (builder, plug->interface->name);
            if (g_hash_table_size (plug->attributes) > 0) {
                json_builder_set_member_name (builder, "attrs");
                make_attributes (plug->attributes, builder);
            }
            json_builder_set_member_name (builder, "label");
            json_builder_add_string_value (builder, plug->label);
            if (slots != NULL) {
                json_builder_set_member_name (builder, "connections");
                json_builder_begin_array (builder);
                for (GList *l2 = slots; l2; l2 = l2->next) {
                    MockSlot *slot = l2->data;
                    json_builder_begin_object (builder);
                    json_builder_set_member_name (builder, "snap");
                    json_builder_add_string_value (builder, slot->snap->name);
                    json_builder_set_member_name (builder, "slot");
                    json_builder_add_string_value (builder, slot->name);
                    json_builder_end_object (builder);
                }
                json_builder_end_array (builder);
            }
            json_builder_end_object (builder);
        }
    }
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "slots");
    json_builder_begin_array (builder);
    for (GList *link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;

        for (GList *l = snap->slots_; l; l = l->next) {
            MockSlot *slot = l->data;

            g_autoptr(GList) plugs = NULL;
            for (GList *l2 = snapd->established_connections; l2; l2 = l2->next) {
                MockConnection *connection = l2->data;
                if (connection->slot == slot)
                    plugs = g_list_append (plugs, connection->plug);
            }

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "snap");
            json_builder_add_string_value (builder, snap->name);
            json_builder_set_member_name (builder, "slot");
            json_builder_add_string_value (builder, slot->name);
            json_builder_set_member_name (builder, "interface");
            json_builder_add_string_value (builder, slot->interface->name);
            if (g_hash_table_size (slot->attributes) > 0) {
                json_builder_set_member_name (builder, "attrs");
                make_attributes (slot->attributes, builder);
            }
            json_builder_set_member_name (builder, "label");
            json_builder_add_string_value (builder, slot->label);
            if (plugs != NULL) {
                json_builder_set_member_name (builder, "connections");
                json_builder_begin_array (builder);
                for (GList *l2 = plugs; l2; l2 = l2->next) {
                    MockPlug *plug = l2->data;
                    json_builder_begin_object (builder);
                    json_builder_set_member_name (builder, "snap");
                    json_builder_add_string_value (builder, plug->snap->name);
                    json_builder_set_member_name (builder, "plug");
                    json_builder_add_string_value (builder, plug->name);
                    json_builder_end_object (builder);
                }
                json_builder_end_array (builder);
            }
            json_builder_end_object (builder);
        }
    }
    json_builder_end_array (builder);
    json_builder_end_object (builder);
}

static gboolean
filter_interfaces (GStrv selected_interfaces, MockInterface *interface)
{
    /* If no filter selected, then return all interfaces */
    if (selected_interfaces == NULL || selected_interfaces[0] == NULL)
        return TRUE;

    for (int i = 0; selected_interfaces[i] != NULL; i++) {
        if (strcmp (selected_interfaces[i], interface->name) == 0)
            return TRUE;
    }

    return FALSE;
}

static gboolean
interface_connected (MockSnapd *snapd, MockInterface *interface)
{
    for (GList *l = snapd->snaps; l != NULL; l = l->next) {
        MockSnap *snap = l->data;

        for (GList *l2 = snap->plugs; l2 != NULL; l2 = l2->next) {
            MockPlug *plug = l2->data;
            if (plug->interface == interface)
                return TRUE;
        }
        for (GList *l2 = snap->slots_; l2 != NULL; l2 = l2->next) {
            MockSlot *slot = l2->data;
            if (slot->interface == interface)
                return TRUE;
        }
    }

    return FALSE;
}

static void
make_interfaces (MockSnapd *snapd, GHashTable *query, JsonBuilder *builder)
{
    const gchar *names_param = g_hash_table_lookup (query, "names");
    g_auto(GStrv) selected_interfaces = NULL;
    if (names_param != NULL)
        selected_interfaces = g_strsplit (names_param, ",", -1);
    gboolean only_connected = g_strcmp0 (g_hash_table_lookup (query, "select"), "connected") == 0;
    gboolean include_plugs = g_strcmp0 (g_hash_table_lookup (query, "plugs"), "true") == 0;
    gboolean include_slots = g_strcmp0 (g_hash_table_lookup (query, "slots"), "true") == 0;

    json_builder_begin_array (builder);

    for (GList *l = snapd->interfaces; l != NULL; l = l->next) {
        MockInterface *interface = l->data;

        if (!filter_interfaces (selected_interfaces, interface))
            continue;

        if (only_connected && !interface_connected (snapd, interface))
            continue;

        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "name");
        json_builder_add_string_value (builder, interface->name);
        json_builder_set_member_name (builder, "summary");
        json_builder_add_string_value (builder, interface->summary);
        json_builder_set_member_name (builder, "doc-url");
        json_builder_add_string_value (builder, interface->doc_url);

        if (include_plugs) {
            json_builder_set_member_name (builder, "plugs");
            json_builder_begin_array (builder);

            for (GList *l2 = snapd->snaps; l2 != NULL; l2 = l2->next) {
                MockSnap *snap = l2->data;

                for (GList *l3 = snap->plugs; l3 != NULL; l3 = l3->next) {
                    MockPlug *plug = l3->data;

                    if (plug->interface != interface)
                        continue;

                    json_builder_begin_object (builder);
                    json_builder_set_member_name (builder, "snap");
                    json_builder_add_string_value (builder, plug->snap->name);
                    json_builder_set_member_name (builder, "plug");
                    json_builder_add_string_value (builder, plug->name);
                    json_builder_end_object (builder);
                }
            }

            json_builder_end_array (builder);
        }

        if (include_slots) {
            json_builder_set_member_name (builder, "slots");
            json_builder_begin_array (builder);

            for (GList *l2 = snapd->snaps; l2 != NULL; l2 = l2->next) {
                MockSnap *snap = l2->data;

                for (GList *l3 = snap->slots_; l3 != NULL; l3 = l3->next) {
                    MockSlot *slot = l3->data;

                    if (slot->interface != interface)
                        continue;

                    json_builder_begin_object (builder);
                    json_builder_set_member_name (builder, "snap");
                    json_builder_add_string_value (builder, slot->snap->name);
                    json_builder_set_member_name (builder, "slot");
                    json_builder_add_string_value (builder, slot->name);
                    json_builder_end_object (builder);
                }
            }

            json_builder_end_array (builder);
        }

        json_builder_end_object (builder);
    }

    json_builder_end_array (builder);
}

static void
handle_interfaces (MockSnapd *snapd, SoupMessage *message, GHashTable *query)
{
    if (strcmp (message->method, "GET") == 0) {
        g_autoptr(JsonBuilder) builder = json_builder_new ();

        if (query != NULL && g_hash_table_lookup (query, "select") != NULL) {
            make_interfaces (snapd, query, builder);
        }
        else {
            make_connections (snapd, builder);
        }
        send_sync_response (snapd, message, 200, json_builder_get_root (builder));
    }
    else if (strcmp (message->method, "POST") == 0) {
        g_autoptr(JsonNode) request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "unknown content type", NULL);
            return;
        }

        JsonObject *o = json_node_get_object (request);
        const gchar *action = json_object_get_string_member (o, "action");

        JsonArray *a = json_object_get_array_member (o, "plugs");
        g_autoptr(GList) plugs = NULL;
        g_autoptr(GList) slots = NULL;
        for (guint i = 0; i < json_array_get_length (a); i++) {
            JsonObject *po = json_array_get_object_element (a, i);

            MockSnap *snap = find_snap (snapd, json_object_get_string_member (po, "snap"));
            if (snap == NULL) {
                send_error_bad_request (snapd, message, "invalid snap", NULL);
                return;
            }
            MockPlug *plug = mock_snap_find_plug (snap, json_object_get_string_member (po, "plug"));
            if (plug == NULL) {
                send_error_bad_request (snapd, message, "invalid plug", NULL);
                return;
            }
            plugs = g_list_append (plugs, plug);
        }

        a = json_object_get_array_member (o, "slots");
        for (guint i = 0; i < json_array_get_length (a); i++) {
            JsonObject *so = json_array_get_object_element (a, i);

            MockSnap *snap = find_snap (snapd, json_object_get_string_member (so, "snap"));
            if (snap == NULL) {
                send_error_bad_request (snapd, message, "invalid snap", NULL);
                return;
            }
            MockSlot *slot = mock_snap_find_slot (snap, json_object_get_string_member (so, "slot"));
            if (slot == NULL) {
                send_error_bad_request (snapd, message, "invalid slot", NULL);
                return;
            }
            slots = g_list_append (slots, slot);
        }

        if (strcmp (action, "connect") == 0) {
            if (g_list_length (plugs) < 1 || g_list_length (slots) < 1) {
                send_error_bad_request (snapd, message, "at least one plug and slot is required", NULL);
                return;
            }

            for (GList *link = plugs; link; link = link->next) {
                MockPlug *plug = link->data;
                mock_snapd_connect (snapd, plug, slots->data, TRUE, FALSE);
            }

            MockChange *change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            mock_change_add_task (change, "connect-snap");
            send_async_response (snapd, message, 202, change->id);
        }
        else if (strcmp (action, "disconnect") == 0) {
            if (g_list_length (plugs) < 1 || g_list_length (slots) < 1) {
                send_error_bad_request (snapd, message, "at least one plug and slot is required", NULL);
                return;
            }

            for (GList *link = plugs; link; link = link->next) {
                MockPlug *plug = link->data;
                mock_snapd_connect (snapd, plug, NULL, TRUE, FALSE);
            }

            MockChange *change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            mock_change_add_task (change, "disconnect");
            send_async_response (snapd, message, 202, change->id);
        }
        else
            send_error_bad_request (snapd, message, "unsupported interface action", NULL);
    }
    else
        send_error_method_not_allowed (snapd, message, "method not allowed");
}

static void
add_connection (JsonBuilder *builder, MockConnection *connection)
{
    json_builder_begin_object (builder);

    MockSlot *slot = connection->slot;
    json_builder_set_member_name (builder, "slot");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, slot->snap->name);
    json_builder_set_member_name (builder, "slot");
    json_builder_add_string_value (builder, slot->name);
    json_builder_end_object (builder);

    if (g_hash_table_size (slot->attributes) > 0) {
        json_builder_set_member_name (builder, "slot-attrs");
        make_attributes (slot->attributes, builder);
    }

    MockPlug *plug = connection->plug;
    json_builder_set_member_name (builder, "plug");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, plug->snap->name);
    json_builder_set_member_name (builder, "plug");
    json_builder_add_string_value (builder, plug->name);
    json_builder_end_object (builder);

    if (g_hash_table_size (plug->attributes) > 0) {
        json_builder_set_member_name (builder, "plug-attrs");
        make_attributes (plug->attributes, builder);
    }

    json_builder_set_member_name (builder, "interface");
    json_builder_add_string_value (builder, plug->interface->name);

    if (connection->manual) {
        json_builder_set_member_name (builder, "manual");
        json_builder_add_boolean_value (builder, TRUE);
    }

    if (connection->gadget) {
        json_builder_set_member_name (builder, "gadget");
        json_builder_add_boolean_value (builder, TRUE);
    }

    json_builder_end_object (builder);
}

static void
handle_connections (MockSnapd *snapd, SoupMessage *message)
{
    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);

    json_builder_set_member_name (builder, "established");
    json_builder_begin_array (builder);
    for (GList *link = snapd->established_connections; link; link = link->next) {
        MockConnection *connection = link->data;
        add_connection (builder, connection);
    }
    json_builder_end_array (builder);

    if (snapd->undesired_connections != NULL) {
        json_builder_set_member_name (builder, "undesired");
        json_builder_begin_array (builder);
        for (GList *link = snapd->undesired_connections; link; link = link->next) {
            MockConnection *connection = link->data;
            add_connection (builder, connection);
        }
        json_builder_end_array (builder);
    }

    json_builder_set_member_name (builder, "plugs");
    json_builder_begin_array (builder);
    for (GList *link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;

        for (GList *l = snap->plugs; l; l = l->next) {
            MockPlug *plug = l->data;

            g_autoptr(GList) slots = NULL;
            for (GList *l2 = snapd->established_connections; l2; l2 = l2->next) {
                MockConnection *connection = l2->data;
                if (connection->plug == plug)
                    slots = g_list_append (slots, connection->slot);
            }

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "snap");
            json_builder_add_string_value (builder, snap->name);
            json_builder_set_member_name (builder, "plug");
            json_builder_add_string_value (builder, plug->name);
            json_builder_set_member_name (builder, "interface");
            json_builder_add_string_value (builder, plug->interface->name);
            if (g_hash_table_size (plug->attributes) > 0) {
                json_builder_set_member_name (builder, "attrs");
                make_attributes (plug->attributes, builder);
            }
            json_builder_set_member_name (builder, "label");
            json_builder_add_string_value (builder, plug->label);
            if (slots != NULL) {
                json_builder_set_member_name (builder, "connections");
                json_builder_begin_array (builder);
                for (GList *l2 = slots; l2; l2 = l2->next) {
                    MockSlot *slot = l2->data;
                    json_builder_begin_object (builder);
                    json_builder_set_member_name (builder, "snap");
                    json_builder_add_string_value (builder, slot->snap->name);
                    json_builder_set_member_name (builder, "slot");
                    json_builder_add_string_value (builder, slot->name);
                    json_builder_end_object (builder);
                }
                json_builder_end_array (builder);
            }
            json_builder_end_object (builder);
        }
    }
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "slots");
    json_builder_begin_array (builder);
    for (GList *link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;

        for (GList *l = snap->slots_; l; l = l->next) {
            MockSlot *slot = l->data;

            g_autoptr(GList) plugs = NULL;
            for (GList *l2 = snapd->established_connections; l2; l2 = l2->next) {
                MockConnection *connection = l2->data;
                if (connection->slot == slot)
                    plugs = g_list_append (plugs, connection->plug);
            }

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "snap");
            json_builder_add_string_value (builder, snap->name);
            json_builder_set_member_name (builder, "slot");
            json_builder_add_string_value (builder, slot->name);
            json_builder_set_member_name (builder, "interface");
            json_builder_add_string_value (builder, slot->interface->name);
            if (g_hash_table_size (slot->attributes) > 0) {
                json_builder_set_member_name (builder, "attrs");
                make_attributes (slot->attributes, builder);
            }
            json_builder_set_member_name (builder, "label");
            json_builder_add_string_value (builder, slot->label);
            if (plugs != NULL) {
                json_builder_set_member_name (builder, "connections");
                json_builder_begin_array (builder);
                for (GList *l2 = plugs; l2; l2 = l2->next) {
                    MockPlug *plug = l2->data;
                    json_builder_begin_object (builder);
                    json_builder_set_member_name (builder, "snap");
                    json_builder_add_string_value (builder, plug->snap->name);
                    json_builder_set_member_name (builder, "plug");
                    json_builder_add_string_value (builder, plug->name);
                    json_builder_end_object (builder);
                }
                json_builder_end_array (builder);
            }
            json_builder_end_object (builder);
        }
    }
    json_builder_end_array (builder);
    json_builder_end_object (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder));
}

static MockTask *
get_current_task (MockChange *change)
{
    for (GList *link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;
        if (strcmp (task->status, "Done") != 0)
            return task;
    }

    return NULL;
}

static gboolean
change_get_ready (MockChange *change)
{
    MockTask *task = get_current_task (change);
    return task == NULL || strcmp (task->status, "Error") == 0;
}

static JsonNode *
make_change_node (MockChange *change)
{
    int progress_total = 0, progress_done = 0;
    for (GList *link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;
        progress_done += task->progress_done;
        progress_total += task->progress_total;
    }

    MockTask *task = get_current_task (change);
    const gchar *status = task != NULL ? task->status : "Done";
    const gchar *error = task != NULL && strcmp (task->status, "Error") == 0 ? task->error : NULL;

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "id");
    json_builder_add_string_value (builder, change->id);
    json_builder_set_member_name (builder, "kind");
    json_builder_add_string_value (builder, change->kind);
    json_builder_set_member_name (builder, "summary");
    json_builder_add_string_value (builder, change->summary);
    json_builder_set_member_name (builder, "status");
    json_builder_add_string_value (builder, status);
    json_builder_set_member_name (builder, "tasks");
    json_builder_begin_array (builder);
    for (GList *link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;

        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "id");
        json_builder_add_string_value (builder, task->id);
        json_builder_set_member_name (builder, "kind");
        json_builder_add_string_value (builder, task->kind);
        json_builder_set_member_name (builder, "summary");
        json_builder_add_string_value (builder, task->summary);
        json_builder_set_member_name (builder, "status");
        json_builder_add_string_value (builder, task->status);
        json_builder_set_member_name (builder, "progress");
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "label");
        json_builder_add_string_value (builder, task->progress_label);
        json_builder_set_member_name (builder, "done");
        json_builder_add_int_value (builder, task->progress_done);
        json_builder_set_member_name (builder, "total");
        json_builder_add_int_value (builder, task->progress_total);
        json_builder_end_object (builder);
        if (task->spawn_time != NULL) {
            json_builder_set_member_name (builder, "spawn-time");
            json_builder_add_string_value (builder, task->spawn_time);
        }
        if (task->progress_done >= task->progress_total && task->ready_time != NULL) {
            json_builder_set_member_name (builder, "ready-time");
            json_builder_add_string_value (builder, task->ready_time);
        }
        json_builder_end_object (builder);
    }
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "ready");
    json_builder_add_boolean_value (builder, change_get_ready (change));
    json_builder_set_member_name (builder, "spawn-time");
    json_builder_add_string_value (builder, change->spawn_time);
    if (change_get_ready (change) && change->ready_time != NULL) {
        json_builder_set_member_name (builder, "ready-time");
        json_builder_add_string_value (builder, change->ready_time);
    }
    if (change_get_ready (change) && change->data != NULL) {
        json_builder_set_member_name (builder, "data");
        json_builder_add_value (builder, json_node_ref (change->data));
    }
    if (error != NULL) {
        json_builder_set_member_name (builder, "err");
        json_builder_add_string_value (builder, error);
    }
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static gboolean
change_relates_to_snap (MockChange *change, const gchar *snap_name)
{
    for (GList *link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;

        if (g_strcmp0 (snap_name, task->snap_name) == 0)
            return TRUE;
        if (task->snap != NULL && g_strcmp0 (snap_name, task->snap->name) == 0)
            return TRUE;
    }

    return FALSE;
}

static void
handle_changes (MockSnapd *snapd, SoupMessage *message, GHashTable *query)
{
    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    const gchar *select_param = g_hash_table_lookup (query, "select");
    if (select_param == NULL)
        select_param = "in-progress";
    const gchar *for_param = g_hash_table_lookup (query, "for");

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (GList *link = snapd->changes; link; link = link->next) {
        MockChange *change = link->data;

        if (g_strcmp0 (select_param, "in-progress") == 0 && change_get_ready (change))
            continue;
        if (g_strcmp0 (select_param, "ready") == 0 && !change_get_ready (change))
            continue;
        if (for_param != NULL && !change_relates_to_snap (change, for_param))
            continue;

        json_builder_add_value (builder, make_change_node (change));
    }
    json_builder_end_array (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder));
}

static void
mock_task_complete (MockSnapd *snapd, MockTask *task)
{
    if (strcmp (task->kind, "install") == 0 || strcmp (task->kind, "try") == 0)
        snapd->snaps = g_list_append (snapd->snaps, g_steal_pointer (&task->snap));
    else if (strcmp (task->kind, "remove") == 0) {
        MockSnap *snap = find_snap (snapd, task->snap_name);
        snapd->snaps = g_list_remove (snapd->snaps, snap);
        mock_snap_free (snap);
    }
    mock_task_set_status (task, "Done");
}

static void
mock_change_progress (MockSnapd *snapd, MockChange *change)
{
    for (GList *link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;

        if (task->error != NULL) {
            mock_task_set_status (task, "Error");
            return;
        }

        if (task->progress_done < task->progress_total) {
            mock_task_set_status (task, "Doing");
            task->progress_done++;
            if (task->progress_done == task->progress_total)
                mock_task_complete (snapd, task);
            return;
        }
    }
}

static void
handle_change (MockSnapd *snapd, SoupMessage *message, const gchar *change_id)
{
    if (strcmp (message->method, "GET") == 0) {
        MockChange *change = get_change (snapd, change_id);
        if (change == NULL) {
            send_error_not_found (snapd, message, "cannot find change", NULL);
            return;
        }
        mock_change_progress (snapd, change);

        send_sync_response (snapd, message, 200, make_change_node (change));
    }
    else if (strcmp (message->method, "POST") == 0) {
        MockChange *change = get_change (snapd, change_id);
        if (change == NULL) {
            send_error_not_found (snapd, message, "cannot find change", NULL);
            return;
        }

        g_autoptr(JsonNode) request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "unknown content type", NULL);
            return;
        }

        JsonObject *o = json_node_get_object (request);
        const gchar *action = json_object_get_string_member (o, "action");
        if (strcmp (action, "abort") == 0) {
            MockTask *task = get_current_task (change);
            if (task == NULL) {
                send_error_bad_request (snapd, message, "task not in progress", NULL);
                return;
            }
            mock_task_set_status (task, "Error");
            task->error = g_strdup ("cancelled");
            send_sync_response (snapd, message, 200, make_change_node (change));
        }
        else {
            send_error_bad_request (snapd, message, "change action is unsupported", NULL);
            return;
        }
    }
    else {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }
}

static gboolean
matches_query (MockSnap *snap, const gchar *query)
{
    return query == NULL || strstr (snap->name, query) != NULL;
}

static gboolean
matches_name (MockSnap *snap, const gchar *name)
{
    return name == NULL || strcmp (snap->name, name) == 0;
}

static gboolean
matches_scope (MockSnap *snap, const gchar *scope)
{
    if (g_strcmp0 (scope, "wide") == 0)
        return TRUE;

    return !snap->scope_is_wide;
}

static gboolean
has_common_id (MockSnap *snap, const gchar *common_id)
{
    if (common_id == NULL)
        return TRUE;

    for (GList *link = snap->apps; link; link = link->next) {
        MockApp *app = link->data;
        if (g_strcmp0 (app->common_id, common_id) == 0)
            return TRUE;
    }

    return FALSE;
}

static gboolean
in_section (MockSnap *snap, const gchar *section)
{
    if (section == NULL)
        return TRUE;

    for (GList *link = snap->store_sections; link; link = link->next)
        if (strcmp (link->data, section) == 0)
            return TRUE;

    return FALSE;
}

static void
handle_find (MockSnapd *snapd, SoupMessage *message, GHashTable *query)
{
    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    const gchar *common_id_param = g_hash_table_lookup (query, "common-id");
    const gchar *query_param = g_hash_table_lookup (query, "q");
    const gchar *name_param = g_hash_table_lookup (query, "name");
    const gchar *select_param = g_hash_table_lookup (query, "select");
    const gchar *section_param = g_hash_table_lookup (query, "section");
    const gchar *scope_param = g_hash_table_lookup (query, "scope");

    if (common_id_param && strcmp (common_id_param, "") == 0)
        common_id_param = NULL;
    if (query_param && strcmp (query_param, "") == 0)
        query_param = NULL;
    if (name_param && strcmp (name_param, "") == 0)
        name_param = NULL;
    if (select_param && strcmp (select_param, "") == 0)
        select_param = NULL;
    if (section_param && strcmp (section_param, "") == 0)
        section_param = NULL;
    if (scope_param && strcmp (scope_param, "") == 0)
        scope_param = NULL;

    if (g_strcmp0 (select_param, "refresh") == 0) {
        g_autoptr(GList) refreshable_snaps = NULL;

        if (query_param != NULL) {
            send_error_bad_request (snapd, message, "cannot use 'q' with 'select=refresh'", NULL);
            return;
        }
        if (name_param != NULL) {
            send_error_bad_request (snapd, message, "cannot use 'name' with 'select=refresh'", NULL);
            return;
        }

        g_autoptr(JsonBuilder) builder = json_builder_new ();
        json_builder_begin_array (builder);
        refreshable_snaps = get_refreshable_snaps (snapd);
        for (GList *link = refreshable_snaps; link; link = link->next)
            json_builder_add_value (builder, make_snap_node (link->data));
        json_builder_end_array (builder);

        send_sync_response (snapd, message, 200, json_builder_get_root (builder));
        return;
    }
    else if (g_strcmp0 (select_param, "private") == 0)
        g_assert (FALSE);

    /* Make a special query that never responds */
    if (g_strcmp0 (query_param, "do-not-respond") == 0)
        return;

    /* Make a special query that simulates a network timeout */
    if (g_strcmp0 (query_param, "network-timeout") == 0) {
        send_error_bad_request (snapd, message, "unable to contact snap store", "network-timeout");
        return;
    }

    /* Make a special query that simulates a DNS Failure */
    if (g_strcmp0 (query_param, "dns-failure") == 0) {
        send_error_bad_request (snapd, message, "failed to resolve address", "dns-failure");
        return;
    }

    /* Certain characters not allowed in queries */
    if (query_param != NULL) {
        for (int i = 0; query_param[i] != '\0'; i++) {
            const gchar *invalid_chars = "+=&|><!(){}[]^\"~*?:\\/";
            if (strchr (invalid_chars, query_param[i]) != NULL) {
                send_error_bad_request (snapd, message, "bad query", "bad-query");
                return;
            }
        }
    }

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (GList *link = snapd->store_snaps; link; link = link->next) {
        MockSnap *snap = link->data;

        if (!has_common_id (snap, common_id_param))
            continue;

        if (!in_section (snap, section_param))
            continue;

        if (!matches_query (snap, query_param))
            continue;

        if (!matches_name (snap, name_param))
            continue;

        if (!matches_scope (snap, scope_param))
            continue;

        json_builder_add_value (builder, make_snap_node (snap));
    }
    json_builder_end_array (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder));
}

static void
handle_sections (MockSnapd *snapd, SoupMessage *message)
{
    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (GList *link = snapd->store_sections; link; link = link->next)
        json_builder_add_string_value (builder, link->data);
    json_builder_end_array (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder));
}

static void
handle_request (SoupServer *server G_GNUC_UNUSED, SoupMessage *message, const gchar *path, GHashTable *query, SoupClientContext *client, gpointer user_data)
{
    MockSnapd *snapd = MOCK_SNAPD (user_data);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    if (snapd->close_on_request) {
        g_autoptr(GIOStream) stream = soup_client_context_steal_connection (client);
        g_autoptr(GError) error = NULL;

        if (!g_io_stream_close (stream, NULL, &error))
            g_warning("Failed to close stream: %s", error->message);
        return;
    }

    g_clear_pointer (&snapd->last_request_headers, soup_message_headers_free);
    snapd->last_request_headers = g_boxed_copy (SOUP_TYPE_MESSAGE_HEADERS, message->request_headers);

    if (strcmp (path, "/v2/system-info") == 0)
        handle_system_info (snapd, message);
    else if (strcmp (path, "/v2/snaps") == 0)
        handle_snaps (snapd, message, query);
    else if (g_str_has_prefix (path, "/v2/snaps/")) {
        if (g_str_has_suffix (path, "/conf")) {
            g_autofree gchar *name = g_strndup (path + strlen ("/v2/snaps/"), strlen (path) - strlen ("/v2/snaps/") - strlen ("/conf"));
            handle_snap_conf (snapd, message, name, query);
        }
        else
            handle_snap (snapd, message, path + strlen ("/v2/snaps/"));
    }
    else if (g_str_has_prefix (path, "/v2/icons/"))
        handle_icon (snapd, message, path + strlen ("/v2/icons/"));
    else if (strcmp (path, "/v2/interfaces") == 0)
        handle_interfaces (snapd, message, query);
    else if (strcmp (path, "/v2/connections") == 0)
        handle_connections (snapd, message);
    else if (strcmp (path, "/v2/changes") == 0)
        handle_changes (snapd, message, query);
    else if (g_str_has_prefix (path, "/v2/changes/"))
        handle_change (snapd, message, path + strlen ("/v2/changes/"));
    else if (strcmp (path, "/v2/find") == 0)
        handle_find (snapd, message, query);
    else if (strcmp (path, "/v2/sections") == 0)
        handle_sections (snapd, message);
    else
        send_error_not_found (snapd, message, "not found", NULL);
}

static gboolean
mock_snapd_thread_quit (gpointer user_data)
{
    MockSnapd *snapd = MOCK_SNAPD (user_data);

    g_main_loop_quit (snapd->loop);

    return G_SOURCE_REMOVE;
}

static void
mock_snapd_finalize (GObject *object)
{
    MockSnapd *snapd = MOCK_SNAPD (object);

    /* shut down the server if it is running */
    mock_snapd_stop (snapd);

    if (g_unlink (snapd->socket_path) < 0 && errno != ENOENT)
        g_printerr ("Failed to unlink mock snapd socket: %s\n", g_strerror (errno));
    if (g_rmdir (snapd->dir_path) < 0)
        g_printerr ("Failed to remove temporary directory: %s\n", g_strerror (errno));

    g_clear_pointer (&snapd->dir_path, g_free);
    g_clear_pointer (&snapd->socket_path, g_free);
    g_list_free_full (snapd->interfaces, (GDestroyNotify) mock_interface_free);
    snapd->interfaces = NULL;
    g_list_free_full (snapd->snaps, (GDestroyNotify) mock_snap_free);
    snapd->snaps = NULL;
    g_free (snapd->build_id);
    g_free (snapd->confinement);
    g_clear_pointer (&snapd->sandbox_features, g_hash_table_unref);
    g_free (snapd->store);
    g_free (snapd->maintenance_kind);
    g_free (snapd->maintenance_message);
    g_free (snapd->refresh_hold);
    g_free (snapd->refresh_last);
    g_free (snapd->refresh_next);
    g_free (snapd->refresh_schedule);
    g_free (snapd->refresh_timer);
    g_list_free_full (snapd->store_sections, g_free);
    snapd->store_sections = NULL;
    g_list_free_full (snapd->store_snaps, (GDestroyNotify) mock_snap_free);
    snapd->store_snaps = NULL;
    g_list_free_full (snapd->established_connections, (GDestroyNotify) mock_connection_free);
    snapd->established_connections = NULL;
    g_list_free_full (snapd->undesired_connections, (GDestroyNotify) mock_connection_free);
    snapd->undesired_connections = NULL;
    g_list_free_full (snapd->assertions, g_free);
    snapd->assertions = NULL;
    g_list_free_full (snapd->changes, (GDestroyNotify) mock_change_free);
    snapd->changes = NULL;
    g_clear_pointer (&snapd->spawn_time, g_free);
    g_clear_pointer (&snapd->ready_time, g_free);
    g_clear_pointer (&snapd->last_request_headers, soup_message_headers_free);
    g_clear_pointer (&snapd->context, g_main_context_unref);
    g_clear_pointer (&snapd->loop, g_main_loop_unref);

    g_cond_clear (&snapd->condition);
    g_mutex_clear (&snapd->mutex);

    G_OBJECT_CLASS (mock_snapd_parent_class)->finalize (object);
}

static GSocket *
open_listening_socket (SoupServer *server, const gchar *socket_path, GError **error)
{
    g_autoptr(GSocket) socket = NULL;
    g_autoptr(GSocketAddress) address = NULL;

    socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                           G_SOCKET_TYPE_STREAM,
                           G_SOCKET_PROTOCOL_DEFAULT,
                           error);
    if (socket == NULL)
        return NULL;

    address = g_unix_socket_address_new (socket_path);
    if (!g_socket_bind (socket, address, TRUE, error))
        return NULL;

    if (!g_socket_listen (socket, error))
        return NULL;

    if (!soup_server_listen_socket (server, socket, 0, error))
        return NULL;

    return g_steal_pointer (&socket);
}

gpointer
mock_snapd_init_thread (gpointer user_data)
{
    MockSnapd *snapd = MOCK_SNAPD (user_data);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    snapd->context = g_main_context_new ();
    g_main_context_push_thread_default (snapd->context);
    snapd->loop = g_main_loop_new (snapd->context, FALSE);

    g_autoptr(SoupServer) server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "MockSnapd/1.0", NULL);
    soup_server_add_handler (server, NULL,
                             handle_request, snapd, NULL);

    g_autoptr(GError) error = NULL;
    g_autoptr(GSocket) socket = open_listening_socket (server, snapd->socket_path, &error);

    g_cond_signal (&snapd->condition);
    if (socket == NULL)
        g_propagate_error (snapd->thread_init_error, error);
    g_clear_pointer (&locker, g_mutex_locker_free);

    /* run until we're told to stop */
    if (socket != NULL)
        g_main_loop_run (snapd->loop);

    if (g_unlink (snapd->socket_path) < 0)
        g_printerr ("Failed to unlink mock snapd socket\n");

    g_clear_pointer (&snapd->loop, g_main_loop_unref);
    g_clear_pointer (&snapd->context, g_main_context_unref);

    return NULL;
}

gboolean
mock_snapd_start (MockSnapd *snapd, GError **dest_error)
{
    g_autoptr(GError) error = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), FALSE);

    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    /* Has the server already started? */
    if (snapd->thread)
        return TRUE;

    snapd->thread_init_error = &error;
    snapd->thread = g_thread_new ("mock_snapd_thread",
                                  mock_snapd_init_thread,
                                  snapd);
    g_cond_wait (&snapd->condition, &snapd->mutex);
    snapd->thread_init_error = NULL;

    /* If an error occurred during thread startup, clean it up */
    if (error != NULL) {
        g_thread_join (snapd->thread);
        snapd->thread = NULL;

        if (dest_error)
            g_propagate_error (dest_error, error);
        else
            g_warning ("Failed to start server: %s", error->message);
    }

    g_assert_nonnull (snapd->loop);
    g_assert_nonnull (snapd->context);

    return error == NULL;
}

void
mock_snapd_stop (MockSnapd *snapd)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    if (!snapd->thread)
        return;

    g_main_context_invoke (snapd->context, mock_snapd_thread_quit, snapd);
    g_thread_join (snapd->thread);
    snapd->thread = NULL;
    g_assert_null (snapd->loop);
    g_assert_null (snapd->context);
}

static void
mock_snapd_class_init (MockSnapdClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = mock_snapd_finalize;
}

static void
mock_snapd_init (MockSnapd *snapd)
{
    g_mutex_init (&snapd->mutex);
    g_cond_init (&snapd->condition);

    snapd->sandbox_features = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_ptr_array_unref);
    g_autoptr(GError) error = NULL;
    snapd->dir_path = g_dir_make_tmp ("mock-snapd-XXXXXX", &error);
    if (snapd->dir_path == NULL)
        g_warning ("Failed to make temporary directory: %s", error->message);
    g_clear_error (&error);
    snapd->socket_path = g_build_filename (snapd->dir_path, "snapd.socket", NULL);
}

int
main (int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED)
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) server = mock_snapd_new ();
    g_autoptr(GError) error = NULL;
    if (!mock_snapd_start (server, &error)) {
        g_printerr ("Failed to start server: %s\n", error->message);
        return EXIT_FAILURE;
    }
    g_printerr ("Listening on socket %s\n", mock_snapd_get_socket_path (server));

    g_main_loop_run (loop);

    return EXIT_SUCCESS;
}
