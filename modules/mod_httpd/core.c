/* $Id$ */
#include "common_core.h"
#include "protocol.h"
#include "request.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_util.h"
#include "filter.h"
#include "http_filter.h"
#include "http_core.h" /* AP_MIN_BYTES_TO_WRITE */
#include "apr_strings.h"
#include "log.h" /* ap_log_rerror */
#include "core.h"

/* LimitXMLRequestBody handling */
#define AP_LIMIT_UNSET                  ((long) -1)
#define AP_DEFAULT_LIMIT_XML_BODY       ((size_t)1000000)

#define AP_MIN_SENDFILE_BYTES           (256)

/* Server core module... This module provides support for really basic
 * server operations, including options and commands which control the
 * operation of other modules.  Consider this the bureaucracy module.
 *
 * The core module also defines handlers, etc., do handle just enough
 * to allow a server with the core module ONLY to actually serve documents
 * (though it slaps DefaultType on all of 'em); this was useful in testing,
 * but may not be worth preserving.
 *
 * This file could almost be mod_core.c, except for the stuff which affects
 * the http_conf_globals.
 */

/* Handles for core filters */
AP_DECLARE_DATA ap_filter_rec_t *ap_subreq_core_filter_handle;
AP_DECLARE_DATA ap_filter_rec_t *ap_core_output_filter_handle;
AP_DECLARE_DATA ap_filter_rec_t *ap_content_length_filter_handle;
AP_DECLARE_DATA ap_filter_rec_t *ap_net_time_filter_handle;
AP_DECLARE_DATA ap_filter_rec_t *ap_core_input_filter_handle;

void *create_core_dir_config(apr_pool_t *a, char *dir)
{
    core_dir_config *conf;

    conf = (core_dir_config *)apr_pcalloc(a, sizeof(core_dir_config));

    /* conf->r and conf->d[_*] are initialized by dirsection() or left NULL */

    conf->opts = dir ? OPT_UNSET : OPT_UNSET|OPT_ALL;
    conf->opts_add = conf->opts_remove = OPT_NONE;
    conf->override = dir ? OR_UNSET : OR_UNSET|OR_ALL;

    conf->content_md5 = 2;
    conf->accept_path_info = 3;

    conf->use_canonical_name = USE_CANONICAL_NAME_UNSET;

    conf->hostname_lookups = HOSTNAME_LOOKUP_UNSET;
    conf->do_rfc1413 = DEFAULT_RFC1413 | 2; /* set bit 1 to indicate default */
    conf->satisfy = SATISFY_NOSPEC;

#ifdef RLIMIT_CPU
    conf->limit_cpu = NULL;
#endif
#if defined(RLIMIT_DATA) || defined(RLIMIT_VMEM) || defined(RLIMIT_AS)
    conf->limit_mem = NULL;
#endif
#ifdef RLIMIT_NPROC
    conf->limit_nproc = NULL;
#endif

    conf->limit_req_body = 0;
    conf->limit_xml_body = AP_LIMIT_UNSET;
    conf->sec_file = apr_array_make(a, 2, sizeof(ap_conf_vector_t *));

    conf->server_signature = srv_sig_unset;

    conf->add_default_charset = ADD_DEFAULT_CHARSET_UNSET;
    conf->add_default_charset_name = DEFAULT_ADD_DEFAULT_CHARSET_NAME;

    /* Overriding all negotiation
     */
    conf->mime_type = NULL;
    conf->handler = NULL;
    conf->output_filters = NULL;
    conf->input_filters = NULL;

    /*
     * Flag for use of inodes in ETags.
     */
    conf->etag_bits = ETAG_UNSET;
    conf->etag_add = ETAG_UNSET;
    conf->etag_remove = ETAG_UNSET;

    conf->enable_mmap = ENABLE_MMAP_UNSET;

    return (void *)conf;
}

void *create_core_server_config(apr_pool_t *a, server_rec *s)
{
    core_server_config *conf;
    int is_virtual = s->is_virtual;

    conf = (core_server_config *)apr_pcalloc(a, sizeof(core_server_config));

#ifdef GPROF
    conf->gprof_dir = NULL;
#endif

    conf->access_name = is_virtual ? NULL : DEFAULT_ACCESS_FNAME;
    conf->ap_document_root = is_virtual ? NULL : DOCUMENT_LOCATION;
    conf->sec_dir = apr_array_make(a, 40, sizeof(ap_conf_vector_t *));
    conf->sec_url = apr_array_make(a, 40, sizeof(ap_conf_vector_t *));

    return (void *)conf;
}

