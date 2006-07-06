/* $Id$ */
#ifndef __CORE_H__
#define __CORE_H__
/*
 * core.h
 */
#include "conf.h"

//#include "apr.h"
#include "apr_hash.h"
#include "pcreposix.h"

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

#define OPT_NONE 0
#define OPT_INDEXES 1
#define OPT_INCLUDES 2
#define OPT_SYM_LINKS 4
#define OPT_EXECCGI 8
#define OPT_UNSET 16
#define OPT_INCNOEXEC 32
#define OPT_SYM_OWNER 64
#define OPT_MULTI 128
#define OPT_ALL (OPT_INDEXES|OPT_INCLUDES|OPT_SYM_LINKS|OPT_EXECCGI)


#define SATISFY_ALL 0
#define SATISFY_ANY 1
#define SATISFY_NOSPEC 2


/*
 * Core is also unlike other modules in being implemented in more than
 * one file... so, data structures are declared here, even though most of
 * the code that cares really is in http_core.c.  Also, another accessor.
 */

extern module core_httpd_module;

/* Per-request configuration */

typedef struct {
    /* bucket brigade used by getline for look-ahead and 
     * ap_get_client_block for holding left-over request body */
    struct apr_bucket_brigade *bb;

    /* an array of per-request working data elements, accessed
     * by ID using ap_get_request_note()
     * (Use ap_register_request_note() during initialization
     * to add elements)
     */
    void **notes;
} core_request_config;

/* Standard entries that are guaranteed to be accessible via
 * ap_get_request_note() for each request (additional entries
 * can be added with ap_register_request_note())
 */
#define AP_NOTE_DIRECTORY_WALK 0
#define AP_NOTE_LOCATION_WALK  1
#define AP_NOTE_FILE_WALK      2
#define AP_NUM_STD_NOTES       3

/* Per-directory configuration */

typedef unsigned char allow_options_t;
typedef unsigned char overrides_t;

/*
 * Bits of info that go into making an ETag for a file
 * document.  Why a long?  Because char historically
 * proved too short for Options, and int can be different
 * sizes on different platforms.
 */
typedef unsigned long etag_components_t;

#define ETAG_UNSET 0
#define ETAG_NONE  (1 << 0)
#define ETAG_MTIME (1 << 1)
#define ETAG_INODE (1 << 2)
#define ETAG_SIZE  (1 << 3)
#define ETAG_BACKWARD (ETAG_MTIME | ETAG_INODE | ETAG_SIZE)
#define ETAG_ALL   (ETAG_MTIME | ETAG_INODE | ETAG_SIZE)

typedef enum {
    srv_sig_unset,
    srv_sig_off,
    srv_sig_on,
    srv_sig_withmail
} server_signature_e;

typedef struct {
    /* path of the directory/regex/etc. see also d_is_fnmatch/absolute below */
    char *d;
    /* the number of slashes in d */
    unsigned d_components;

    /* If (opts & OPT_UNSET) then no absolute assignment to options has
     * been made.
     * invariant: (opts_add & opts_remove) == 0
     * Which said another way means that the last relative (options + or -)
     * assignment made to each bit is recorded in exactly one of opts_add
     * or opts_remove.
     */
    allow_options_t opts;
    allow_options_t opts_add;
    allow_options_t opts_remove;
    overrides_t override;
    
    /* MIME typing --- the core doesn't do anything at all with this,
     * but it does know what to slap on a request for a document which
     * goes untyped by other mechanisms before it slips out the door...
     */
    
    char *ap_default_type;
  
    /* Authentication stuff.  Groan... */
    
    int satisfy;
    char *ap_auth_type;
    char *ap_auth_name;
    apr_array_header_t *ap_requires;

    /* Custom response config. These can contain text or a URL to redirect to.
     * if response_code_strings is NULL then there are none in the config,
     * if it's not null then it's allocated to sizeof(char*)*RESPONSE_CODES.
     * This lets us do quick merges in merge_core_dir_configs().
     */
  
    char **response_code_strings;

    /* Hostname resolution etc */
#define HOSTNAME_LOOKUP_OFF	0
#define HOSTNAME_LOOKUP_ON	1
#define HOSTNAME_LOOKUP_DOUBLE	2
#define HOSTNAME_LOOKUP_UNSET	3
    unsigned int hostname_lookups : 4;

    signed int do_rfc1413 : 2;   /* See if client is advertising a username? */

    signed int content_md5 : 2;  /* calculate Content-MD5? */

#define USE_CANONICAL_NAME_OFF   (0)
#define USE_CANONICAL_NAME_ON    (1)
#define USE_CANONICAL_NAME_DNS   (2)
#define USE_CANONICAL_NAME_UNSET (3)
    unsigned use_canonical_name : 2;

    /* since is_fnmatch(conf->d) was being called so frequently in
     * directory_walk() and its relatives, this field was created and
     * is set to the result of that call.
     */
    unsigned d_is_fnmatch : 1;

    /* should we force a charset on any outgoing parameterless content-type?
     * if so, which charset?
     */
#define ADD_DEFAULT_CHARSET_OFF   (0)
#define ADD_DEFAULT_CHARSET_ON    (1)
#define ADD_DEFAULT_CHARSET_UNSET (2)
    unsigned add_default_charset : 2;
    const char *add_default_charset_name;

    /* System Resource Control */
#ifdef RLIMIT_CPU
    struct rlimit *limit_cpu;
#endif
#if defined (RLIMIT_DATA) || defined (RLIMIT_VMEM) || defined(RLIMIT_AS)
    struct rlimit *limit_mem;
#endif
#ifdef RLIMIT_NPROC
    struct rlimit *limit_nproc;
#endif
    apr_off_t limit_req_body;      /* limit on bytes in request msg body */
    long limit_xml_body;           /* limit on bytes in XML request msg body */

    /* logging options */

    server_signature_e server_signature;

    int loglevel;
    
    /* Access control */
    apr_array_header_t *sec_file;
    regex_t *r;

    const char *mime_type;       /* forced with ForceType  */
    const char *handler;         /* forced with SetHandler */
    const char *output_filters;  /* forced with SetOutputFilters */
    const char *input_filters;   /* forced with SetInputFilters */
    int accept_path_info;        /* forced with AcceptPathInfo */

    apr_hash_t *ct_output_filters; /* added with AddOutputFilterByType */

    /*
     * What attributes/data should be included in ETag generation?
     */
    etag_components_t etag_bits;
    etag_components_t etag_add;
    etag_components_t etag_remove;

    /*
     * Run-time performance tuning
     */
#define ENABLE_MMAP_OFF    (0)
#define ENABLE_MMAP_ON     (1)
#define ENABLE_MMAP_UNSET  (2)
    int enable_mmap;  /* whether files in this dir can be mmap'ed */

} core_dir_config;

