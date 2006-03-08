/* $Id$ */
/*
 * http_core.c
 */

#include "softbot.h"
#include "mod_httpd.h"
#define CORE_PRIVATE
#include "http_core.h"
#include "http_config.h"
#include "connection.h"
#include "request.h"
#include "protocol.h"
#include "http_request.h"
#include "http_protocol.h"
#include "filter.h"
#include "http_filter.h"
#include "util_filter.h"
/*#include "util_time.h"*/

/*#include "apr.h"*/
#include "apr_strings.h"
/*#include "apr_buckets.h"*/
/*#include "apr_hash.h"*/
/*#include "apr_lib.h"*/

/*#include "apr_uri.h" |+ read_request_line() +|*/
/*#include "apr_date.h"	|+ For apr_date_parse_http and APR_DATE_BAD +|*/
/*#include "util_time.h"*/


#if 0
#include "apr_strings.h"
#include "apr_thread_proc.h"    /* for RLIMIT stuff */

#define APR_WANT_STRFUNC
#include "apr_want.h"

#define CORE_PRIVATE
#include "httpd.h"
#include "http_config.h"
#include "http_connection.h"
#include "http_core.h"
#include "http_protocol.h"	/* For index_of_response().  Grump. */
#include "http_request.h"

#include "util_filter.h"
#include "util_ebcdic.h"
#include "ap_mpm.h"
#include "scoreboard.h"

#include "mod_core.h"
#endif

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


#if 0
static const char *set_keep_alive_timeout(cmd_parms *cmd, void *dummy,
					  const char *arg)
{
    const char *err = ap_check_cmd_context(cmd, NOT_IN_DIR_LOC_FILE|NOT_IN_LIMIT);
    if (err != NULL) {
        return err;
    }

    cmd->server->keep_alive_timeout = apr_time_from_sec(atoi(arg));
    return NULL;
}

static const char *set_keep_alive(cmd_parms *cmd, void *dummy,
				  const char *arg) 
{
    const char *err = ap_check_cmd_context(cmd, NOT_IN_DIR_LOC_FILE|NOT_IN_LIMIT);
    if (err != NULL) {
        return err;
    }

    /* We've changed it to On/Off, but used to use numbers
     * so we accept anything but "Off" or "0" as "On"
     */
    if (!strcasecmp(arg, "off") || !strcmp(arg, "0")) {
	cmd->server->keep_alive = 0;
    }
    else {
	cmd->server->keep_alive = 1;
    }
    return NULL;
}

static const char *set_keep_alive_max(cmd_parms *cmd, void *dummy,
				      const char *arg)
{
    const char *err = ap_check_cmd_context(cmd, NOT_IN_DIR_LOC_FILE|NOT_IN_LIMIT);
    if (err != NULL) {
        return err;
    }

    cmd->server->keep_alive_max = atoi(arg);
    return NULL;
}

static const command_rec http_cmds[] = {
    AP_INIT_TAKE1("KeepAliveTimeout", set_keep_alive_timeout, NULL, RSRC_CONF,
                  "Keep-Alive timeout duration (sec)"),
    AP_INIT_TAKE1("MaxKeepAliveRequests", set_keep_alive_max, NULL, RSRC_CONF,
     "Maximum number of Keep-Alive requests per connection, or 0 for infinite"),
    AP_INIT_TAKE1("KeepAlive", set_keep_alive, NULL, RSRC_CONF,
                  "Whether persistent connections should be On or Off"),
    { NULL }
};
#endif

static const char *http_method(const request_rec *r)
    { return "http"; }

/*
 * HTTP/1.1 chunked transfer encoding filter.
 */
