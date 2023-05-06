/************************************************************\
 * Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <getopt.h>
#include <flux/core.h>
#include <flux/optparse.h>
#include <jansson.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <argz.h>
#include <assert.h>

#include "src/common/libutil/xzmalloc.h"
#include "src/common/libutil/log.h"
#include "src/common/libutil/oom.h"
#include "src/common/libutil/digest.h"
#include "src/common/libutil/jpath.h"

const int max_idle = 99;

int cmd_list (optparse_t *p, int argc, char **argv);
int cmd_remove (optparse_t *p, int argc, char **argv);
int cmd_load (optparse_t *p, int argc, char **argv);
int cmd_reload (optparse_t *p, int argc, char **argv);
int cmd_info (optparse_t *p, int argc, char **argv);
int cmd_stats (optparse_t *p, int argc, char **argv);
int cmd_debug (optparse_t *p, int argc, char **argv);

static struct optparse_option legacy_opts[] =  {
    { .name = "rank", .key = 'r', .has_arg = 1, .arginfo = "RANK",
      .usage = "Send RPC to specified rank",
    },
    OPTPARSE_TABLE_END,
};

static struct optparse_option remove_opts[] =  {
    { .name = "rank", .key = 'r', .has_arg = 1, .arginfo = "RANK",
      .usage = "Send RPC to specified rank",
    },
    { .name = "force", .key = 'f',
      .usage = "Ignore nonexistent modules",
    },
    OPTPARSE_TABLE_END,
};

static struct optparse_option stats_opts[] =  {
    { .name = "parse", .key = 'p', .has_arg = 1, .arginfo = "OBJNAME",
      .usage = "Parse object period-delimited object name",
    },
    { .name = "scale", .key = 's', .has_arg = 1, .arginfo = "N",
      .usage = "Scale numeric JSON value by N",
    },
    { .name = "type", .key = 't', .has_arg = 1, .arginfo = "int|double",
      .usage = "Convert JSON value to specified type",
    },
    { .name = "rusage", .key = 'R', .has_arg = 0,
      .usage = "Request rusage data instead of stats",
    },
    { .name = "clear", .key = 'c', .has_arg = 0,
      .usage = "Clear stats on target rank",
    },
    { .name = "clear-all", .key = 'C', .has_arg = 0,
      .usage = "Clear stats on all ranks",
    },
    OPTPARSE_TABLE_END
};
static struct optparse_option debug_opts[] = {
    { .name = "clear",  .key = 'C',  .has_arg = 0,
      .usage = "Set debug flags to 0", },
    { .name = "set",  .key = 'S',  .has_arg = 1,
      .usage = "Set debug flags to MASK", },
    { .name = "setbit",  .key = 's',  .has_arg = 1,
      .usage = "Set one debug flag to 1", },
    { .name = "clearbit",  .key = 'c',  .has_arg = 1,
      .usage = "Set one debug flag to 0", },
    OPTPARSE_TABLE_END,
};

static struct optparse_subcommand subcommands[] = {
    { "list",
      "[OPTIONS] [module]",
      "List loaded modules",
      cmd_list,
      0,
      NULL,
    },
    { "remove",
      "[OPTIONS] module",
      "Unload module",
      cmd_remove,
      0,
      remove_opts,
    },
    { "unload",
      "[OPTIONS] module",
      "Unload module",
      cmd_remove,
      OPTPARSE_SUBCMD_HIDDEN,
      remove_opts,
    },
    { "load",
      "[OPTIONS] module",
      "Load module",
      cmd_load,
      0,
      legacy_opts,
    },
    { "reload",
      "[OPTIONS] module",
      "Reload module",
      cmd_reload,
      0,
      remove_opts,
    },
    { "info",
      "[OPTIONS] module",
      "Display module info",
      cmd_info,
      0,
      NULL
    },
    { "stats",
      "[OPTIONS] module",
      "Display stats on module",
      cmd_stats,
      0,
      stats_opts,
    },
    { "debug",
      "[OPTIONS] module",
      "Get/set module debug flags",
      cmd_debug,
      0,
      debug_opts,
    },
    OPTPARSE_SUBCMD_END
};

int usage (optparse_t *p, struct optparse_option *o, const char *optarg)
{
    struct optparse_subcommand *s;
    optparse_print_usage (p);
    fprintf (stderr, "\n");
    fprintf (stderr, "flux module subcommands:\n");
    s = subcommands;
    while (s->name) {
        if (!(s->flags & OPTPARSE_SUBCMD_HIDDEN))
            fprintf (stderr, "   %-15s %s\n", s->name, s->doc);
        s++;
    }
    exit (1);
}

int main (int argc, char *argv[])
{
    optparse_t *p;
    char *cmdusage = "COMMAND [OPTIONS]";
    int optindex;
    int exitval;

    log_init ("flux-module");

    p = optparse_create ("flux-module");

    if (optparse_set (p, OPTPARSE_USAGE, cmdusage) != OPTPARSE_SUCCESS)
        log_msg_exit ("optparse_set (USAGE)");

    if (optparse_reg_subcommands (p, subcommands) != OPTPARSE_SUCCESS)
        log_msg_exit ("optparse_reg_subcommands");

    if (optparse_set (p, OPTPARSE_OPTION_CB, "help", usage) != OPTPARSE_SUCCESS)
        log_msg_exit ("optparse_set() failed");

    if (optparse_set (p, OPTPARSE_PRINT_SUBCMDS, 0) != OPTPARSE_SUCCESS)
        log_msg_exit ("optparse_set (PRINT_SUBCMDS)");

    if ((optindex = optparse_parse_args (p, argc, argv)) < 0)
        exit (1);

    if ((exitval = optparse_run_subcommand (p, argc, argv)) < 0)
        exit (1);

    optparse_destroy (p);
    log_fini ();
    return (exitval);
}

int filesize (const char *path)
{
    struct stat sb;
    if (stat (path, &sb) < 0)
        return 0;
    return sb.st_size;
}

void module_dlerror (const char *errmsg, void *arg)
{
    log_msg ("%s", errmsg);
}

void parse_modarg (const char *arg, char **name, char **path)
{
    char *modpath = NULL;
    char *modname = NULL;

    if (strchr (arg, '/')) {
        if (!(modpath = realpath (arg, NULL)))
            log_err_exit ("%s", arg);
        if (!(modname = flux_modname (modpath, module_dlerror, NULL)))
            log_msg_exit ("%s", modpath);
    } else {
        char *searchpath = getenv ("FLUX_MODULE_PATH");
        if (!searchpath)
            log_msg_exit ("FLUX_MODULE_PATH is not set");
        modname = xstrdup (arg);
        if (!(modpath = flux_modfind (searchpath, modname,
                                      module_dlerror, NULL)))
            log_msg_exit ("%s: not found in module search path", modname);
    }
    *name = modname;
    *path = modpath;
}

int cmd_info (optparse_t *p, int argc, char **argv)
{
    char *modpath = NULL;
    char *modname = NULL;
    char *digest = NULL;
    int n;

    if ((n = optparse_option_index (p)) != argc - 1) {
        optparse_print_usage (p);
        exit (1);
    }
    parse_modarg (argv[n], &modname, &modpath);
    digest = digest_file (modpath, NULL);
    printf ("Module name:  %s\n", modname);
    printf ("Module path:  %s\n", modpath);
    printf ("SHA1 Digest:  %s\n", digest);
    printf ("Size:         %d bytes\n", filesize (modpath));

    free (modpath);
    free (modname);
    free (digest);
    return (0);
}

/* Derive name of module loading service from module name.
 * Caller must free.
 */