/* Per-server core configuration */

typedef struct {
  
#ifdef GPROF
    char *gprof_dir;
#endif

    /* Name translations --- we want the core to be able to do *something*
     * so it's at least a minimally functional web server on its own (and
     * can be tested that way).  But let's keep it to the bare minimum:
     */
    const char *ap_document_root;
  
    /* Access control */

    char *access_name;
    apr_array_header_t *sec_dir;
    apr_array_header_t *sec_url;
} core_server_config;

/* for AddOutputFiltersByType in core.c */
void ap_add_output_filters_by_type(request_rec *r);

/**
 * Retrieve the value of Options for this request
 * @param r The current request
 * @return the Options bitmask
 * @deffunc int ap_allow_options(request_rec *r)
 */
AP_DECLARE(int) ap_allow_options(request_rec *r);

/**
 * Retrieve the value of the AllowOverride for this request
 * @param r The current request
 * @return the overrides bitmask
 * @deffunc int ap_allow_overrides(request_rec *r)
 */
AP_DECLARE(int) ap_allow_overrides(request_rec *r);

/* Authentication stuff.  This is one of the places where compatibility
 * with the old config files *really* hurts; they don't discriminate at
 * all between different authentication schemes, meaning that we need
 * to maintain common state for all of them in the core, and make it
 * available to the other modules through interfaces.
 */
typedef struct require_line require_line;

/** A structure to keep track of authorization requirements */
struct require_line {
    /** Where the require line is in the config file. */
    apr_int64_t method_mask;
    /** The complete string from the command line */
    char *requirement;
};

/**
 * Return the current Authorization realm
 * @param r The current request
 * @return The current authorization realm
 * @deffunc const char *ap_auth_name(request_rec *r)
 */
AP_DECLARE(const char *) ap_auth_name(request_rec *r);     

/**
 * Return the type of authorization required for this request
 * @param r The current request
 * @return The authorization required
 * @deffunc const char *ap_auth_type(request_rec *r)
 */
AP_DECLARE(const char *) ap_auth_type(request_rec *r);

/**
 * Retrieve the value of the DefaultType directive, or text/plain if not set
 * @param r The current request
 * @return The default type
 * @deffunc const char *ap_default_type(request_rec *r)
 */
AP_DECLARE(const char *) ap_default_type(request_rec *r);     

/**
 * Retrieve the document root for this server
 * @param r The current request
 * @warning Don't use this!  If your request went through a Userdir, or 
 * something like that, it'll screw you.  But it's back-compatible...
 * @return The document root
 * @deffunc const char *ap_document_root(request_rec *r)
 */

AP_DECLARE(const char *) ap_document_root(request_rec *r);
/**
 * Retrieve information about all of the requires directives for this request
 * @param r The current request
 * @return An array of all requires directives for this request
 * @deffunc const apr_array_header_t *ap_requires(request_rec *r)
 */
AP_DECLARE(const apr_array_header_t *) ap_requires(request_rec *r);    

/**
 * How the requires lines must be met.
 * @param r The current request
 * @return How the requirements must be met.  One of:
 * <pre>
 *      SATISFY_ANY    -- any of the requirements must be met.
 *      SATISFY_ALL    -- all of the requirements must be met.
 *      SATISFY_NOSPEC -- There are no applicable satisfy lines
 * </pre>
 * @deffunc int ap_satisfies(request_rec *r)
 */