/* Should probably just get rid of this... the only code that cares is
 * part of the core anyway (and in fact, it isn't publicised to other
 * modules).
 */

char *ap_response_code_string(request_rec *r, int error_index)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   CORE_HTTPD_MODULE);

    if (conf->response_code_strings == NULL) {
        return NULL;
    }

    return conf->response_code_strings[error_index];
}

/*****************************************************************
 *
 * There are some elements of the core config structures in which
 * other modules have a legitimate interest (this is ugly, but necessary
 * to preserve NCSA back-compatibility).  So, we have a bunch of accessors
 * here...
 */

AP_DECLARE(int) ap_allow_options(request_rec *r)
{
    core_dir_config *conf = (core_dir_config *)
				ap_get_module_config(r->per_dir_config, CORE_HTTPD_MODULE);

    return conf->opts;
}

AP_DECLARE(int) ap_allow_overrides(request_rec *r)
{
    core_dir_config *conf;
    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   CORE_HTTPD_MODULE);

    return conf->override;
}

AP_DECLARE(const char *) ap_auth_type(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   CORE_HTTPD_MODULE);

    return conf->ap_auth_type;
}

AP_DECLARE(const char *) ap_auth_name(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   CORE_HTTPD_MODULE);

    return conf->ap_auth_name;
}

AP_DECLARE(const char *) ap_default_type(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   CORE_HTTPD_MODULE);

    return conf->ap_default_type
               ? conf->ap_default_type
               : DEFAULT_CONTENT_TYPE;
}

AP_DECLARE(const char *) ap_document_root(request_rec *r) /* Don't use this! */
{
    core_server_config *conf;

    conf = (core_server_config *)
		ap_get_module_config(r->server->module_config, CORE_HTTPD_MODULE);

    return conf->ap_document_root;
}

AP_DECLARE(const apr_array_header_t *) ap_requires(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   CORE_HTTPD_MODULE);

    return conf->ap_requires;
}

AP_DECLARE(int) ap_satisfies(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   CORE_HTTPD_MODULE);

    return conf->satisfy;
}


/* Code from Harald Hanche-Olsen <hanche@imf.unit.no> */
static APR_INLINE void do_double_reverse (conn_rec *conn)
{
    apr_sockaddr_t *sa;
    apr_status_t rv;

    if (conn->double_reverse) {
        /* already done */
        return;
    }

    if (conn->remote_host == NULL || conn->remote_host[0] == '\0') {
        /* single reverse failed, so don't bother */
        conn->double_reverse = -1;
        return;
    }

    rv = apr_sockaddr_info_get(&sa, conn->remote_host, APR_UNSPEC, 0, 0, conn->pool);
    if (rv == APR_SUCCESS) {
        while (sa) {
            if (apr_sockaddr_equal(sa, conn->remote_addr)) {
                conn->double_reverse = 1;
                return;
            }

            sa = sa->next;
        }
    }

    conn->double_reverse = -1;
}