char *getservice (const char *modname)
{
    char *service = NULL;
    if (strchr (modname, '.')) {
        service = xstrdup (modname);
        char *p = strrchr (service, '.');
        *p = '\0';
    } else
        service = xstrdup ("broker");
    return service;
}

static void module_load (flux_t *h, optparse_t *p, int argc, char **argv)
{
    char *modname;
    char *modpath;
    int n;
    flux_future_t *f;

    if ((n = optparse_option_index (p)) == argc) {
        optparse_print_usage (p);
        exit (1);
    }
    parse_modarg (argv[n++], &modname, &modpath);

    char *service = getservice (modname);
    char *topic = xasprintf ("%s.insmod", service);
    json_t *args = json_array ();
    if (!args)
        log_msg_exit ("json_array() failed");
    while (n < argc) {
        json_t *str = json_string (argv[n]);
        if (!str || json_array_append_new (args, str) < 0)
            log_msg_exit ("json_string() or json_array_append_new() failed");
        n++;
    }

    if (!(f = flux_rpc_pack (h,
                             topic,
                             optparse_get_int (p, "rank", FLUX_NODEID_ANY),
                             0,
                             "{s:s s:O}",
                             "path",
                             modpath,
                             "args",
                             args)))
        log_err_exit ("%s", topic);
    if (flux_rpc_get (f, NULL) < 0) {
        if (errno == EEXIST)
            log_msg_exit ("%s: %s module/service is in use", topic, modname);
        log_err_exit ("%s", topic);
    }
    flux_future_destroy (f);
    free (topic);
    free (service);
    json_decref (args);
    free (modpath);
    free (modname);
}

