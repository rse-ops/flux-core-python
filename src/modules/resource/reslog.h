/************************************************************\
 * Copyright 2020 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#ifndef _FLUX_RESOURCE_RESLOG_H
#define _FLUX_RESOURCE_RESLOG_H

struct reslog;

typedef void (*reslog_cb_f)(struct reslog *reslog,
                            const char *name,
                            void *arg);

struct reslog *reslog_create (flux_t *h);
void reslog_destroy (struct reslog *reslog);

/* Post an event to the eventlog.  This function returns immediately,
 * and the commit to * the eventlog completes asynchronously.
 * If 'request' is non-NULL, a success/fail response is sent upon commit
 * completion.
 */
int reslog_post_pack (struct reslog *reslog,
                      const flux_msg_t *request,
                      double timestamp,
                      const char *name,
                      const char *fmt,
                      ...);

/* Force all pending commits to the eventlog to complete.
 */
int reslog_sync (struct reslog *reslog);

/* Get a callback for each event.
 */
void reslog_set_callback (struct reslog *reslog, reslog_cb_f cb, void *arg);

#define RESLOG_KEY "resource.eventlog"

#endif /* !_FLUX_RESOURCE_RESLOG_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