static apr_status_t chunk_filter(ap_filter_t *f, apr_bucket_brigade *b)
{
#define ASCII_CRLF  "\015\012"
#define ASCII_ZERO  "\060"
    conn_rec *c = f->r->connection;
    apr_bucket_brigade *more;
    apr_bucket *e;
    apr_status_t rv;

    for (more = NULL; b; b = more, more = NULL) {
        apr_off_t bytes = 0;
        apr_bucket *eos = NULL;
        apr_bucket *flush = NULL;
        /* XXX: chunk_hdr must remain at this scope since it is used in a 
         *      transient bucket.
         */
        char chunk_hdr[20]; /* enough space for the snprintf below */

        APR_BRIGADE_FOREACH(e, b) {
            if (APR_BUCKET_IS_EOS(e)) {
                /* there shouldn't be anything after the eos */
                eos = e;
                break;
            }
            if (APR_BUCKET_IS_FLUSH(e)) {
                flush = e;
            }
            else if (e->length == -1) {
                /* unknown amount of data (e.g. a pipe) */
                const char *data;
                apr_size_t len;

                rv = apr_bucket_read(e, &data, &len, APR_BLOCK_READ);
                if (rv != APR_SUCCESS) {
                    return rv;
                }
                if (len > 0) {
                    /*
                     * There may be a new next bucket representing the
                     * rest of the data stream on which a read() may
                     * block so we pass down what we have so far.
                     */
                    bytes += len;
                    more = apr_brigade_split(b, APR_BUCKET_NEXT(e));
                    break;
                }
                else {
                    /* If there was nothing in this bucket then we can
                     * safely move on to the next one without pausing
                     * to pass down what we have counted up so far.
                     */
                    continue;
                }
            }
            else {
                bytes += e->length;
            }
        }

        /*
         * XXX: if there aren't very many bytes at this point it may
         * be a good idea to set them aside and return for more,
         * unless we haven't finished counting this brigade yet.
         */
        /* if there are content bytes, then wrap them in a chunk */
        if (bytes > 0) {
            apr_size_t hdr_len;
            /*
             * Insert the chunk header, specifying the number of bytes in
             * the chunk.
             */
            /* XXX might be nice to have APR_OFF_T_FMT_HEX */
            hdr_len = apr_snprintf(chunk_hdr, sizeof(chunk_hdr),
                                   "%qx" CRLF, (apr_uint64_t)bytes);
			// FIXME
            //ap_xlate_proto_to_ascii(chunk_hdr, hdr_len);
            e = apr_bucket_transient_create(chunk_hdr, hdr_len,
                                            c->bucket_alloc);
            APR_BRIGADE_INSERT_HEAD(b, e);

            /*
             * Insert the end-of-chunk CRLF before an EOS or
             * FLUSH bucket, or appended to the brigade
             */
            e = apr_bucket_immortal_create(ASCII_CRLF, 2, c->bucket_alloc);
            if (eos != NULL) {
                APR_BUCKET_INSERT_BEFORE(eos, e);
            }
            else if (flush != NULL) {
                APR_BUCKET_INSERT_BEFORE(flush, e);
            }
            else {
                APR_BRIGADE_INSERT_TAIL(b, e);
            }
        }

        /* RFC 2616, Section 3.6.1
         *
         * If there is an EOS bucket, then prefix it with:
         *   1) the last-chunk marker ("0" CRLF)
         *   2) the trailer
         *   3) the end-of-chunked body CRLF
         *
         * If there is no EOS bucket, then do nothing.
         *
         * XXX: it would be nice to combine this with the end-of-chunk
         * marker above, but this is a bit more straight-forward for
         * now.
         */
        if (eos != NULL) {
            /* XXX: (2) trailers ... does not yet exist */
            e = apr_bucket_immortal_create(ASCII_ZERO ASCII_CRLF
                                           /* <trailers> */
                                           ASCII_CRLF, 5, c->bucket_alloc);
            APR_BUCKET_INSERT_BEFORE(eos, e);
        }

        /* pass the brigade to the next filter. */
        rv = ap_pass_brigade(f->next, b);
        if (rv != APR_SUCCESS || eos != NULL) {
            return rv;
        }
    }
    return APR_SUCCESS;
}


