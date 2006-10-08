/* $Id$ */
/*
 * http_request.c: functions to get and process requests
 *
 * Rob McCool 3/21/93
 *
 * Thoroughly revamped by rst for Apache.  NB this file reads
 * best from the bottom up.
 *
 */

#define APR_WANT_STRFUNC
/*#include "apr_want.h"*/
#include "common_core.h"
#include "mod_httpd.h"
#include "request.h"
#include "protocol.h"
#include "http_protocol.h"
#include "http_config.h"
#include "http_util.h" /* ap_unescape_url, ap_getparents */
#include "util_string.h"
#include "core.h" /* ap_satisfies */
#include "log.h" /* ap_log_rerror */

#if APR_HAVE_STDARG_H
/*#include <stdarg.h>*/
#endif

static int decl_die(int status, char *phase, request_rec *r)
{   
	if (status == DECLINE) {
		ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r,
					  "configuration error:  couldn't %s: %s", phase, r->uri);
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	else {
		return status;
	}
}


/* This is the master logic for processing requests.  Do NOT duplicate
 * this logic elsewhere, or the security model will be broken by future
 * API changes.  Each phase must be individually optimized to pick up
 * redundant/duplicate calls by subrequests, and redirects.
 */
AP_DECLARE(int) ap_process_request_internal(request_rec *r)
{
    int file_req = (r->main && r->filename);
    int access_status = DECLINE;

	debug("started");
    /* Ignore embedded %2F's in path for proxy requests */
    if (!r->proxyreq && r->parsed_uri.path) {
        access_status = ap_unescape_url(r->parsed_uri.path);
        if (access_status) {
            return access_status;
        }
    }

    ap_getparents(r->uri);     /* OK --- shrinking transformations... */

debug("here 1");
    /* All file subrequests are a huge pain... they cannot bubble through the
     * next several steps.  Only file subrequests are allowed an empty uri,
     * otherwise let translate_name kill the request.
     */
    if (!file_req) {
        if ((access_status = ap_location_walk(r))) {
            return access_status;
        }

        if ((access_status = sb_run_translate_name(r))) {
            return decl_die(access_status, "translate", r);
        }
    }
debug("here 2");

    /* Reset to the server default config prior to running map_to_storage
     */
    r->per_dir_config = r->server->lookup_defaults;

    if ((access_status = sb_run_map_to_storage(r))) {
        /* This request wasn't in storage (e.g. TRACE) */
        return access_status;
    }

    /* Excluding file-specific requests with no 'true' URI...
     */
    if (!file_req) {
        /* Rerun the location walk, which overrides any map_to_storage config.
         */
        if ((access_status = ap_location_walk(r))) {
            return access_status;
        }
    }

debug("here 3");
    /* Only on the main request! */
    if (r->main == NULL) {
        if ((access_status = sb_run_header_parser(r))) {
            return access_status;
        }
    }
debug("here 4");

    /* Skip authn/authz if the parent or prior request passed the authn/authz,
     * and that configuration didn't change (this requires optimized _walk()
     * functions in map_to_storage that use the same merge results given
     * identical input.)  If the config changes, we must re-auth.
     */
    if (r->main && (r->main->per_dir_config == r->per_dir_config)) {
        r->user = r->main->user;
        r->auth_type = r->main->auth_type;
    }
    else if (r->prev && (r->prev->per_dir_config == r->per_dir_config)) {
        r->user = r->prev->user;
        r->auth_type = r->prev->auth_type;
    }
    else {
        switch (ap_satisfies(r)) {
        case SATISFY_ALL:
        case SATISFY_NOSPEC:
            if ((access_status = sb_run_access_checker(r)) != 0) {
                return decl_die(access_status, "check access", r);
            }

            if (ap_some_auth_required(r)) {
                if (((access_status = sb_run_check_user_id(r)) != 0)
                    || !ap_auth_type(r)) {
                    return decl_die(access_status, ap_auth_type(r)
                                  ? "check user.  No user file?"
                                  : "perform authentication. AuthType not set!",
                                  r);
                }

                if (((access_status = sb_run_auth_checker(r)) != 0)
                    || !ap_auth_type(r)) {
                    return decl_die(access_status, ap_auth_type(r)
                                  ? "check access.  No groups file?"
                                  : "perform authentication. AuthType not set!",
                                   r);
                }
            }
            break;

        case SATISFY_ANY:
            if (((access_status = sb_run_access_checker(r)) != 0)
                || !ap_auth_type(r)) {
                if (!ap_some_auth_required(r)) {
                    return decl_die(access_status, ap_auth_type(r)
                                  ? "check access"
                                  : "perform authentication. AuthType not set!",
                                  r);
                }

                if (((access_status = sb_run_check_user_id(r)) != 0)
                    || !ap_auth_type(r)) {
                    return decl_die(access_status, ap_auth_type(r)
                                  ? "check user.  No user file?"
                                  : "perform authentication. AuthType not set!",
                                  r);
                }

                if (((access_status = sb_run_auth_checker(r)) != 0)
                    || !ap_auth_type(r)) {
                    return decl_die(access_status, ap_auth_type(r)
                                  ? "check access.  No groups file?"
                                  : "perform authentication. AuthType not set!",
                                  r);
                }
            }
            break;
        }
    }
debug("here 5");
    /* XXX Must make certain the ap_run_type_checker short circuits mime
     * in mod-proxy for r->proxyreq && r->parsed_uri.scheme
     *                              && !strcmp(r->parsed_uri.scheme, "http")
     */
    if ((access_status = sb_run_type_checker(r)) != 0) {
        return decl_die(access_status, "find types", r);
    }

    if ((access_status = sb_run_fixups(r)) != 0) {
        return access_status;
    }

	debug("ended");
    return SUCCESS;
}


