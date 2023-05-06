/************************************************************\
 * Copyright 2020 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#ifndef _BROKER_BROKERCFG_H
#define _BROKER_BROKERCFG_H

struct brokercfg;

struct brokercfg *brokercfg_create (flux_t *h,
                                    const char *path,
                                    attr_t *attr,
                                    modhash_t *modhash);
void brokercfg_destroy (struct brokercfg *cfg);

#endif /* !_BROKER_BROKERCFG_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