int cmd_load (optparse_t *p, int argc, char **argv)
{
    flux_t *h;
    if (!(h = flux_open (NULL, 0)))
        log_err_exit ("flux_open");
    module_load (h, p, argc, argv);
    flux_close (h);
    return 0;
}

static void module_remove (flux_t *h, const char *modname, optparse_t *p)
{
    flux_future_t *f;
    char *service = getservice (modname);
    char *topic = xasprintf ("%s.rmmod", service);

    if (!(f = flux_rpc_pack (h,
                             topic,
                             optparse_get_int (p, "rank", FLUX_NODEID_ANY),
                             0,
                             "{s:s}",
                             "name",
                             modname)))
        log_err_exit ("%s %s", topic, modname);
    if (flux_rpc_get (f, NULL) < 0) {
        if (!(optparse_hasopt (p, "force") && errno == ENOENT))
            log_err_exit ("%s %s", topic, modname);
    }
    flux_future_destroy (f);
    free (topic);
    free (service);
}

int cmd_remove (optparse_t *p, int argc, char **argv)
{
    char *modname;
    flux_t *h;
    int n;

    if ((n = optparse_option_index (p)) != argc - 1) {
        optparse_print_usage (p);
        exit (1);
    }
    modname = argv[n++];

    if (!(h = flux_open (NULL, 0)))
        log_err_exit ("flux_open");

    module_remove (h, modname, p);

    flux_close (h);
    return (0);
}

int cmd_reload (optparse_t *p, int argc, char **argv)
{
    char *modname;
    char *modpath;
    flux_t *h;
    int n;

    if ((n = optparse_option_index (p)) == argc) {
        optparse_print_usage (p);
        exit (1);
    }
    parse_modarg (argv[n], &modname, &modpath);

    if (!(h = flux_open (NULL, 0)))
        log_err_exit ("flux_open");

    module_remove (h, modname, p);
    module_load (h, p, argc, argv);
    return (0);
}

/* Translate an array of service names to a comma-delimited string.
 * If list is empty, NULL is returned.
 * If skip != NULL, skip over that name in the list (intended to be
 * the module name, implicitly registered as a service).
 * Caller must free result.
 */
char *lsmod_services_string (json_t *services, const char *skip)
{
    size_t index;
    json_t *value;
    char *argz = NULL;
    size_t argz_len = 0;

    json_array_foreach (services, index, value) {
        const char *name = json_string_value (value);
        if (name && (!skip || strcmp (name, skip) != 0))
            if (argz_add (&argz, &argz_len, name) != 0)
                oom ();
    }
    if (argz)
        argz_stringify (argz, argz_len, ',');
    return argz;
}

char *lsmod_idle_string (int idle, char *buf, int bufsz)
{
    if (idle <= max_idle)
        snprintf (buf, bufsz, "%d", idle);
    else
        snprintf (buf, bufsz, "idle");
    return buf;
}

char lsmod_state_char (int state)
{
    switch (state) {
    case FLUX_MODSTATE_INIT:
        return 'I';
    case FLUX_MODSTATE_RUNNING:
        return 'R';
    case FLUX_MODSTATE_FINALIZING:
        return 'F';
    case FLUX_MODSTATE_EXITED:
        return 'X';
    default:
        return '?';
    }
}