AP_DECLARE(void) ap_allow_standard_methods(request_rec *r, int reset, ...)
{
    int method;
    va_list methods;
    apr_int64_t mask;

    /*
     * Get rid of any current settings if requested; not just the
     * well-known methods but any extensions as well.
     */
    if (reset) {
        ap_clear_method_list(r->allowed_methods);
    }

    mask = 0;
    va_start(methods, reset);
    while ((method = va_arg(methods, int)) != -1) {
        mask |= (AP_METHOD_BIT << method);
    }
    va_end(methods);

    r->allowed_methods->method_mask |= mask;
}

/*****************************************************************
 *
 * Mainline request processing...
 */

void ap_die(int type, request_rec *r)
{
    int error_index = ap_index_of_response(type);
    char *custom_response = ap_response_code_string(r, error_index);
    int recursive_error = 0;

    if (type == AP_FILTER_ERROR) {
        return;
    }

    if (type == SUCCESS) {
        ap_finalize_request_protocol(r);
        return;
    }

    /*
     * The following takes care of Apache redirects to custom response URLs
     * Note that if we are already dealing with the response to some other
     * error condition, we just report on the original error, and give up on
     * any attempt to handle the other thing "intelligently"...
     */
    if (r->status != HTTP_OK && !status_drops_connection(type)) {
        recursive_error = type;

        while (r->prev && (r->prev->status != HTTP_OK))
            r = r->prev;        /* Get back to original error */

        type = r->status;
        custom_response = NULL; /* Do NOT retry the custom thing! */
    }

    r->status = type;

    /*
     * This test is done here so that none of the auth modules needs to know
     * about proxy authentication.  They treat it like normal auth, and then
     * we tweak the status.
     */
    if (HTTP_UNAUTHORIZED == r->status && PROXYREQ_PROXY == r->proxyreq) {
        r->status = HTTP_PROXY_AUTHENTICATION_REQUIRED;
    }

    /* If we don't want to keep the connection, make sure we mark that the
     * connection is not eligible for keepalive.  If we want to keep the
     * connection, be sure that the request body (if any) has been read.
     */
    if (status_drops_connection(r->status)) {
        r->connection->keepalive = 0;
    }

    /*
     * Two types of custom redirects --- plain text, and URLs. Plain text has
     * a leading '"', so the URL code, here, is triggered on its absence
     */

    if (custom_response && custom_response[0] != '"') {

        if (sb_is_url(custom_response)) {
            /*
             * The URL isn't local, so lets drop through the rest of this
             * apache code, and continue with the usual REDIRECT handler.
             * But note that the client will ultimately see the wrong
             * status...
             */
            r->status = HTTP_MOVED_TEMPORARILY;
            apr_table_setn(r->headers_out, "Location", custom_response);
        }
		// FIXME internal_redirect is not yet implemented
#if 0
        else if (custom_response[0] == '/') {
            const char *error_notes;
            r->no_local_copy = 1;       /* Do NOT send HTTP_NOT_MODIFIED for
                                         * error documents! */
            /*
             * This redirect needs to be a GET no matter what the original
             * method was.
             */
            apr_table_setn(r->subprocess_env, "REQUEST_METHOD", r->method);

            /*
             * Provide a special method for modules to communicate
             * more informative (than the plain canned) messages to us.
             * Propagate them to ErrorDocuments via the ERROR_NOTES variable:
             */
            if ((error_notes = apr_table_get(r->notes, 
                                             "error-notes")) != NULL) {
                apr_table_setn(r->subprocess_env, "ERROR_NOTES", error_notes);
            }
            r->method = apr_pstrdup(r->pool, "GET");
            r->method_number = M_GET;
            ap_internal_redirect(custom_response, r);
            return;
        }
#endif
        else {
            /*
             * Dumb user has given us a bad url to redirect to --- fake up
             * dying with a recursive server error...
             */
            recursive_error = HTTP_INTERNAL_SERVER_ERROR;
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                        "Invalid error redirection directive: %s",
                        custom_response);
        }
    }
    ap_send_error_response(r, recursive_error);
}

