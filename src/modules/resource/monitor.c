/************************************************************\
 * Copyright 2020 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

/* monitor.c - track execution targets joining/leaving the instance
 *
 * Watches the broker.online group and posts online/offline events as
 * the broker.online set changes.
 *
 * The initial online set used in the restart event will be empty as
 * the initial response to the request to watch broker.online cannot
 * be processed until the reactor runs.
 *
 * Some synchronization notes:
 * - rc1 completes on rank 0 before any other ranks can join broker.online,
 *   therefore the scheduler must allow flux module load to complete with
 *   potentially all node resources offline, or deadlock will result.
 * - it is racy to read broker.online and assume that online events have
 *   been posted for those ranks, as the resource module needs time to
 *   receive notification from the broker and process it.
 * - the initial program starts once broker.online reaches the configured
 *   quorum (all ranks unless configured otherwise, e.g. system instance).
 *   It is racy to assume that online events have been posted for the quorum
 *   ranks in the initial program for the same reason as above.
 * - the 'resource.monitor-waitup' RPC allows a test to wait for some number
 *   of ranks to be up, where "up" is defined as having had an online event
 *   posted.  Thus, after waiting, resource.status (flux resource status)
 *   should show those ranks up, while sched.resource-status
 *   (flux resource list command) may still show them down.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <flux/core.h>
#include <jansson.h>

#include "src/common/libczmqcontainers/czmq_containers.h"
#include "src/common/libidset/idset.h"
#include "src/common/libutil/errno_safe.h"

#include "resource.h"
#include "reslog.h"
#include "monitor.h"
#include "rutil.h"

struct monitor {
    struct resource_ctx *ctx;
    flux_future_t *f;
    struct idset *up;
    struct idset *down; // cached result of monitor_get_down()
    flux_msg_handler_t **handlers;
    struct flux_msglist *waitup_requests;
    int size;
};

static void notify_waitup (struct monitor *monitor);

const struct idset *monitor_get_up (struct monitor *monitor)
{
    return monitor->up;
}

const struct idset *monitor_get_down (struct monitor *monitor)
{
    unsigned int id;

    if (!monitor->down) {
        if (!(monitor->down = idset_create (monitor->size, 0)))
            return NULL;
    }
    for (id = 0; id < monitor->size; id++) {
        if (idset_test (monitor->up, id))
            (void)idset_clear (monitor->down, id);
        else
            (void)idset_set (monitor->down, id);
    }
    return monitor->down;
}

/* Leader: set of online brokers has changed.
 * Update monitor->up and post online/offline events to resource.eventlog.
 * Avoid posting events if nothing changed.
 */
static void broker_online_cb (flux_future_t *f, void *arg)
{
    struct monitor *monitor = arg;
    flux_t *h = monitor->ctx->h;
    const char *members;
    struct idset *new_up = NULL;
    struct idset *join = NULL;
    struct idset *leave = NULL;
    char *online = NULL;
    char *offline = NULL;

    if (flux_rpc_get_unpack (f, "{s:s}", "members", &members) < 0) {
        flux_log_error (h, "monitor: group.get failed");
        return;
    }
    if (!(new_up = idset_decode (members))
        || !(join = idset_difference (new_up, monitor->up))
        || !(leave = idset_difference (monitor->up, new_up)))
        goto done;
    if (idset_count (join) > 0) {
        if (!(online = idset_encode (join, IDSET_FLAG_RANGE))
            || reslog_post_pack (monitor->ctx->reslog,
                                 NULL,
                                 0.,
                                 "online",
                                 "{s:s}",
                                 "idset",
                                 online) < 0) {
            flux_log_error (h, "monitor: error posting online event");
            goto done;
        }
    }
    if (idset_count (leave) > 0) {
        if (!(offline = idset_encode (leave, IDSET_FLAG_RANGE))
            || reslog_post_pack (monitor->ctx->reslog,
                                 NULL,
                                 0.,
                                 "offline",
                                 "{s:s}",
                                 "idset",
                                 offline) < 0) {
            flux_log_error (h, "monitor: error posting offline event");
            goto done;
        }
    }
    idset_destroy (monitor->up);
    monitor->up = new_up;
    new_up = NULL;
    notify_waitup (monitor);
done:
    free (online);
    free (offline);
    idset_destroy (join);
    idset_destroy (leave);
    idset_destroy (new_up);
    flux_future_reset (f);
}

