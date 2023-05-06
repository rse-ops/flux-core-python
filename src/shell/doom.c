/************************************************************\
 * Copyright 2021 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

/* doom.c - log first task exit
 *
 * Each shell sends a message to shell-0 when its first task exits.
 * Shell-0 posts an event to the exec eventlog for the first one received.
 *
 * Shell-0 sets a timer and posts a fatal exception when the timer fires.
 *
 * Shell options to modify the default behavior:
 *
 * exit-timeout
 *   Change the timeout value (FSD), or disable the timer with value "none".
 *
 * exit-on-error
 *   Raise the fatal exception immediately if the first task fails,
 *   e.g. calls exit with a nonzero value or is terminated by signal.
 */
#define FLUX_SHELL_PLUGIN_NAME "doom"

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <flux/core.h>
#include <jansson.h>
#include <assert.h>

#include "src/common/libeventlog/eventlog.h"
#include "src/common/libutil/fsd.h"

#include "builtins.h"
#include "internal.h"
#include "task.h"

#define TIMEOUT_NONE (-1.)

static const double default_timeout = 30.;

struct shell_doom {
    flux_shell_t *shell;
    bool done; // event already posted (shell rank 0) or message sent (> 0)
    flux_watcher_t *timer;
    double timeout;
    bool exit_on_error;
    int exit_rc;
    int exit_rank;
};

static int get_exit_code (json_t *task_info)
{
    int status;

    if (json_unpack (task_info, "{s:i}", "wait_status", &status) < 0)
        goto error;
    if (WIFEXITED (status))
        return WEXITSTATUS (status);
    if (WIFSIGNALED (status))
        return 128 + WTERMSIG (status);
error:
    shell_log_error ("error decoding task wait status");
    return 1;
}

static int get_exit_rank (json_t *task_info)
{
    int rank = -1;

    if (json_unpack (task_info, "{s:i}", "rank", &rank) < 0)
        shell_log_error ("error decoding task rank");
    return rank;
}

static void doom_post (struct shell_doom *doom, json_t *task_info)
{
    flux_kvs_txn_t *txn;
    json_t *entry = NULL;
    char *entrystr = NULL;
    flux_future_t *f = NULL;

    assert (doom->shell->info->shell_rank == 0);

    if (!(txn = flux_kvs_txn_create ())
        || !(entry = eventlog_entry_pack (0.,
                                          "shell.task-exit",
                                          "O",
                                          task_info))
        || !(entrystr = eventlog_entry_encode (entry))
        || flux_kvs_txn_put (txn,
                             FLUX_KVS_APPEND,
                             "exec.eventlog",
                             entrystr) < 0
        || !(f = flux_kvs_commit (doom->shell->h, NULL, 0, txn)))
        shell_log_errno ("error posting task-exit eventlog entry");

    doom->exit_rc = get_exit_code (task_info);
    doom->exit_rank = get_exit_rank (task_info);

    if (doom->exit_on_error && doom->exit_rc != 0) {
        shell_die (doom->exit_rc,
                   "rank %d failed and exit-on-error is set",
                   doom->exit_rank);
    }
    else if (f && doom->timeout != TIMEOUT_NONE)
        flux_watcher_start (doom->timer);

    flux_future_destroy (f); // fire and forget
    free (entrystr);
    json_decref (task_info);
    flux_kvs_txn_destroy (txn);
}

static void doom_notify_cb (flux_t *h,
                            flux_msg_handler_t *mh,
                            const flux_msg_t *msg,
                            void *arg)
{
    struct shell_doom *doom = arg;
    json_t *task_info;

    assert (doom->shell->info->shell_rank == 0);

    if (doom->done)
        return;
    if (flux_request_unpack (msg, NULL, "o", &task_info) < 0) {
        shell_log_errno ("error parsing first task exit notification");
        return;
    }
    doom_post (doom, task_info);
    doom->done = true;
}

static void doom_notify (struct shell_doom *doom, json_t *task_info)
{
    flux_future_t *f;

    assert (doom->shell->info->shell_rank > 0);

    if (!(f = flux_shell_rpc_pack (doom->shell,
                                   "doom",
                                   0,
                                   FLUX_RPC_NORESPONSE,
                                   "O",
                                   task_info)))
        shell_log_errno ("error notifying rank 0 of first task exit");
    flux_future_destroy (f);
}