void lsmod_print_header (FILE *f)
{
    fprintf (f, "%-24s %8s %-7s %4s  %c %s\n",
            "Module", "Size", "Digest", "Idle", 'S', "Service");
}

void lsmod_print_entry (FILE *f,
                       const char *name,
                       int size,
                       const char *digest,
                       int idle,
                       int status,
                       json_t *services)
{
    int digest_len = strlen (digest);
    char *serv_s = lsmod_services_string (services, name);
    char idle_s[16];

    fprintf (f, "%-24.24s %8d %7s %4s  %c %s\n",
             name,
             size,
             digest_len > 7 ? digest + digest_len - 7 : digest,
             lsmod_idle_string (idle, idle_s, sizeof (idle_s)),
             lsmod_state_char (status),
             serv_s ? serv_s : "");

    free (serv_s);
}

void lsmod_print_list (FILE *f, json_t *o)
{
    size_t index;
    json_t *value;
    const char *name;
    int size;
    const char *digest;
    int idle;
    int status;
    json_t *services;

    json_array_foreach (o, index, value) {
        if (json_unpack (value, "{s:s s:i s:s s:i s:i s:o}",
                         "name", &name,
                         "size", &size,
                         "digest", &digest,
                         "idle", &idle,
                         "status", &status,
                         "services", &services) < 0)
            log_msg_exit ("Erorr parsing lsmod response");
        if (!json_is_array (services))
            log_msg_exit ("Erorr parsing lsmod services array");
        lsmod_print_entry (f,
                           name,
                           size,
                           digest,
                           idle,
                           status,
                           services);
    }
}

/* Fetch lsmod data from one or more ranks.  Each lsmod response returns
 * an array of module records.  The records are merged to produce a hash
 * by module SHA1 digest (computed over module file) + services list.
 * Each hash entry is then displayed as a line of output.
 */
int cmd_list (optparse_t *p, int argc, char **argv)
{
    char *service = "broker";
    char *topic;
    flux_future_t *f;
    flux_t *h;
    int n;
    json_t *o;

    if ((n = optparse_option_index (p)) < argc - 1) {
        optparse_print_usage (p);
        exit (1);
    }
    if (n < argc)
        service = argv[n++];
    if (!(h = flux_open (NULL, 0)))
        log_err_exit ("flux_open");

    topic = xasprintf ("%s.lsmod", service);
    if (!(f = flux_rpc (h, topic, NULL, FLUX_NODEID_ANY, 0)))
        log_err_exit ("%s", topic);
    if (flux_rpc_get_unpack (f, "{s:o}", "mods", &o) < 0)
        log_err_exit ("%s", topic);
    if (!json_is_array (o))
        log_msg_exit ("%s: module list is not an array", topic);
    lsmod_print_header (stdout);
    lsmod_print_list (stdout, o);
    flux_future_destroy (f);
    free (topic);
    flux_close (h);
    return (0);
}

static void parse_json (optparse_t *p, const char *json_str)
{
    json_t *obj, *o;
    const char *objname, *typestr;
    double scale;

    if (!(obj = json_loads (json_str, 0, NULL)))
        log_msg_exit ("error parsing JSON response");

    /* If --parse OBJNAME was provided, walk to that
     * portion of the returned object.
     */
    if ((objname = optparse_get_str (p, "parse", NULL))) {
        if (!(o = jpath_get (obj, objname)))
            log_msg_exit ("`%s' not found in response", objname);
    }
    else
        o = obj;

    /* Display the resulting object/value, optionally forcing
     * the type to int or dobule, and optionally scaling the result.
     */
    scale = optparse_get_double (p, "scale", 1.0);
    typestr = optparse_get_str (p, "type", NULL);
    if (json_typeof (o) == JSON_INTEGER || (typestr && !strcmp (typestr, "int"))) {
        double d = json_number_value (o);
        printf ("%d\n", (int)(d * scale));
    } else if (json_typeof (o) == JSON_REAL || (typestr && !strcmp (typestr, "double"))) {
        double d = json_number_value (o);
        printf ("%lf\n", d * scale);
    } else {
        char *s;
        s = json_dumps (o, JSON_INDENT(1) | JSON_ENCODE_ANY);
        printf ("%s\n", s ? s : "Error encoding JSON");
        free (s);
    }

    json_decref (obj);
}