static void check_pipeline_flush(request_rec *r)
{
    conn_rec *c = r->connection;
    /* ### if would be nice if we could PEEK without a brigade. that would
       ### allow us to defer creation of the brigade to when we actually
       ### need to send a FLUSH. */
    apr_bucket_brigade *bb = apr_brigade_create(r->pool, c->bucket_alloc);

    /* Flush the filter contents if:
     *
     *   1) the connection will be closed
     *   2) there isn't a request ready to be read
     */
    /* ### shouldn't this read from the connection input filters? */
    /* ### is zero correct? that means "read one line" */
    if (!r->connection->keepalive || 
        ap_get_brigade(r->input_filters, bb, AP_MODE_EATCRLF, 
                       APR_NONBLOCK_READ, 0) != APR_SUCCESS) {
        apr_bucket *e = apr_bucket_flush_create(c->bucket_alloc);

        /* We just send directly to the connection based filters.  At
         * this point, we know that we have seen all of the data
         * (request finalization sent an EOS bucket, which empties all
         * of the request filters). We just want to flush the buckets
         * if something hasn't been sent to the network yet.
         */
        APR_BRIGADE_INSERT_HEAD(bb, e);
        ap_pass_brigade(r->connection->output_filters, bb);
    }
}

void process_http_request(request_rec *r)
{
    int access_status = DECLINE;

    /* Give quick handlers a shot at serving the request on the fast
     * path, bypassing all of the other Apache hooks.
     *
     * This hook was added to enable serving files out of a URI keyed 
     * content cache ( e.g., Mike Abbott's Quick Shortcut Cache, 
     * described here: http://oss.sgi.com/projects/apache/mod_qsc.html )
     *
     * It may have other uses as well, such as routing requests directly to
     * content handlers that have the ability to grok HTTP and do their
     * own access checking, etc (e.g. servlet engines). 
     * 
     * Use this hook with extreme care and only if you know what you are 
     * doing.
     */
//    access_status = sb_run_quick_handler(r, 0);  /* Not a look-up request */

    if (access_status == DECLINE) {
        access_status = ap_process_request_internal(r);
        if (access_status == SUCCESS) {
            access_status = ap_invoke_handler(r);
        }
    }

    //debug("access_status = %d", access_status);

    if (access_status == DONE) {
        /* e.g., something not in storage like TRACE */
        //debug("DONE -> SUCCESS");
        access_status = SUCCESS;
    }

    if (access_status == SUCCESS) {
        //debug("SUCCESS .: ap_finalize_request_protocol");
        ap_finalize_request_protocol(r);
        debug("ap_finalize_request_protocol returns");
    }
    else {
        //debug("!SUCCESS .: ap_die");
        ap_die(access_status, r);
    }
    
    /*
     * We want to flush the last packet if this isn't a pipelining connection
     * *before* we start into logging.  Suppose that the logging causes a DNS
     * lookup to occur, which may have a high latency.  If we hold off on
     * this packet, then it'll appear like the link is stalled when really
     * it's the application that's stalled.
     */
//    debug("check_pipeline_flush");
    check_pipeline_flush(r);
//    debug("run_log_transaction");
//    sb_run_log_transaction(r);
//    debug("ended");
}


