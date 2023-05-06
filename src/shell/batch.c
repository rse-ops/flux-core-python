/************************************************************\
 * Copyright 2020 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

/* batch script handler
 */
#define FLUX_SHELL_PLUGIN_NAME "batch"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <jansson.h>
#include <flux/core.h>
#include <flux/shell.h>

#include "src/common/libutil/read_all.h"

#include "builtins.h"

struct batch_info {
    flux_jobid_t id; /* This jobid */
    int shell_rank;  /* This shell rank */
    char *script;    /* Path to locally created job script */
    json_t *options; /* Extra broker options */
};

static void batch_info_destroy (void *arg)
{
    struct batch_info *b = arg;
    if (b) {
        if (b->shell_rank == 0 && b->script)
            unlink (b->script);
        free (b->script);
        free (b);
    }
}

static struct batch_info *
batch_info_create (flux_shell_t *shell, json_t *batch)
{
    struct batch_info *b = calloc (1, sizeof (*b));
    json_error_t err;
    const char *data;
    size_t len;

    if (flux_shell_info_unpack (shell,
                               "{s:i s:I}",
                               "rank",
                               &b->shell_rank,
                               "jobid",
                               &b->id) < 0) {
        shell_log_errno ("failed to unpack shell info");
        goto error;
    }

    if (json_unpack_ex (batch, &err, 0,
                        "{s?o s:s%}",
                        "broker-opts", &b->options,
                        "script", &data, &len) < 0) {
        shell_log_error ("failed to unpack batch info: %s", err.text);
        goto error;
    }

    if (b->options && !json_is_array (b->options)) {
        shell_log_error ("batch.broker-opts attribute must be an array");
        goto error;
    }

    if (data && b->shell_rank == 0) {
        int fd = -1;
        const char *tmpdir = getenv ("TMPDIR");

        if (asprintf (&b->script, "%s/flux-script-%ju-XXXXXX",
                      tmpdir ? tmpdir : "/tmp",
                      b->id) < 0) {
            shell_log_error ("asprintf script templated failed");
            goto error;
        }
        if ((fd = mkstemp (b->script)) < 0) {
            shell_log_error ("mkstemp");
            goto error;
        }
        shell_debug ("Copying batch script size=%zu for job to %s",
                     len,
                     b->script);
        if (write_all (fd, data, len) < 0) {
            shell_log_error ("failed to write batch script");
            goto error;
        }
        if (fchmod (fd, 0700) < 0) {
            shell_log_error ("chmod(%s)", b->script);
            goto error;
        }
        close (fd);
    }
    return b;
error:
    batch_info_destroy (b);
    return NULL;
}

static int cmd_append_broker_options (flux_cmd_t *cmd,
                                      struct batch_info *b)
{
    int index;
    if (b->options == 0)
        return 0;
    index = json_array_size (b->options) - 1;
    while (index >= 0) {
        json_t *val = json_array_get (b->options, index);
        const char *s = json_string_value (val);
        if (s && flux_cmd_argv_insert (cmd, 0, s) < 0)
            return shell_log_errno ("Failed to prepend broker opt %s", s);
        index--;
    }
    return 0;
}

static void log_task_commandline (flux_cmd_t *cmd, int taskid)
{
    char *s = flux_cmd_stringify (cmd);
    shell_debug ("task%d: re-writing command to %s", taskid, s);
    free (s);
}

static int task_batchify (flux_plugin_t *p,
                          const char *topic,
                          flux_plugin_arg_t *args,
                          void *data)
{
    struct batch_info *b = data;
    flux_shell_t *shell = flux_plugin_get_shell (p);
    flux_shell_task_t *task = NULL;
    flux_cmd_t *cmd = NULL;
    int taskid = -1;

    if (!shell
        || !(task = flux_shell_current_task (shell))
        || !(cmd = flux_shell_task_cmd (task)))
        return shell_log_errno ("failed to get task cmd");

    if (flux_shell_task_info_unpack (task, "{s:i}", "rank", &taskid) < 0)
        return shell_log_errno ("failed to unpack task rank");

    if (taskid == 0) {
        /* For rank 0 broker, delete argv0 and replace
         *  with path to our script
         */
        if (flux_cmd_argv_delete (cmd, 0) < 0
            || flux_cmd_argv_insert (cmd, 0, b->script) < 0)
        return shell_log_errno ("failed to replace command with batch script");
    }
    else {
        /* Other ranks, delete all args, they are unused */
        while (flux_cmd_argv_delete (cmd, 0) == 0)
           ; 
    }

    /*  All broker ranks, add broker options */
    if (cmd_append_broker_options (cmd, b) < 0)
        return -1;

    /*  All broker ranks: prepend 'flux broker' */
    if (flux_cmd_argv_insert (cmd, 0, "broker") < 0
        || flux_cmd_argv_insert (cmd, 0, "flux") < 0)
        return shell_log_errno ("failed to prepend command with flux broker");

    log_task_commandline (cmd, taskid);

    return 0;
}


static int batch_init (flux_plugin_t *p,
                       const char *topic,
                       flux_plugin_arg_t *args,
                       void *data)
{
    flux_shell_t *shell = flux_plugin_get_shell (p);
    json_t *jobspec = NULL;
    json_t *batch = NULL;

    if (flux_shell_info_unpack (shell,
                               "{s:o}",
                               "jobspec",
                               &jobspec) < 0)
        return shell_log_errno ("failed to unpack jobspec");
    json_error_t err;
    if (json_unpack_ex (jobspec, &err, 0,
                     "{s:{s:{s?o}}}",
                     "attributes",
                       "system",
                         "batch", &batch) < 0) {
        shell_log_error ("failed to unpack batch object from jobspec: %s",
                        err.text);
        return -1;
    }

    if (batch) {
        struct batch_info *b = batch_info_create (shell, batch);

        if (!b || flux_plugin_aux_set (p, "batch", b, batch_info_destroy) < 0) {
            batch_info_destroy (b);
            return -1;
        }
        if (flux_plugin_add_handler (p, "task.init", task_batchify, b) < 0)
            return shell_log_errno ("failed to add task.init handler");
    }
    return 0;
}

struct shell_builtin builtin_batch = {
    .name = FLUX_SHELL_PLUGIN_NAME,
    .init = batch_init,
};

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
