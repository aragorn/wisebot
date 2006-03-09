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

#define CORE_PRIVATE
#include "softbot.h"
#include "conf.h"
#include "request.h"
#include "protocol.h"
#include "http_protocol.h"
#include "http_config.h"
#include "util_string.h"

#if APR_HAVE_STDARG_H
/*#include <stdarg.h>*/
#endif



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


