/************************************************************\
 * Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#ifndef _UTIL_GETIP_H
#define _UTIL_GETIP_H

#include <sys/socket.h> // for AF_INET, AF_INET6

/* Guess at a usable network address for the local node using one
 * of these methods:
 * 1. Find the interface associated with the default route, then
 *    look up address of that interface.
 * 2. Look up address associated with the hostname
 *
 * Main use case: determine bind address for a PMI-bootstrapped flux broker.
 *
 * Some environment variables alter the default behavior:
 *
 * FLUX_IPADDR_IPV6
 *   if set, IPv6 addresses are preferred, with fallback to IPv4
 *   if unset, IPv4 addresses are preferred, with fallback to IPv6
 * FLUX_IPADDR_HOSTNAME
 *   if set, only method 2 is tried above
 *   if unset, first method 1 is tried, then if that fails, method 2 is tried
 *
 * Return address as a string in buf (up to len bytes, always null terminated)
 * Return 0 on success, -1 on error with error message written to errstr
 * if non-NULL.
 */
int ipaddr_getprimary (char *buf, int len, char *errstr, int errstrsz);

#endif /* !_UTIL_GETIP_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
