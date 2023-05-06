/************************************************************\
 * Copyright 2018 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#ifndef _SUBPROCESS_SERVER_H
#define _SUBPROCESS_SERVER_H

#include "subprocess.h"

int server_start (flux_subprocess_server_t *s);

void server_stop (flux_subprocess_server_t *s);

int server_signal_subprocesses (flux_subprocess_server_t *s, int signum);

int server_terminate_subprocesses (flux_subprocess_server_t *s);

int server_terminate_by_uuid (flux_subprocess_server_t *s,
                              const char *id);

int server_terminate_setup (flux_subprocess_server_t *s,
                            double wait_time);

void server_terminate_cleanup (flux_subprocess_server_t *s);

int server_terminate_wait (flux_subprocess_server_t *s);

#endif /* !_SUBPROCESS_SERVER_H */
