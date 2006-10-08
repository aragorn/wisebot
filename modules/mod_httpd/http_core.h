/* $Id$ */
#ifndef __HTTP_CORE_H__
#define __HTTP_CORE_H__

#include "core.h"
#include "apr.h"
#include "apr_hash.h"

#if APR_HAVE_STRUCT_RLIMIT
#include <sys/time.h>
#include <sys/resource.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @package CORE HTTP Daemon
 */

/* ****************************************************************
 *
 * The most basic server code is encapsulated in a single module
 * known as the core, which is just *barely* functional enough to
 * serve documents, though not terribly well.
 *
 * Largely for NCSA back-compatibility reasons, the core needs to
 * make pieces of its config structures available to other modules.
 * The accessors are declared here, along with the interpretation
 * of one of them (allow_options).
 */


/* options for get_remote_host() */
/* REMOTE_HOST returns the hostname, or NULL if the hostname
 * lookup fails.  It will force a DNS lookup according to the
 * HostnameLookups setting.
 */
#define REMOTE_HOST (0)

/* REMOTE_NAME returns the hostname, or the dotted quad if the
 * hostname lookup fails.  It will force a DNS lookup according
 * to the HostnameLookups setting.
 */
#define REMOTE_NAME (1)

/* REMOTE_NOLOOKUP is like REMOTE_NAME except that a DNS lookup is
 * never forced.
 */
#define REMOTE_NOLOOKUP (2)

/* REMOTE_DOUBLE_REV will always force a DNS lookup, and also force
 * a double reverse lookup, regardless of the HostnameLookups
 * setting.  The result is the (double reverse checked) hostname,
 * or NULL if any of the lookups fail.
 */
#define REMOTE_DOUBLE_REV (3)


/* Make sure we don't write less than 8000 bytes at any one time.
 */
#define AP_MIN_BYTES_TO_WRITE  8000



/**
 * Install a custom response handler for a given status
 * @param r The current request
 * @param status The status for which the custom response should be used
 * @param string The custom response.  This can be a static string, a file
 *               or a URL
 */
//AP_DECLARE(void) ap_custom_response(request_rec *r, int status, const char *string);

/**
 * Check for a definition from the server command line
 * @param name The define to check for
 * @return 1 if defined, 0 otherwise
 * @deffunc int ap_exists_config_define(const char *name)
 */
//AP_DECLARE(int) ap_exists_config_define(const char *name);
/* FIXME! See STATUS about how */
AP_DECLARE_NONSTD(int) ap_core_translate(request_rec *r);

/* ----------------------------------------------------------------------
 *
 * Runtime status/management
 */

typedef enum {
    ap_mgmt_type_string,
    ap_mgmt_type_long,
    ap_mgmt_type_hash
} ap_mgmt_type_e;

typedef union {
    const char *s_value;
    long i_value;
    apr_hash_t *h_value;
} ap_mgmt_value;

typedef struct {
    const char *description;
    const char *name;
    ap_mgmt_type_e vtype;
    ap_mgmt_value v;
} ap_mgmt_item_t;

/**
 * This hook provdes a way for modules to provide metrics/statistics about
 * their operational status.
 *
 * @param p A pool to use to create entries in the hash table
 * @param val The name of the parameter(s) that is wanted. This is
 *            tree-structured would be in the form ('*' is all the tree,
 *            'module.*' all of the module , 'module.foo.*', or
 *            'module.foo.bar' )
 * @param ht The hash table to store the results. Keys are item names, and
 *           the values point to ap_mgmt_item_t structures.
 * @ingroup hooks
 */
SB_DECLARE_HOOK(int, get_mgmt_items,
                (apr_pool_t *p, const char * val, apr_hash_t *ht))

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif	/* !__HTTP_CORE_H__ */