int cmd_stats (optparse_t *p, int argc, char **argv)
{
    int n;
    char *topic = NULL;
    char *service;
    uint32_t nodeid;
    const char *json_str;
    flux_future_t *f = NULL;
    flux_t *h;

    if ((n = optparse_option_index (p)) < argc - 1) {
        optparse_print_usage (p);
        exit (1);
    }
    service = n < argc ? argv[n++] : "broker";
    nodeid = FLUX_NODEID_ANY;

    if (!(h = flux_open (NULL, 0)))
        log_err_exit ("flux_open");

    if (optparse_hasopt (p, "clear")) {
        topic = xasprintf ("%s.stats-clear", service);
        if (!(f = flux_rpc (h, topic, NULL, nodeid, 0)))
            log_err_exit ("%s", topic);
        if (flux_future_get (f, NULL) < 0)
            log_err_exit ("%s", topic);
    } else if (optparse_hasopt (p, "clear-all")) {
        topic = xasprintf ("%s.stats-clear", service);
        flux_msg_t *msg = flux_event_encode (topic, NULL);
        if (!msg)
            log_err_exit ("creating event");
        if (flux_send (h, msg, 0) < 0)
            log_err_exit ("sending event");
        flux_msg_destroy (msg);
    } else if (optparse_hasopt (p, "rusage")) {
        topic = xasprintf ("%s.rusage", service);
        if (!(f = flux_rpc (h, topic, NULL, nodeid, 0)))
            log_err_exit ("%s", topic);
        if (flux_rpc_get (f, &json_str) < 0)
            log_err_exit ("%s", topic);
        if (!json_str)
            log_errn_exit (EPROTO, "%s", topic);
        parse_json (p, json_str);
    } else {
        topic = xasprintf ("%s.stats-get", service);
        if (!(f = flux_rpc (h, topic, NULL, nodeid, 0)))
            log_err_exit ("%s", topic);
        if (flux_rpc_get (f, &json_str) < 0)
            log_err_exit ("%s", topic);
        if (!json_str)
            log_errn_exit (EPROTO, "%s", topic);
        parse_json (p, json_str);
    }
    free (topic);
    flux_future_destroy (f);
    flux_close (h);
    return (0);
}

int cmd_debug (optparse_t *p, int argc, char **argv)
{
    int n;
    flux_t *h;
    char *topic = NULL;
    const char *op = "setbit";
    int flags = 0;
    flux_future_t *f = NULL;

    if ((n = optparse_option_index (p)) != argc - 1)
        log_msg_exit ("flux-debug requires service argument");
    topic = xasprintf ("%s.debug", argv[n]);

    if (!(h = flux_open (NULL, 0)))
        log_err_exit ("flux_open");
    if (optparse_hasopt (p, "clear")) {
        op = "clr";
    }
    else if (optparse_hasopt (p, "set")) {
        op = "set";
        flags = strtoul (optparse_get_str (p, "set", NULL), NULL, 0);
    }
    else if (optparse_hasopt (p, "clearbit")) {
        op = "clrbit";
        flags = strtoul (optparse_get_str (p, "clearbit", NULL), NULL, 0);
    }
    else if (optparse_hasopt (p, "setbit")) {
        op = "setbit";
        flags = strtoul (optparse_get_str (p, "setbit", NULL), NULL, 0);
    }
    if (!(f = flux_rpc_pack (h, topic, FLUX_NODEID_ANY, 0, "{s:s s:i}",
                             "op", op, "flags", flags)))
        log_err_exit ("%s", topic);
    if (flux_rpc_get_unpack (f, "{s:i}", "flags", &flags) < 0)
        log_err_exit ("%s", topic);
    printf ("0x%x\n", flags);
    flux_future_destroy (f);
    flux_close (h);
    free (topic);
    return (0);
}

/*
 * vi: ts=4 sw=4 expandtab
 */