AP_DECLARE(const char *) ap_get_remote_host(conn_rec *conn, void *dir_config,
                                            int type, int *str_is_ip)
{
    int hostname_lookups;

    if (str_is_ip) { /* if caller wants to know */
        *str_is_ip = 0;
    }

    /* If we haven't checked the host name, and we want to */
    if (dir_config) {
        hostname_lookups =
            ((core_dir_config *)ap_get_module_config(dir_config, CORE_HTTPD_MODULE))
            ->hostname_lookups;

        if (hostname_lookups == HOSTNAME_LOOKUP_UNSET) {
            hostname_lookups = HOSTNAME_LOOKUP_OFF;
        }
    }
    else {
        /* the default */
        hostname_lookups = HOSTNAME_LOOKUP_OFF;
    }

    if (type != REMOTE_NOLOOKUP
        && conn->remote_host == NULL
        && (type == REMOTE_DOUBLE_REV
        || hostname_lookups != HOSTNAME_LOOKUP_OFF)) {

        if (apr_getnameinfo(&conn->remote_host, conn->remote_addr, 0)
            == APR_SUCCESS) {
            ap_str_tolower(conn->remote_host);

            if (hostname_lookups == HOSTNAME_LOOKUP_DOUBLE) {
                do_double_reverse(conn);
                if (conn->double_reverse != 1) {
                    conn->remote_host = NULL;
                }
            }
        }

        /* if failed, set it to the NULL string to indicate error */
        if (conn->remote_host == NULL) {
            conn->remote_host = "";
        }
    }

    if (type == REMOTE_DOUBLE_REV) {
        do_double_reverse(conn);
        if (conn->double_reverse == -1) {
            return NULL;
        }
    }

    /*
     * Return the desired information; either the remote DNS name, if found,
     * or either NULL (if the hostname was requested) or the IP address
     * (if any identifier was requested).
     */
    if (conn->remote_host != NULL && conn->remote_host[0] != '\0') {
        return conn->remote_host;
    }
    else {
        if (type == REMOTE_HOST || type == REMOTE_DOUBLE_REV) {
            return NULL;
        }
        else {
            if (str_is_ip) { /* if caller wants to know */
                *str_is_ip = 1;
            }

            return conn->remote_ip;
        }
    }
}

AP_DECLARE(const char *) ap_get_remote_logname(request_rec *r)
{
    core_dir_config *dir_conf;

    if (r->connection->remote_logname != NULL) {
        return r->connection->remote_logname;
    }

    /* If we haven't checked the identity, and we want to */
    dir_conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                       CORE_HTTPD_MODULE);

    if (dir_conf->do_rfc1413 & 1) {
		/* XXX Adopting ap_rfc1413.[ch] requires some hacking. */
        //return ap_rfc1413(r->connection, r->server);
        return NULL;
    }
    else {
        return NULL;
    }
}

AP_DECLARE(apr_off_t) ap_get_limit_req_body(const request_rec *r)
{
    core_dir_config *d =
      (core_dir_config *)ap_get_module_config(r->per_dir_config,
											  CORE_HTTPD_MODULE);

    return d->limit_req_body;
}

/*****************************************************************
 *
 * Commands... this module handles almost all of the NCSA httpd.conf
 * commands, but most of the old srm.conf is in the the modules.
 */

/* returns a parent if it matches the given directive */
static const ap_directive_t * find_parent(const ap_directive_t *dirp,
                                          const char *what)
{
    while (dirp->parent != NULL) {
        dirp = dirp->parent;

        /* ### it would be nice to have atom-ized directives */
        if (strcasecmp(dirp->directive, what) == 0)
            return dirp;
    }

    return NULL;
}

AP_DECLARE(const char *) ap_check_cmd_context(cmd_parms *cmd,
                                              unsigned forbidden)
{
    const char *gt = (cmd->cmd->name[0] == '<'
                      && cmd->cmd->name[strlen(cmd->cmd->name)-1] != '>')
                         ? ">" : "";
    const ap_directive_t *found;

    if ((forbidden & NOT_IN_VIRTUALHOST) && cmd->server->is_virtual) {
        return apr_pstrcat(cmd->pool, cmd->cmd->name, gt,
                           " cannot occur within <VirtualHost> section", NULL);
    }

    if ((forbidden & NOT_IN_LIMIT) && cmd->limited != -1) {
        return apr_pstrcat(cmd->pool, cmd->cmd->name, gt,
                           " cannot occur within <Limit> section", NULL);
    }

    if ((forbidden & NOT_IN_DIR_LOC_FILE) == NOT_IN_DIR_LOC_FILE) {
        if (cmd->path != NULL) {
            return apr_pstrcat(cmd->pool, cmd->cmd->name, gt,
                            " cannot occur within <Directory/Location/Files> "
                            "section", NULL);
        }
        if (cmd->cmd->req_override & EXEC_ON_READ) {
            /* EXEC_ON_READ must be NOT_IN_DIR_LOC_FILE, if not, it will
             * (deliberately) segfault below in the individual tests...
             */
            return NULL;
        }
    }
    
    if (((forbidden & NOT_IN_DIRECTORY)
         && ((found = find_parent(cmd->directive, "<Directory"))
             || (found = find_parent(cmd->directive, "<DirectoryMatch"))))
        || ((forbidden & NOT_IN_LOCATION)
            && ((found = find_parent(cmd->directive, "<Location"))
                || (found = find_parent(cmd->directive, "<LocationMatch"))))
        || ((forbidden & NOT_IN_FILES)
            && ((found = find_parent(cmd->directive, "<Files"))
                || (found = find_parent(cmd->directive, "<FilesMatch"))))) {
        return apr_pstrcat(cmd->pool, cmd->cmd->name, gt,
                           " cannot occur within ", found->directive,
                           "> section", NULL);
    }

    return NULL;
}