static void notify_waitup (struct monitor *monitor)
{
    const flux_msg_t *msg;
    int upcount = idset_count (monitor->up);

    msg = flux_msglist_first (monitor->waitup_requests);
    while (msg) {
        int upwant;
        int rc;
        if (flux_request_unpack (msg, NULL, "{s:i}", "up", &upwant) < 0)
            rc = flux_respond_error (monitor->ctx->h, msg, errno, NULL);
        else if (upwant == upcount)
            rc = flux_respond (monitor->ctx->h, msg, NULL);
        else
            goto next;
        if (rc < 0)
            flux_log_error (monitor->ctx->h,
                            "error responding to monitor-waitup request");
        flux_msglist_delete (monitor->waitup_requests);
next:
        msg = flux_msglist_next (monitor->waitup_requests);
    }
}

static void waitup_cb (flux_t *h,
                       flux_msg_handler_t *mh,
                       const flux_msg_t *msg,
                       void *arg)
{
    struct monitor *monitor = arg;
    const char *errstr = NULL;
    int up;

    if (flux_request_unpack (msg, NULL, "{s:i}", "up", &up) < 0)
        goto error;
    if (monitor->ctx->rank != 0) {
        errno = EPROTO;
        errstr = "this RPC only works on rank 0";
        goto error;
    }
    if (up > monitor->size || up < 0) {
        errno = EPROTO;
        errstr = "up value is out of range";
        goto error;
    }
    if (idset_count (monitor->up) != up) {
        if (flux_msglist_append (monitor->waitup_requests, msg) < 0)
            goto error;
        return; // response deferred
    }
    if (flux_respond (h, msg, NULL) < 0)
        flux_log_error (h, "error responding to monitor-waitup request");
    return;
error:
    if (flux_respond_error (h, msg, errno, errstr) < 0)
        flux_log_error (h, "error responding to monitor-waitup request");
}

static const struct flux_msg_handler_spec htab[] = {
    { FLUX_MSGTYPE_REQUEST,  "resource.monitor-waitup", waitup_cb, 0 },
    FLUX_MSGHANDLER_TABLE_END,
};

void monitor_destroy (struct monitor *monitor)
{
    if (monitor) {
        int saved_errno = errno;
        flux_msg_handler_delvec (monitor->handlers);
        idset_destroy (monitor->up);
        idset_destroy (monitor->down);
        flux_future_destroy (monitor->f);
        flux_msglist_destroy (monitor->waitup_requests);
        free (monitor);
        errno = saved_errno;
    }
}

struct monitor *monitor_create (struct resource_ctx *ctx,
                                int inventory_size,
                                bool monitor_force_up)
{
    struct monitor *monitor;

    if (!(monitor = calloc (1, sizeof (*monitor))))
        return NULL;
    monitor->ctx = ctx;
    /* In recovery mode, if the instance was started by PMI, the size of
     * the recovery instance will be 1 but the resource inventory size may be
     * larger.  Up/down sets should be built with the inventory size in this
     * case.  However, we cannot unconditionally use the inventory size, since
     * it will be zero at this point if resources are being dynamically
     * discovered, e.g. when Flux is launched by a foreign resource manager.
     */
    monitor->size = ctx->size;
    if (monitor->size < inventory_size)
        monitor->size = inventory_size;
    if (flux_msg_handler_addvec (ctx->h, htab, monitor, &monitor->handlers) < 0)
        goto error;
    if (!(monitor->waitup_requests = flux_msglist_create ()))
        goto error;
    if (ctx->rank == 0) {
        /* Initialize up to the empty set unless 'monitor_force_up' is true.
         * N.B. Initial up value will appear in 'restart' event posted
         * to resource.eventlog.
         */
        if (!(monitor->up = idset_create (monitor->size, 0)))
            goto error;
        if (monitor_force_up) {
            if (idset_range_set (monitor->up, 0, monitor->size - 1) < 0)
                goto error;
        }
        else if (!flux_attr_get (ctx->h, "broker.recovery-mode")) {
            if (!(monitor->f = flux_rpc_pack (ctx->h,
                                              "groups.get",
                                               FLUX_NODEID_ANY,
                                               FLUX_RPC_STREAMING,
                                               "{s:s}",
                                               "name", "broker.online"))
                || flux_future_then (monitor->f,
                                     -1,
                                     broker_online_cb,
                                     monitor) < 0)
                goto error;
        }
    }
    return monitor;
error:
    monitor_destroy (monitor);
    return NULL;
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