AP_DECLARE(int) ap_satisfies(request_rec *r);


/**
 * Lookup the remote client's DNS name or IP address
 * @param conn The current connection
 * @param dir_config The directory config vector from the request
 * @param type The type of lookup to perform.  One of:
 * <pre>
 *     REMOTE_HOST returns the hostname, or NULL if the hostname
 *                 lookup fails.  It will force a DNS lookup according to the
 *                 HostnameLookups setting.
 *     REMOTE_NAME returns the hostname, or the dotted quad if the
 *                 hostname lookup fails.  It will force a DNS lookup according
 *                 to the HostnameLookups setting.
 *     REMOTE_NOLOOKUP is like REMOTE_NAME except that a DNS lookup is
 *                     never forced.
 *     REMOTE_DOUBLE_REV will always force a DNS lookup, and also force
 *                   a double reverse lookup, regardless of the HostnameLookups
 *                   setting.  The result is the (double reverse checked) 
 *                   hostname, or NULL if any of the lookups fail.
 * </pre>
 * @param str_is_ip unless NULL is passed, this will be set to non-zero on output when an IP address 
 *        string is returned
 * @return The remote hostname
 * @deffunc const char *ap_get_remote_host(conn_rec *conn, void *dir_config, int type, int *str_is_ip)
 */
AP_DECLARE(const char *) ap_get_remote_host(conn_rec *conn, void *dir_config, int type, int *str_is_ip);

/**
 * Retrieve the login name of the remote user.  Undef if it could not be
 * determined
 * @param r The current request
 * @return The user logged in to the client machine
 * @deffunc const char *ap_get_remote_logname(request_rec *r)
 */
AP_DECLARE(const char *) ap_get_remote_logname(request_rec *r);

/* Used for constructing self-referencing URLs, and things like SERVER_PORT,
 * and SERVER_NAME.
 */
/**
 * build a fully qualified URL from the uri and information in the request rec
 * @param p The pool to allocate the URL from
 * @param uri The path to the requested file
 * @param r The current request
 * @return A fully qualified URL
 * @deffunc char *ap_construct_url(apr_pool_t *p, const char *uri, request_rec *r)
 */
AP_DECLARE(char *) ap_construct_url(apr_pool_t *p, const char *uri, request_rec *r);

/**
 * Get the current server name from the request
 * @param r The current request
 * @return the server name
 * @deffunc const char *ap_get_server_name(request_rec *r)
 */
AP_DECLARE(const char *) ap_get_server_name(request_rec *r);

/**
 * Get the current server port
 * @param The current request
 * @return The server's port
 * @deffunc apr_port_t ap_get_server_port(const request_rec *r)
 */
AP_DECLARE(apr_port_t) ap_get_server_port(const request_rec *r);

/**
 * Return the limit on bytes in request msg body 
 * @param r The current request
 * @return the maximum number of bytes in the request msg body
 * @deffunc apr_off_t ap_get_limit_req_body(const request_rec *r)
 */
AP_DECLARE(apr_off_t) ap_get_limit_req_body(const request_rec *r);

/**
 * Return the limit on bytes in XML request msg body
 * @param r The current request
 * @return the maximum number of bytes in XML request msg body
 * @deffunc size_t ap_get_limit_xml_body(const request_rec *r)
 */
AP_DECLARE(size_t) ap_get_limit_xml_body(const request_rec *r);

AP_DECLARE(void **) ap_get_request_note(request_rec *r, apr_size_t note_num);


/*****************************************************************************/

/**
 * @defgroup ap_check_cmd_context ap_check_cmd_context
 * @{
 */
/**
 * Check the context a command is used in.
 * @param cmd The command to check
 * @param forbidden Where the command is forbidden.
 * @return Error string on error, NULL on success
 */
const char *ap_check_cmd_context(cmd_parms *cmd, unsigned forbidden);

#define  NOT_IN_VIRTUALHOST     0x01 /**< Forbidden in <Virtualhost> */
#define  NOT_IN_LIMIT           0x02 /**< Forbidden in <Limit> */
#define  NOT_IN_DIRECTORY       0x04 /**< Forbidden in <Directory> */
#define  NOT_IN_LOCATION        0x08 /**< Forbidden in <Location> */
#define  NOT_IN_FILES           0x10 /**< Forbidden in <Files> */
/** Forbidden in <Directory>/<Location>/<Files>*/
#define  NOT_IN_DIR_LOC_FILE    (NOT_IN_DIRECTORY|NOT_IN_LOCATION|NOT_IN_FILES) 
/** Forbidden in <VirtualHost>/<Limit>/<Directory>/<Location>/<Files> */
#define  GLOBAL_ONLY      (NOT_IN_VIRTUALHOST|NOT_IN_LIMIT|NOT_IN_DIR_LOC_FILE)

/** @} */

void *create_core_dir_config(apr_pool_t *a, char *dir);
void *create_core_server_config(apr_pool_t *a, server_rec *s);

extern const command_rec core_cmds[];

#endif