#if 0
static const char *set_timeout(cmd_parms *cmd, void *dummy, const char *arg)
{
    const char *err = ap_check_cmd_context(cmd, NOT_IN_DIR_LOC_FILE|NOT_IN_LIMIT);
    float strtof(const char *, char **);
    double floor(double);
    double fmod(double, double);
    float t;

    if (err != NULL) {
        return err;
    }

    debug("setting timeout to %d\n", atoi(arg));

    /* cmd->server->timeout = apr_time_from_sec(atoi(arg)); */
    t = strtof(arg, NULL);

    /* the precedence is wrong in apr_time_from_sec() to handle fractional seconds... */
    cmd->server->timeout = apr_time_make(floor(t), fmod(t, 1.0) * 1000000);
    return NULL;
}
#endif

static const char *set_timeout(cmd_parms *cmd, void *dummy, const char *arg)
{
    const char *err = ap_check_cmd_context(cmd, NOT_IN_DIR_LOC_FILE|NOT_IN_LIMIT);

    if (err != NULL) {
        return err;
    }

    cmd->server->timeout = apr_time_from_sec(atoi(arg));

    return NULL;
}


/*****************************************************************************/
const command_rec core_cmds[] = {
#if 0
/* Old access config file commands */

AP_INIT_RAW_ARGS("<Directory", dirsection, NULL, RSRC_CONF,
  "Container for directives affecting resources located in the specified "
  "directories"),
AP_INIT_RAW_ARGS("<Location", urlsection, NULL, RSRC_CONF,
  "Container for directives affecting resources accessed through the "
  "specified URL paths"),
AP_INIT_RAW_ARGS("<Files", filesection, NULL, OR_ALL,
  "Container for directives affecting files matching specified patterns"),
AP_INIT_RAW_ARGS("<Limit", ap_limit_section, NULL, OR_ALL,
  "Container for authentication directives when accessed using specified HTTP "
  "methods"),
AP_INIT_RAW_ARGS("<LimitExcept", ap_limit_section, (void*)1, OR_ALL,
  "Container for authentication directives to be applied when any HTTP "
  "method other than those specified is used to access the resource"),
AP_INIT_TAKE1("<IfModule", start_ifmod, NULL, EXEC_ON_READ | OR_ALL,
  "Container for directives based on existance of specified modules"),
AP_INIT_TAKE1("<IfDefine", start_ifdefine, NULL, EXEC_ON_READ | OR_ALL,
  "Container for directives based on existance of command line defines"),
AP_INIT_RAW_ARGS("<DirectoryMatch", dirsection, (void*)1, RSRC_CONF,
  "Container for directives affecting resources located in the "
  "specified directories"),
AP_INIT_RAW_ARGS("<LocationMatch", urlsection, (void*)1, RSRC_CONF,
  "Container for directives affecting resources accessed through the "
  "specified URL paths"),
AP_INIT_RAW_ARGS("<FilesMatch", filesection, (void*)1, OR_ALL,
  "Container for directives affecting files matching specified patterns"),
AP_INIT_TAKE1("AuthType", ap_set_string_slot,
  (void*)APR_OFFSETOF(core_dir_config, ap_auth_type), OR_AUTHCFG,
  "An HTTP authorization type (e.g., \"Basic\")"),
AP_INIT_TAKE1("AuthName", set_authname, NULL, OR_AUTHCFG,
  "The authentication realm (e.g. \"Members Only\")"),
AP_INIT_RAW_ARGS("Require", require, NULL, OR_AUTHCFG,
  "Selects which authenticated users or groups may access a protected space"),
AP_INIT_TAKE1("Satisfy", satisfy, NULL, OR_AUTHCFG,
  "access policy if both allow and require used ('all' or 'any')"),
#ifdef GPROF
AP_INIT_TAKE1("GprofDir", set_gprof_dir, NULL, RSRC_CONF,
  "Directory to plop gmon.out files"),
#endif
AP_INIT_TAKE1("AddDefaultCharset", set_add_default_charset, NULL, OR_FILEINFO,
  "The name of the default charset to add to any Content-Type without one or 'Off' to disable"),
AP_INIT_TAKE1("AcceptPathInfo", set_accept_path_info, NULL, OR_FILEINFO,
  "Set to on or off for PATH_INFO to be accepted by handlers, or default for the per-handler preference"),

/* Old resource config file commands */

AP_INIT_RAW_ARGS("AccessFileName", set_access_name, NULL, RSRC_CONF,
  "Name(s) of per-directory config files (default: .htaccess)"),
AP_INIT_TAKE1("DocumentRoot", set_document_root, NULL, RSRC_CONF,
  "Root directory of the document tree"),
AP_INIT_TAKE2("ErrorDocument", set_error_document, NULL, OR_FILEINFO,
  "Change responses for HTTP errors"),
AP_INIT_RAW_ARGS("AllowOverride", set_override, NULL, ACCESS_CONF,
  "Controls what groups of directives can be configured by per-directory "
  "config files"),
AP_INIT_RAW_ARGS("Options", set_options, NULL, OR_OPTIONS,
  "Set a number of attributes for a given directory"),
AP_INIT_TAKE1("DefaultType", ap_set_string_slot,
  (void*)APR_OFFSETOF(core_dir_config, ap_default_type),
  OR_FILEINFO, "the default MIME type for untypable files"),
AP_INIT_RAW_ARGS("FileETag", set_etag_bits, NULL, OR_FILEINFO,
  "Specify components used to construct a file's ETag"),
AP_INIT_TAKE1("EnableMMAP", set_enable_mmap, NULL, OR_FILEINFO,
  "Controls whether memory-mapping may be used to read files"),
#endif
/* Old server config file commands */

AP_INIT_TAKE1("Timeout", set_timeout, NULL, RSRC_CONF|EXEC_ON_READ,
  "Timeout duration (sec)"), /* Brian added EXEC_ON_READ */

#if 0
AP_INIT_TAKE1("LimitRequestLine", set_limit_req_line, NULL, RSRC_CONF,
  "Limit on maximum size of an HTTP request line"),
AP_INIT_TAKE1("LimitRequestFieldsize", set_limit_req_fieldsize, NULL,
  RSRC_CONF,
  "Limit on maximum size of an HTTP request header field"),

AP_INIT_TAKE1("ForceType", ap_set_string_slot_lower,
       (void *)APR_OFFSETOF(core_dir_config, mime_type), OR_FILEINFO,
     "a mime type that overrides other configured type"),
AP_INIT_TAKE1("SetHandler", ap_set_string_slot_lower,
       (void *)APR_OFFSETOF(core_dir_config, handler), OR_FILEINFO,
   "a handler name that overrides any other configured handler"),
AP_INIT_TAKE1("SetOutputFilter", ap_set_string_slot,
       (void *)APR_OFFSETOF(core_dir_config, output_filters), OR_FILEINFO,
   "filter (or ; delimited list of filters) to be run on the request content"),
AP_INIT_TAKE1("SetInputFilter", ap_set_string_slot,
       (void *)APR_OFFSETOF(core_dir_config, input_filters), OR_FILEINFO,
   "filter (or ; delimited list of filters) to be run on the request body"),
AP_INIT_ITERATE2("AddOutputFilterByType", add_ct_output_filters,
       (void *)APR_OFFSETOF(core_dir_config, ct_output_filters), OR_FILEINFO,
     "output filter name followed by one or more content-types"),
#endif

{ NULL }
};

