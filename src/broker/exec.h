/************************************************************\
 * Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#ifndef BROKER_EXEC_H
#define BROKER_EXEC_H

#include <stdint.h>
#include <flux/core.h>
#include "attr.h"

/* Send SIGTERM / SIGKILL to all subprocesses, to be called at
 * beginning of teardown of broker */
void exec_terminate_subprocesses (flux_t *h);

int exec_initialize (flux_t *h, uint32_t rank, attr_t *attrs);

#endif /* BROKER_EXEC_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