static int process_http_connection(conn_rec * c)
{
    request_rec *r;

	debug("started");
    /*
     * read and process each request found on our connection
     * until no request is left or we decide to close.
     */
    update_slot_state(c->slot, SLOT_READ, NULL);
    while ((r = read_request(c)) != NULL) {
		c->keepalive = 0;
		debug("read_request() is done.");

		update_slot_state(c->slot, SLOT_WRITE, r);
		if (r->status == HTTP_OK)
			process_http_request(r);
		else
			debug("r->status != HTTP_OK: %d", r->status);

		// FIXME
		//if (mExtendedStatus);
		//increment_counts(c->slot, r); //FIXME

		if (!c->keepalive || c->aborted)
			break;

		update_slot_state(c->slot, SLOT_KEEPALIVE, r);
		apr_pool_destroy(r->pool);

		if (graceful_stop_signalled())
			break;
    }

	debug("ended");
    return SUCCESS;
}

static int create_http_request(request_rec *r)
{
	debug("started");

    if (!r->main && !r->prev) {
		SB_DEBUG_ASSERT(ap_byterange_filter_handle
						&& ap_content_length_filter_handle
						&& ap_http_header_filter_handle);

        ap_add_output_filter_handle(ap_byterange_filter_handle,
                                    NULL, r, r->connection);
        ap_add_output_filter_handle(ap_content_length_filter_handle,
                                    NULL, r, r->connection);
        ap_add_output_filter_handle(ap_http_header_filter_handle,
                                    NULL, r, r->connection);
    }
	debug("ended");
    return SUCCESS;
}

#if 0
static void register_hooks(apr_pool_t *p)
{
    ap_hook_process_connection(ap_process_http_connection,NULL,NULL,
			       APR_HOOK_REALLY_LAST);
    ap_hook_map_to_storage(ap_send_http_trace,NULL,NULL,APR_HOOK_MIDDLE);
    ap_hook_http_method(http_method,NULL,NULL,APR_HOOK_REALLY_LAST);
    ap_hook_default_port(http_port,NULL,NULL,APR_HOOK_REALLY_LAST);
    ap_hook_create_request(http_create_request, NULL, NULL, APR_HOOK_REALLY_LAST);
    ap_http_input_filter_handle =
        ap_register_input_filter("HTTP_IN", ap_http_filter,
                                 AP_FTYPE_PROTOCOL);
    ap_http_header_filter_handle =
        ap_register_output_filter("HTTP_HEADER", ap_http_header_filter, 
                                  AP_FTYPE_PROTOCOL);
    ap_chunk_filter_handle =
        ap_register_output_filter("CHUNK", chunk_filter, AP_FTYPE_TRANSCODE);
    ap_byterange_filter_handle =
        ap_register_output_filter("BYTERANGE", ap_byterange_filter,
                                  AP_FTYPE_PROTOCOL);
    ap_method_registry_init(p);
}

/****************************************************************************/
/*
 * There are some elements of the core config structures in which
 * other modules have a legitimate interest (this is ugly, but necessary
 * to preserve NCSA back-compatibility).  So, we have a bunch of accessors
 * here...
 */

int ap_allow_options(request_rec *r)
{
    core_dir_config *conf =
      (core_dir_config *)ap_get_module_config(r->per_dir_config, &httpd_core_module);

    return conf->opts;
}

int ap_allow_overrides(request_rec *r)
{
    core_dir_config *conf;
    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   &httpd_core_module);

    return conf->override;
}

const char * ap_auth_type(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   &httpd_core_module);

    return conf->ap_auth_type;
}

const char * ap_auth_name(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   &httpd_core_module);

    return conf->ap_auth_name;
}

const char * ap_default_type(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   &httpd_core_module);

    return conf->ap_default_type
               ? conf->ap_default_type
               : DEFAULT_CONTENT_TYPE;
}