/*****************************************************************************/
static int core_post_config(apr_pool_t *pconf, apr_pool_t *ptemp, server_rec *s)
{
/*    ap_set_version(pconf);*/
    ap_setup_make_content_type(pconf);
    return SUCCESS;
}

/*****************************************************************
 *
 * Core handlers for various phases of server operation...
 */

AP_DECLARE_NONSTD(int) ap_core_translate(request_rec *r)
{
    void *sconf = r->server->module_config;
    core_server_config *conf = ap_get_module_config(sconf, CORE_HTTPD_MODULE);

    /* XXX this seems too specific, this should probably become
     * some general-case test
     */
    if (r->proxyreq) {
        return HTTP_FORBIDDEN;
    }
    if (!r->uri || ((r->uri[0] != '/') && strcmp(r->uri, "*"))) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                     "Invalid URI in request %s", r->the_request);
        return HTTP_BAD_REQUEST;
    }

    if (r->server->path
        && !strncmp(r->uri, r->server->path, r->server->pathlen)
        && (r->server->path[r->server->pathlen - 1] == '/'
            || r->uri[r->server->pathlen] == '/'
            || r->uri[r->server->pathlen] == '\0')) {
        if (apr_filepath_merge(&r->filename, conf->ap_document_root,
                               r->uri + r->server->pathlen,
                               APR_FILEPATH_TRUENAME
                             | APR_FILEPATH_SECUREROOT, r->pool)
                    != APR_SUCCESS) {
            return HTTP_FORBIDDEN;
        }
        r->canonical_filename = r->filename;
    }
    else {
        /*
         * Make sure that we do not mess up the translation by adding two
         * /'s in a row.  This happens under windows when the document
         * root ends with a /
         */
        if (apr_filepath_merge(&r->filename, conf->ap_document_root,
                               r->uri + ((*(r->uri) == '/') ? 1 : 0),
                               APR_FILEPATH_TRUENAME
                             | APR_FILEPATH_SECUREROOT, r->pool)
                    != APR_SUCCESS) {
            return HTTP_FORBIDDEN;
        }
        r->canonical_filename = r->filename;
    }

    return SUCCESS;
}