static void doom_timeout (flux_reactor_t *r,
                          flux_watcher_t *w,
                          int revents,
                          void *arg)
{
    struct shell_doom *doom = arg;
    char fsd[64];

    fsd_format_duration (fsd, sizeof (fsd), doom->timeout);
    shell_die (doom->exit_rc,
               "rank %d exited and exit-timeout=%s has expired",
               doom->exit_rank,
               fsd);
}

static int doom_task_exit (flux_plugin_t *p,
                           const char *topic,
                           flux_plugin_arg_t *args,
                           void *arg)
{
    flux_shell_t *shell;
    struct shell_doom *doom;
    flux_shell_task_t *task;

    if (!(shell = flux_plugin_get_shell (p))
        || !(doom = flux_plugin_aux_get (p, "doom"))
        || !(task = flux_shell_current_task (shell)))
        return -1;
    if (!doom->done) {
        json_t *task_info;

        if (flux_shell_task_info_unpack (task, "o", &task_info) < 0)
            return -1;
        if (shell->info->shell_rank == 0)
            doom_post (doom, task_info);
        else
            doom_notify (doom, task_info);
        doom->done = true;
    }
    return 0;
}

static void doom_destroy (struct shell_doom *doom)
{
    if (doom) {
        int saved_errno = errno;
        flux_watcher_destroy (doom->timer);
        free (doom);
        errno = saved_errno;
    }
}

static int parse_args (flux_shell_t *shell,
                       double *timeout,
                       bool *exit_on_error)
{
    json_t *val = NULL;

    if (flux_shell_getopt_unpack (shell, "exit-timeout", "o", &val) < 0)
        return -1;
    if (val) {
        if (json_is_string (val)) {
            double n;
            if (fsd_parse_duration (json_string_value (val), &n) < 0) {
                if (!strcasecmp (json_string_value (val), "none"))
                    n = TIMEOUT_NONE;
                else
                    goto error;
            }
            *timeout = n;
        }
        else if (json_is_number (val)) {
            if (json_number_value (val) < 0)
                goto error;
            *timeout = json_number_value (val);
        }
        else
            goto error;
    }
    *exit_on_error = (flux_shell_getopt (shell,
                                         "exit-on-error",
                                         NULL) == 1) ? true : false;
    return 0;
error:
    shell_log_error ("exit-timeout is not a valid Flux Standard Duration");
    return -1;
}

static struct shell_doom *doom_create (flux_shell_t *shell)
{
    struct shell_doom *doom;

    if (!(doom = calloc (1, sizeof (*doom))))
        return NULL;
    doom->shell = shell;
    doom->timeout = default_timeout;
    if (parse_args (shell, &doom->timeout, &doom->exit_on_error) < 0)
        goto error;
    if (shell->info->shell_rank == 0) {
        if (flux_shell_service_register (shell,
                                         "doom",
                                         doom_notify_cb,
                                         doom) < 0)
            goto error;
        if (doom->timeout != TIMEOUT_NONE) {
            if (!(doom->timer = flux_timer_watcher_create (shell->r,
                                                           doom->timeout,
                                                           0.,
                                                           doom_timeout,
                                                           doom)))
                goto error;
        }
    }
    return doom;
error:
    doom_destroy (doom);
    return NULL;
}

static int doom_init (flux_plugin_t *p,
                      const char *topic,
                      flux_plugin_arg_t *arg,
                      void *data)
{
    flux_shell_t *shell = flux_plugin_get_shell (p);
    struct shell_doom *doom;
    if (!shell || !(doom = doom_create (shell)))
        return -1;
    if (flux_plugin_aux_set (p, "doom", doom, (flux_free_f) doom_destroy) < 0) {
        doom_destroy (doom);
        return -1;
    }
    return 0;
}

struct shell_builtin builtin_doom = {
    .name = FLUX_SHELL_PLUGIN_NAME,
    .init = doom_init,
    .task_exit = doom_task_exit,
};

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