const char * ap_document_root(request_rec *r) /* Don't use this! */
{
    core_server_config *conf;

    conf = (core_server_config *)ap_get_module_config(r->server->module_config,
                                                      &httpd_core_module);

    return conf->ap_document_root;
}

const apr_array_header_t * ap_requires(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   &httpd_core_module);

    return conf->ap_requires;
}

int ap_satisfies(request_rec *r)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   &httpd_core_module);

    return conf->satisfy;
}

/* Should probably just get rid of this... the only code that cares is
 * part of the core anyway (and in fact, it isn't publicised to other
 * modules).
 */

char *ap_response_code_string(request_rec *r, int error_index)
{
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   &httpd_core_module);

    if (conf->response_code_strings == NULL) {
        return NULL;
    }

    return conf->response_code_strings[error_index];
}


apr_off_t ap_get_limit_req_body(const request_rec *r)
{
    core_dir_config *d =
      (core_dir_config *)ap_get_module_config(r->per_dir_config, &httpd_core_module);

    return d->limit_req_body;
}


/****************************************************************************/
static apr_size_t num_request_notes = AP_NUM_STD_NOTES;

void ** ap_get_request_note(request_rec *r, apr_size_t note_num)
{
	core_request_config *req_cfg;

	if (note_num >= num_request_notes) {
		return NULL;
	}

	req_cfg = (core_request_config *)
		ap_get_module_config(r->request_config, &httpd_core_module);

	if (!req_cfg) {
		return NULL;
	}

	return &(req_cfg->notes[note_num]);
}
#endif

/****************************************************************************/
/* from core.c */
/* FIXME
const char * ap_psignature(const char *prefix, request_rec *r)
{
    char sport[20];
    core_dir_config *conf;

    conf = (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                   &core_module);
    if ((conf->server_signature == srv_sig_off)
            || (conf->server_signature == srv_sig_unset)) {
        return "";
    }

    apr_snprintf(sport, sizeof sport, "%u", (unsigned) ap_get_server_port(r));

    if (conf->server_signature == srv_sig_withmail) {
        return apr_pstrcat(r->pool, prefix, "<address>" AP_SERVER_BASEVERSION
                           " Server at <a href=\"mailto:",
                           r->server->server_admin, "\">",
                           ap_get_server_name(r), "</a> Port ", sport,
                           "</address>\n", NULL);
    }

    return apr_pstrcat(r->pool, prefix, "<address>" AP_SERVER_BASEVERSION
                       " Server at ", ap_get_server_name(r), " Port ", sport,
                       "</address>\n", NULL);
}
*/

/****************************************************************************/
static config_t config[] = {
	// KeepAliveTimeout
	// MaxKeepAliveRequests
	// KeepAlive
	{NULL}
};

static int module_init(void)
{
	register_core_filters(); /* filter.c */

	ap_http_input_filter_handle =
        	ap_register_input_filter("HTTP_IN", ap_http_filter,
               		                  	AP_FTYPE_PROTOCOL);
	ap_http_header_filter_handle =
		ap_register_output_filter("HTTP_HEADER", ap_http_header_filter, 
						AP_FTYPE_PROTOCOL);
	ap_chunk_filter_handle =
		ap_register_output_filter("CHUNK", chunk_filter, AP_FTYPE_TRANSCODE);

	ap_byterange_filter_handle =
		ap_register_output_filter("BYTERANGE", ap_byterange_filter,
						AP_FTYPE_PROTOCOL);
	ap_content_length_filter_handle =
		ap_register_output_filter("CONTENT_LENGTH", ap_content_length_filter,
						AP_FTYPE_NETWORK);

	return SUCCESS;
}

static void register_hooks(void)
{
    sb_hook_process_connection(process_http_connection,NULL,NULL, HOOK_REALLY_LAST);
    sb_hook_create_request(create_http_request, NULL, NULL, HOOK_REALLY_LAST);
    sb_hook_http_method(http_method,NULL,NULL,HOOK_REALLY_LAST);

	return;
}

module http_core_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	module_init,			/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};