/*****************************************************************
 *
 * Test the filesystem name through directory_walk and file_walk
 */
static int core_map_to_storage(request_rec *r)
{
    int access_status;

    if ((access_status = ap_directory_walk(r))) {
        return access_status;
    }

    if ((access_status = ap_file_walk(r))) {
        return access_status;
    }

    return SUCCESS;
}


static int do_nothing(request_rec *r) { return SUCCESS; }


static int core_override_type(request_rec *r)
{
    core_dir_config *conf =
        (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                CORE_HTTPD_MODULE);

    /* Check for overrides with ForceType / SetHandler
     */
    if (conf->mime_type && strcmp(conf->mime_type, "none"))
        ap_set_content_type(r, (char*) conf->mime_type);

    if (conf->handler && strcmp(conf->handler, "none"))
        r->handler = conf->handler;

    /* Deal with the poor soul who is trying to force path_info to be
     * accepted within the core_handler, where they will let the subreq
     * address its contents.  This is toggled by the user in the very
     * beginning of the fixup phase, so modules should override the user's
     * discretion in their own module fixup phase.  It is tristate, if
     * the user doesn't specify, the result is 2 (which the module may
     * interpret to its own customary behavior.)  It won't be touched
     * if the value is no longer undefined (2), so any module changing
     * the value prior to the fixup phase OVERRIDES the user's choice.
     */
    if ((r->used_path_info == AP_REQ_DEFAULT_PATH_INFO)
        && (conf->accept_path_info != 3)) {
        r->used_path_info = conf->accept_path_info;
    }

    return SUCCESS;
}



static int default_handler(request_rec *r)
{
    conn_rec *c = r->connection;
    apr_bucket_brigade *bb;
    apr_bucket *e;
    core_dir_config *d;
    int errstatus;
    apr_file_t *fd = NULL;
    apr_status_t status;
    /* XXX if/when somebody writes a content-md5 filter we either need to
     *     remove this support or coordinate when to use the filter vs.
     *     when to use this code
     *     The current choice of when to compute the md5 here matches the 1.3
     *     support fairly closely (unlike 1.3, we don't handle computing md5
     *     when the charset is translated).
     */
/*    int bld_content_md5;*/ /* XXX MD5 */

	debug("dh 1");
    d = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                CORE_HTTPD_MODULE);
