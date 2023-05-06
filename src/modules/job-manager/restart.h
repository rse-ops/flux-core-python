/************************************************************\
 * Copyright 2018 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#ifndef _FLUX_JOB_MANAGER_RESTART_H
#define _FLUX_JOB_MANAGER_RESTART_H

#include <flux/core.h>

#include "job-manager.h"

int restart_from_kvs (struct job_manager *ctx);

/* exposed for unit testing only */
int restart_count_char (const char *s, char c);

int restart_save_state (struct job_manager *ctx);

int restart_save_state_to_txn (struct job_manager *ctx, flux_kvs_txn_t *txn);


#endif /* _FLUX_JOB_MANAGER_RESTART_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