/* XXX MD5 */
/*    bld_content_md5 = (d->content_md5 & 1)*/
/*                      && r->output_filters->frec->ftype != AP_FTYPE_RESOURCE;*/

	debug("dh 2");
    ap_allow_standard_methods(r, MERGE_ALLOW, M_GET, M_OPTIONS, M_POST, -1);

    /* If filters intend to consume the request body, they must
     * register an InputFilter to slurp the contents of the POST
     * data from the POST input stream.  It no longer exists when
     * the output filters are invoked by the default handler.
     */
	debug("dh 3");
    if ((errstatus = ap_discard_request_body(r)) != SUCCESS) {
        return errstatus;
    }

	debug("dh 4");
    if (r->method_number == M_GET || r->method_number == M_POST) {
		debug("dh 4-1 filetype[%d], path_info[%s], filename[%s]", r->finfo.filetype, r->path_info, r->filename);
        if (r->finfo.filetype == 0) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "File does not exist: %s", r->filename);
            return HTTP_NOT_FOUND;
        }

        /* Don't try to serve a dir.  Some OSs do weird things with
         * raw I/O on a dir.
         */
        if (r->finfo.filetype == APR_DIR) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "Attempt to serve directory: %s", r->filename);
            return HTTP_NOT_FOUND;
        }

        if ((r->used_path_info != AP_REQ_ACCEPT_PATH_INFO) &&
            r->path_info && *r->path_info)
        {
            /* default to reject */
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "File does not exist: %s",
                          apr_pstrcat(r->pool, r->filename, r->path_info, NULL));
            return HTTP_NOT_FOUND;
        }

        if ((status = apr_file_open(&fd, r->filename, APR_READ | APR_BINARY, 0,
                                    r->pool)) != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, status, r,
                          "file permissions deny server access: %s", r->filename);
            return HTTP_FORBIDDEN;
        }
		debug("dh 4-2");

        ap_update_mtime(r, r->finfo.mtime);
        ap_set_last_modified(r);
        ap_set_etag(r);
        apr_table_setn(r->headers_out, "Accept-Ranges", "bytes");
        ap_set_content_length(r, r->finfo.size);
        if ((errstatus = ap_meets_conditions(r)) != SUCCESS) {
            apr_file_close(fd);
            return errstatus;
        }
		debug("dh 4-3");

/* XXX MD5 */
/*
        if (bld_content_md5) {
            apr_table_setn(r->headers_out, "Content-MD5",
                           ap_md5digest(r->pool, fd));
        }
*/

        bb = apr_brigade_create(r->pool, c->bucket_alloc);
        e = apr_bucket_file_create(fd, 0, (apr_size_t)r->finfo.size,
                                       r->pool, c->bucket_alloc);

#if APR_HAS_MMAP
        if (d->enable_mmap == ENABLE_MMAP_OFF) {
            (void)apr_bucket_file_enable_mmap(e, 0);
        }
#endif
        APR_BRIGADE_INSERT_TAIL(bb, e);
        e = apr_bucket_eos_create(c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, e);

        return ap_pass_brigade(r->output_filters, bb);
    }
    else {              /* unusual method (not GET or POST) */
		debug("dh 5");
        if (r->method_number == M_INVALID) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "Invalid method in request %s", r->the_request);
            return HTTP_NOT_IMPLEMENTED;
        }

        if (r->method_number == M_OPTIONS) {
            return ap_send_http_options(r);
        }
        return HTTP_METHOD_NOT_ALLOWED;
    }
}

/******************************************************************************/
static apr_size_t num_request_notes = AP_NUM_STD_NOTES;

static apr_status_t reset_request_notes(void *dummy)
{
    num_request_notes = AP_NUM_STD_NOTES;
    return APR_SUCCESS;
}

AP_DECLARE(apr_size_t) ap_register_request_note(void)
{
    apr_pool_cleanup_register(apr_hook_global_pool, NULL, reset_request_notes,
                              apr_pool_cleanup_null);
    return num_request_notes++;
}

AP_DECLARE(void **) ap_get_request_note(request_rec *r, apr_size_t note_num)
{
    core_request_config *req_cfg;

    if (note_num >= num_request_notes) {
        return NULL;
    }

    req_cfg = (core_request_config *)
        ap_get_module_config(r->request_config, CORE_HTTPD_MODULE);

    if (!req_cfg) {
        return NULL;
    }

    return &(req_cfg->notes[note_num]);
}

static int core_create_req(request_rec *r)
{
    /* Alloc the config struct and the array of request notes in
     * a single block for efficiency
     */
    core_request_config *req_cfg;

    req_cfg = apr_pcalloc(r->pool, sizeof(core_request_config) +
                          sizeof(void *) * num_request_notes);
    req_cfg->notes = (void **)((char *)req_cfg + sizeof(core_request_config));
    if (r->main) {
        core_request_config *main_req_cfg = (core_request_config *)
            ap_get_module_config(r->main->request_config, CORE_HTTPD_MODULE);
        req_cfg->bb = main_req_cfg->bb;
    }
    else {
        req_cfg->bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
        if (!r->prev) {
            ap_add_input_filter_handle(ap_net_time_filter_handle,
                                       NULL, r, r->connection);
        }
    }

    ap_set_module_config(r->request_config, CORE_HTTPD_MODULE, req_cfg);

    /* Begin by presuming any module can make its own path_info assumptions,
     * until some module interjects and changes the value.
     */
    r->used_path_info = AP_REQ_DEFAULT_PATH_INFO;

    return SUCCESS;
}


static void core_insert_filter(request_rec *r)
{
    core_dir_config *conf = (core_dir_config *)
                            ap_get_module_config(r->per_dir_config,
                                                 CORE_HTTPD_MODULE);
    const char *filter, *filters = conf->output_filters;

    if (filters) {
        while (*filters && (filter = ap_getword(r->pool, &filters, ';'))) {
            ap_add_output_filter(filter, NULL, r, r->connection);
        }
    }

    filters = conf->input_filters;
    if (filters) {
        while (*filters && (filter = ap_getword(r->pool, &filters, ';'))) {
            ap_add_input_filter(filter, NULL, r, r->connection);
        }
    }
}


/*****************************************************************************/
static int module_init(void)
{
	register_core_filters(); /* filter.c */
	
	ap_content_length_filter_handle =
	    ap_register_output_filter("CONTENT_LENGTH", ap_content_length_filter,
	                              AP_FTYPE_PROTOCOL);
	ap_subreq_core_filter_handle =
	    ap_register_output_filter("SUBREQ_CORE", ap_sub_req_output_filter,
	                              AP_FTYPE_CONTENT_SET);

	ap_old_write_func = ap_register_output_filter("OLD_WRITE",
	                               ap_old_write_filter, AP_FTYPE_RESOURCE - 10);

	return SUCCESS;
}

static void register_hooks(void)
{
	/* create_connection and install_transport_filters are
	 * hooks that should always be APR_HOOK_REALLY_LAST to give other
	 * modules the opportunity to install alternate network transports
	 * and stop other functions from being run.
	 */
/*	sb_hook_pre_connection(core_pre_connection, NULL, NULL, HOOK_REALLY_LAST);*/

	sb_hook_post_config(core_post_config,NULL,NULL,HOOK_REALLY_FIRST);
	sb_hook_translate_name(ap_core_translate,NULL,NULL,HOOK_REALLY_LAST);
	sb_hook_map_to_storage(core_map_to_storage,NULL,NULL,HOOK_REALLY_LAST);
/*	sb_hook_open_logs(core_open_logs,NULL,NULL,HOOK_MIDDLE);*/

	sb_hook_handler(default_handler,NULL,NULL,HOOK_REALLY_LAST);

	/* FIXME: I suspect we can eliminate the need for these do_nothings - Ben */
	sb_hook_type_checker(do_nothing,NULL,NULL,HOOK_REALLY_LAST);
	sb_hook_fixups(core_override_type,NULL,NULL,HOOK_REALLY_FIRST);
	sb_hook_access_checker(do_nothing,NULL,NULL,HOOK_REALLY_LAST);
	sb_hook_create_request(core_create_req, NULL, NULL, HOOK_MIDDLE);
/*	sb_hook_pre_mpm(ap_create_scoreboard, NULL, NULL, HOOK_MIDDLE);*/

	/* register the core's insert_filter hook and register core-provided
	 * filters
	 */
	sb_hook_insert_filter(core_insert_filter, NULL, NULL, APR_HOOK_MIDDLE);

	/* XXX moved to module_init() for apr_pool stuff */
	//register_core_filters(); /* filter.c */
}

module core_httpd_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	module_init,				/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks				/* register hook api */
};

