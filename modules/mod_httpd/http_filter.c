/* $Id$ */
/*
 * http_filter.c
 */


#include "mod_httpd.h"
#include "http_filter.h"
#include "http_core.h"
#include "protocol.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_util.h"
#include "util_time.h"
#include "apr_lib.h"
#include "apr_strings.h"

/* Handles for http core filters */
ap_filter_rec_t *ap_http_input_filter_handle;
ap_filter_rec_t *ap_http_header_filter_handle;
ap_filter_rec_t *ap_chunk_filter_handle;
ap_filter_rec_t *ap_byterange_filter_handle;

/*****************************************************************************/
/* New Apache routine to map status codes into array indicies
 *  e.g.  100 -> 0,  101 -> 1,  200 -> 2 ...
 * The number of status lines must equal the value of RESPONSE_CODES (httpd.h)
 * and must be listed in order.
 */

/* FIXME gotta eliminate duplication from http_protocol.c */

static const char *const status_lines[RESPONSE_CODES] = {
    "100 Continue",
    "101 Switching Protocols",
    "102 Processing",
#define LEVEL_200  3
    "200 OK",
    "201 Created",
    "202 Accepted",
    "203 Non-Authoritative Information",
    "204 No Content",
    "205 Reset Content",
    "206 Partial Content",
    "207 Multi-Status",
#define LEVEL_300 11
    "300 Multiple Choices",
    "301 Moved Permanently",
    "302 Found",
    "303 See Other",
    "304 Not Modified",
    "305 Use Proxy",
    "306 unused",
    "307 Temporary Redirect",
#define LEVEL_400 19
    "400 Bad Request",
    "401 Authorization Required",
    "402 Payment Required",
    "403 Forbidden",
    "404 Not Found",
    "405 Method Not Allowed",
    "406 Not Acceptable",
    "407 Proxy Authentication Required",
    "408 Request Time-out",
    "409 Conflict",
    "410 Gone",
    "411 Length Required",
    "412 Precondition Failed",
    "413 Request Entity Too Large",
    "414 Request-URI Too Large",
    "415 Unsupported Media Type",
    "416 Requested Range Not Satisfiable",
    "417 Expectation Failed",
    "418 unused",
    "419 unused",
    "420 unused",
    "421 unused",
    "422 Unprocessable Entity",
    "423 Locked",
    "424 Failed Dependency",
#define LEVEL_500 44
    "500 Internal Server Error",
    "501 Method Not Implemented",
    "502 Bad Gateway",
    "503 Service Temporarily Unavailable",
    "504 Gateway Time-out",
    "505 HTTP Version Not Supported",
    "506 Variant Also Negotiates",
    "507 Insufficient Storage",
    "508 unused",
    "509 unused",
    "510 Not Extended"
};
/*****************************************************************************/
/* FILTER : http */

static long get_chunk_size(char *);

typedef struct http_filter_ctx {
    apr_off_t remaining;
    apr_off_t limit;
    apr_off_t limit_used;
    enum {
        BODY_NONE,
        BODY_LENGTH,
        BODY_CHUNK
    } state;
    int eos_sent;
} http_ctx_t;

/* This is the HTTP_INPUT filter for HTTP requests and responses from 
 * proxied servers (mod_proxy).  It handles chunked and content-length 
 * bodies.  This can only be inserted/used after the headers
 * are successfully parsed. 
 */
apr_status_t ap_http_filter(ap_filter_t *f, apr_bucket_brigade *b,
                            ap_input_mode_t mode, apr_read_type_e block,
                            apr_off_t readbytes)
{
    apr_bucket *e;
    http_ctx_t *ctx = f->ctx;
    apr_status_t rv;
    apr_off_t totalread;

    /* just get out of the way of things we don't want. */
    if (mode != AP_MODE_READBYTES && mode != AP_MODE_GETLINE) {
        return ap_get_brigade(f->next, b, mode, block, readbytes);
    }

    if (!ctx) {
        const char *tenc, *lenp;
        f->ctx = ctx = apr_palloc(f->r->pool, sizeof(*ctx));
        ctx->state = BODY_NONE;
        ctx->remaining = 0;
        ctx->limit_used = 0;
        ctx->eos_sent = 0;

        /* LimitRequestBody does not apply to proxied responses.
         * Consider implementing this check in its own filter. 
         * Would adding a directive to limit the size of proxied 
         * responses be useful?
         */
        if (!f->r->proxyreq) {
            ctx->limit = ap_get_limit_req_body(f->r);
        }
        else {
            ctx->limit = 0;
        }

        tenc = apr_table_get(f->r->headers_in, "Transfer-Encoding");
        lenp = apr_table_get(f->r->headers_in, "Content-Length");

        if (tenc) {
            if (!strcasecmp(tenc, "chunked")) {
                ctx->state = BODY_CHUNK;
            }
        }
        else if (lenp) {
            const char *pos = lenp;
            int conversion_error = 0;

            /* This ensures that the number can not be negative. */
            while (apr_isdigit(*pos) || apr_isspace(*pos)) {
                ++pos;
            }

            if (*pos == '\0') {
                char *endstr;

                errno = 0;
                ctx->state = BODY_LENGTH;
                ctx->remaining = strtol(lenp, &endstr, 10);

                if (errno || (endstr && *endstr)) {
                    conversion_error = 1; 
                }
            }

            if (*pos != '\0' || conversion_error) {
                apr_bucket_brigade *bb;

                ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, f->r,
                              "Invalid Content-Length");

                bb = apr_brigade_create(f->r->pool, f->c->bucket_alloc);
                e = ap_bucket_error_create(HTTP_REQUEST_ENTITY_TOO_LARGE, NULL,
                                           f->r->pool, f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(bb, e);
                e = apr_bucket_eos_create(f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(bb, e);
                ctx->eos_sent = 1;
                return ap_pass_brigade(f->r->output_filters, bb);
            }
            
            /* If we have a limit in effect and we know the C-L ahead of
             * time, stop it here if it is invalid. 
             */ 
            if (ctx->limit && ctx->limit < ctx->remaining) {
                apr_bucket_brigade *bb;
                ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, f->r,
                          "Requested content-length of %" APR_OFF_T_FMT 
                          " is larger than the configured limit"
                          " of %" APR_OFF_T_FMT, ctx->remaining, ctx->limit);
                bb = apr_brigade_create(f->r->pool, f->c->bucket_alloc);
                e = ap_bucket_error_create(HTTP_REQUEST_ENTITY_TOO_LARGE, NULL,
                                           f->r->pool, f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(bb, e);
                e = apr_bucket_eos_create(f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(bb, e);
                ctx->eos_sent = 1;
                return ap_pass_brigade(f->r->output_filters, bb);
            }
        }

        /* If we don't have a request entity indicated by the headers, EOS.
         * (BODY_NONE is a valid intermediate state due to trailers,
         *  but it isn't a valid starting state.)
         *
         * RFC 2616 Section 4.4 note 5 states that connection-close
         * is invalid for a request entity - request bodies must be
         * denoted by C-L or T-E: chunked.
         *
         * Note that since the proxy uses this filter to handle the
         * proxied *response*, proxy responses MUST be exempt.
         */
        if (ctx->state == BODY_NONE && f->r->proxyreq != PROXYREQ_RESPONSE) {
            e = apr_bucket_eos_create(f->c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(b, e);
            ctx->eos_sent = 1;
            return APR_SUCCESS;
        }

        /* Since we're about to read data, send 100-Continue if needed.
         * Only valid on chunked and C-L bodies where the C-L is > 0. */
        if ((ctx->state == BODY_CHUNK || 
            (ctx->state == BODY_LENGTH && ctx->remaining > 0)) &&
            f->r->expecting_100 && f->r->proto_num >= HTTP_VERSION(1,1)) {
            char *tmp;
            apr_bucket_brigade *bb;

            tmp = apr_pstrcat(f->r->pool, SERVER_PROTOCOL, " ",
                              status_lines[0], CRLF CRLF, NULL);
            bb = apr_brigade_create(f->r->pool, f->c->bucket_alloc);
            e = apr_bucket_pool_create(tmp, strlen(tmp), f->r->pool,
                                       f->c->bucket_alloc);
            APR_BRIGADE_INSERT_HEAD(bb, e);
            e = apr_bucket_flush_create(f->c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(bb, e);

            ap_pass_brigade(f->c->output_filters, bb);
        }

        /* We can't read the chunk until after sending 100 if required. */
        if (ctx->state == BODY_CHUNK) {
            char line[30];
            apr_bucket_brigade *bb;
            apr_size_t len = 30;

            bb = apr_brigade_create(f->r->pool, f->c->bucket_alloc);

            rv = ap_get_brigade(f->next, bb, AP_MODE_GETLINE,
                                APR_BLOCK_READ, 0);

            if (rv != APR_SUCCESS) {
                return rv;
            }
            apr_brigade_flatten(bb, line, &len);

            ctx->remaining = get_chunk_size(line);
            /* Detect chunksize error (such as overflow) */
            if (ctx->remaining < 0) {
                ctx->remaining = 0; /* Reset it in case we have to
                                     * come back here later */
                apr_brigade_cleanup(bb);
                e = ap_bucket_error_create(HTTP_REQUEST_ENTITY_TOO_LARGE, NULL,
                                           f->r->pool,
                                           f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(bb, e);
                e = apr_bucket_eos_create(f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(bb, e);
                ctx->eos_sent = 1;
                return ap_pass_brigade(f->r->output_filters, bb);
            }

            if (!ctx->remaining) {
                /* Handle trailers by calling get_mime_headers again! */
                ctx->state = BODY_NONE;
                get_mime_headers(f->r);
                e = apr_bucket_eos_create(f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(b, e);
                ctx->eos_sent = 1;
                return APR_SUCCESS;
            }
        } 
    }

    if (ctx->eos_sent) {
        e = apr_bucket_eos_create(f->c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(b, e);
        return APR_SUCCESS;
    }
        
    if (!ctx->remaining) {
        switch (ctx->state) {
        case BODY_NONE:
            break;
        case BODY_LENGTH:
            e = apr_bucket_eos_create(f->c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(b, e);
            ctx->eos_sent = 1;
            return APR_SUCCESS;
        case BODY_CHUNK:
            {
                char line[30];
                apr_bucket_brigade *bb;
                apr_size_t len = 30;

                bb = apr_brigade_create(f->r->pool, f->c->bucket_alloc);

                /* We need to read the CRLF after the chunk.  */
                rv = ap_get_brigade(f->next, bb, AP_MODE_GETLINE,
                                    APR_BLOCK_READ, 0);
                if (rv != APR_SUCCESS) {
                    return rv;
                }
                apr_brigade_cleanup(bb);

                /* Read the real chunk line. */
                rv = ap_get_brigade(f->next, bb, AP_MODE_GETLINE,
                                    APR_BLOCK_READ, 0);

                if (rv != APR_SUCCESS) {
                    return rv;
                }
                apr_brigade_flatten(bb, line, &len);
                ctx->remaining = get_chunk_size(line);

                /* Detect chunksize error (such as overflow) */
                if (ctx->remaining < 0) {
                    ctx->remaining = 0; /* Reset it in case we have to
                                         * come back here later */
                    apr_brigade_cleanup(bb);
                    e = ap_bucket_error_create(HTTP_REQUEST_ENTITY_TOO_LARGE,
                                               NULL, f->r->pool,
                                               f->c->bucket_alloc);
                    APR_BRIGADE_INSERT_TAIL(bb, e);
                    e = apr_bucket_eos_create(f->c->bucket_alloc);
                    APR_BRIGADE_INSERT_TAIL(bb, e);
                    ctx->eos_sent = 1;
                    return ap_pass_brigade(f->r->output_filters, bb);
                }

                if (!ctx->remaining) {
                    /* Handle trailers by calling get_mime_headers again! */
                    ctx->state = BODY_NONE;
                    get_mime_headers(f->r);
                    e = apr_bucket_eos_create(f->c->bucket_alloc);
                    APR_BRIGADE_INSERT_TAIL(b, e);
                    ctx->eos_sent = 1;
                    return APR_SUCCESS;
                }
            }
            break;
        }
    }

    /* Ensure that the caller can not go over our boundary point. */
    if (ctx->state == BODY_LENGTH || ctx->state == BODY_CHUNK) {
        if (ctx->remaining < readbytes) {
            readbytes = ctx->remaining;
        }
        SB_DEBUG_ASSERT(readbytes > 0);
    }

    rv = ap_get_brigade(f->next, b, mode, block, readbytes);

    if (rv != APR_SUCCESS) {
        return rv;
    }

    /* How many bytes did we just read? */
    apr_brigade_length(b, 0, &totalread);

    /* If this happens, we have a bucket of unknown length.  Die because
     * it means our assumptions have changed. */
    SB_DEBUG_ASSERT(totalread >= 0);

    if (ctx->state != BODY_NONE) {
        ctx->remaining -= totalread;
    }

    /* We have a limit in effect. */
    if (ctx->limit) {
        /* FIXME: Note that we might get slightly confused on chunked inputs
         * as we'd need to compensate for the chunk lengths which may not
         * really count.  This seems to be up for interpretation.  */
        ctx->limit_used += totalread;
        if (ctx->limit < ctx->limit_used) {
            apr_bucket_brigade *bb;
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, f->r,
                          "Read content-length of %" APR_OFF_T_FMT 
                          " is larger than the configured limit"
                          " of %" APR_OFF_T_FMT, ctx->limit_used, ctx->limit);
            bb = apr_brigade_create(f->r->pool, f->c->bucket_alloc);
            e = ap_bucket_error_create(HTTP_REQUEST_ENTITY_TOO_LARGE, NULL,
                                       f->r->pool,
                                       f->c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(bb, e);
            e = apr_bucket_eos_create(f->c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(bb, e);
            ctx->eos_sent = 1;
            return ap_pass_brigade(f->r->output_filters, bb);
        }
    }

    return APR_SUCCESS;
}

static long get_chunk_size(char *b)
{
    long chunksize = 0;
    size_t chunkbits = sizeof(long) * 8;

    /* Skip leading zeros */
    while (*b == '0') {
        ++b;
    }

    while (apr_isxdigit(*b) && (chunkbits > 0)) {
        int xvalue = 0;

        if (*b >= '0' && *b <= '9') {
            xvalue = *b - '0';
        }
        else if (*b >= 'A' && *b <= 'F') {
            xvalue = *b - 'A' + 0xa;
        }
        else if (*b >= 'a' && *b <= 'f') {
            xvalue = *b - 'a' + 0xa;
        }

        chunksize = (chunksize << 4) | xvalue;
        chunkbits -= 4;
        ++b;
    }
    if (apr_isxdigit(*b) && (chunkbits <= 0)) {
        /* overflow */
        return -1;
    }

    return chunksize;
}

/*****************************************************************************/
/* FILTER : http header */

/*
 * Determine the protocol to use for the response. Potentially downgrade
 * to HTTP/1.0 in some situations and/or turn off keepalives.
 *
 * also prepare r->status_line.
 */
static void basic_http_header_check(request_rec * r, const char **protocol)
{
    if (r->assbackwards) {
	/* no such thing as a response protocol */
	return;
    }

    if (!r->status_line) {
	r->status_line = status_lines[ap_index_of_response(r->status)];
    }

    /* Note that we must downgrade before checking for force responses. */
    if (r->proto_num > HTTP_VERSION(1, 0)
	&& apr_table_get(r->subprocess_env, "downgrade-1.0")) {
	r->proto_num = HTTP_VERSION(1, 0);
    }

    /* kludge around broken browsers when indicated by force-response-1.0
     */
    if (r->proto_num == HTTP_VERSION(1, 0)
	&& apr_table_get(r->subprocess_env, "force-response-1.0")) {
	*protocol = "HTTP/1.0";
	r->connection->keepalive = -1;
    }
    else {
	*protocol = SERVER_PROTOCOL;
    }

}

/* fill "bb" with a barebones/initial HTTP response header */
static void basic_http_header(request_rec * r, apr_bucket_brigade * bb,
			      const char *protocol)
{
    char *date;
    char *tmp;
    const char *server;
    header_struct h;
    apr_size_t len;
    struct iovec vec[4];

    if (r->assbackwards) {
	/* there are no headers to send */
	return;
    }

    /* Output the HTTP/1.x Status-Line and the Date and Server fields */

    vec[0].iov_base = (void *) protocol;
    vec[0].iov_len = strlen(protocol);
    vec[1].iov_base = (void *) " ";
    vec[1].iov_len = sizeof(" ") - 1;
    vec[2].iov_base = (void *) (r->status_line);
    vec[2].iov_len = strlen(r->status_line);
    vec[3].iov_base = (void *) CRLF;
    vec[3].iov_len = sizeof(CRLF) - 1;
    tmp = apr_pstrcatv(r->pool, vec, 4, &len);
    // FIXME 
    //ap_xlate_proto_to_ascii(tmp, len);
    //apr_brigade_write(bb, NULL, NULL, tmp, len);
    apr_brigade_write(bb, NULL, NULL, tmp, len);

    date = apr_palloc(r->pool, APR_RFC822_DATE_LEN);
    ap_recent_rfc822_date(date, r->request_time);

    h.pool = r->pool;
    h.bb = bb;
    form_header_field(&h, "Date", date);

    /* keep a previously set server header (possibly from proxy), otherwise
     * generate a new server header */
    if ((server = apr_table_get(r->headers_out, "Server")) != NULL) {
	form_header_field(&h, "Server", server);
    }
    else {
	form_header_field(&h, "Server", sb_get_server_version());
    }

    /* unset so we don't send them again */
    apr_table_unset(r->headers_out, "Date");	/* Avoid bogosity */
    apr_table_unset(r->headers_out, "Server");
}

/*
AP_DECLARE(void) ap_basic_http_header(request_rec * r,
				      apr_bucket_brigade * bb)
{
    const char *protocol;

    basic_http_header_check(r, &protocol);
    basic_http_header(r, bb, protocol);
}
*/

/* Navigator versions 2.x, 3.x and 4.0 betas up to and including 4.0b2
 * have a header parsing bug.  If the terminating \r\n occur starting
 * at offset 256, 257 or 258 of output then it will not properly parse
 * the headers.  Curiously it doesn't exhibit this problem at 512, 513.
 * We are guessing that this is because their initial read of a new request
 * uses a 256 byte buffer, and subsequent reads use a larger buffer.
 * So the problem might exist at different offsets as well.
 *
 * This should also work on keepalive connections assuming they use the
 * same small buffer for the first read of each new request.
 *
 * At any rate, we check the bytes written so far and, if we are about to
 * tickle the bug, we instead insert a bogus padding header.  Since the bug
 * manifests as a broken image in Navigator, users blame the server.  :(
 * It is more expensive to check the User-Agent than it is to just add the
 * bytes, so we haven't used the BrowserMatch feature here.
 */
static void terminate_header(apr_bucket_brigade * bb)
{
    char tmp[] = "X-Pad: avoid browser bug" CRLF;
    char crlf[] = CRLF;
    apr_off_t len;
    apr_size_t buflen;

    (void) apr_brigade_length(bb, 1, &len);

    if (len >= 255 && len <= 257) {
		buflen = strlen(tmp);
		// FIXME 
		//ap_xlate_proto_to_ascii(tmp, buflen);
		apr_brigade_write(bb, NULL, NULL, tmp, buflen);
    }
    buflen = strlen(crlf);
    // FIXME 
    //ap_xlate_proto_to_ascii(crlf, buflen);
    apr_brigade_write(bb, NULL, NULL, crlf, buflen);
}

/* This routine is called by apr_table_do and merges all instances of
 * the passed field values into a single array that will be further
 * processed by some later routine.  Originally intended to help split
 * and recombine multiple Vary fields, though it is generic to any field
 * consisting of comma/space-separated tokens.
 */
static int uniq_field_values(void *d, const char *key, const char *val)
{
    apr_array_header_t *values;
    char *start;
    char *e;
    char **strpp;
    int  i;

    values = (apr_array_header_t *)d;

    e = apr_pstrdup(values->pool, val);

    do {
        /* Find a non-empty fieldname */

        while (*e == ',' || apr_isspace(*e)) {
            ++e;
        }
        if (*e == '\0') {
            break;
        }
        start = e;
        while (*e != '\0' && *e != ',' && !apr_isspace(*e)) {
            ++e;
        }
        if (*e != '\0') {
            *e++ = '\0';
        }

        /* Now add it to values if it isn't already represented.
         * Could be replaced by a ap_array_strcasecmp() if we had one.
         */
        for (i = 0, strpp = (char **) values->elts; i < values->nelts;
             ++i, ++strpp) {
            if (*strpp && strcasecmp(*strpp, start) == 0) {
                break;
            }
        }
        if (i == values->nelts) {  /* if not found */
            *(char **)apr_array_push(values) = start;
        }
    } while (*e != '\0');

    return 1;
}

/*
 * Since some clients choke violently on multiple Vary fields, or
 * Vary fields with duplicate tokens, combine any multiples and remove
 * any duplicates.
 */

static void fixup_vary(request_rec *r)
{
    apr_array_header_t *varies;

    varies = apr_array_make(r->pool, 5, sizeof(char *));

    /* Extract all Vary fields from the headers_out, separate each into
     * its comma-separated fieldname values, and then add them to varies
     * if not already present in the array.
     */
    apr_table_do((int (*)(void *, const char *, const char *))uniq_field_values,
                 (void *) varies, r->headers_out, "Vary", NULL);

    /* If we found any, replace old Vary fields with unique-ified value */

    if (varies->nelts > 0) {
        apr_table_setn(r->headers_out, "Vary",
                       apr_array_pstrcat(r->pool, varies, ','));
    }
}

typedef struct header_filter_ctx {
    int headers_sent;
} header_filter_ctx;

AP_CORE_DECLARE_NONSTD(apr_status_t) ap_http_header_filter(ap_filter_t *f,
                                                           apr_bucket_brigade *b)
{
    request_rec *r = f->r;
    conn_rec *c = r->connection;
    const char *clheader;
    const char *protocol;
    apr_bucket *e;
    apr_bucket_brigade *b2;
    header_struct h;
    header_filter_ctx *ctx = f->ctx;

    SB_DEBUG_ASSERT(!r->main);

    if (r->header_only) {
        if (!ctx) {
            ctx = f->ctx = apr_pcalloc(r->pool, sizeof(header_filter_ctx));
        }
        else if (ctx->headers_sent) {
            apr_brigade_destroy(b);
            return SUCCESS;
        }
    }

    APR_BRIGADE_FOREACH(e, b) {
        if (e->type == &ap_bucket_type_error) {
            ap_bucket_error *eb = e->data;

            ap_die(eb->status, r);
            return AP_FILTER_ERROR;
        }
    }

    if (r->assbackwards) {
        r->sent_bodyct = 1;
        ap_remove_output_filter(f);
        return ap_pass_brigade(f->next, b);
    }

    /*
     * Now that we are ready to send a response, we need to combine the two
     * header field tables into a single table.  If we don't do this, our
     * later attempts to set or unset a given fieldname might be bypassed.
     */
    if (!apr_is_empty_table(r->err_headers_out)) {
        r->headers_out = apr_table_overlay(r->pool, r->err_headers_out,
                                           r->headers_out);
    }

    /*
     * Remove the 'Vary' header field if the client can't handle it.
     * Since this will have nasty effects on HTTP/1.1 caches, force
     * the response into HTTP/1.0 mode.
     *
     * Note: the force-response-1.0 should come before the call to
     *       basic_http_header_check()
     */
    if (apr_table_get(r->subprocess_env, "force-no-vary") != NULL) {
        apr_table_unset(r->headers_out, "Vary");
        r->proto_num = HTTP_VERSION(1,0);
        apr_table_set(r->subprocess_env, "force-response-1.0", "1");
    }
    else {
        fixup_vary(r);
    }

    /*
     * Now remove any ETag response header field if earlier processing
     * says so (such as a 'FileETag None' directive).
     */
    if (apr_table_get(r->notes, "no-etag") != NULL) {
        apr_table_unset(r->headers_out, "ETag");
    }

    /* determine the protocol and whether we should use keepalives. */
    basic_http_header_check(r, &protocol);
    ap_set_keepalive(r);

    if (r->chunked) {
        apr_table_mergen(r->headers_out, "Transfer-Encoding", "chunked");
        apr_table_unset(r->headers_out, "Content-Length");
    }

    apr_table_setn(r->headers_out, "Content-Type", 
                   ap_make_content_type(r, r->content_type));

    if (r->content_encoding) {
        apr_table_setn(r->headers_out, "Content-Encoding",
                       r->content_encoding);
    }

    if (!apr_is_empty_table(r->content_languages)) {
        int i;
        char **languages = (char **)(r->content_languages->elts);
        for (i = 0; i < r->content_languages->nelts; ++i) {
            apr_table_mergen(r->headers_out, "Content-Language", languages[i]);
        }
    }

    /*
     * Control cachability for non-cachable responses if not already set by
     * some other part of the server configuration.
     */
    if (r->no_cache && !apr_table_get(r->headers_out, "Expires")) {
        char *date = apr_palloc(r->pool, APR_RFC822_DATE_LEN);
        ap_recent_rfc822_date(date, r->request_time);
        apr_table_addn(r->headers_out, "Expires", date);
    }

    /* This is a hack, but I can't find anyway around it.  The idea is that
     * we don't want to send out 0 Content-Lengths if it is a head request.
     * This happens when modules try to outsmart the server, and return
     * if they see a HEAD request.  Apache 1.3 handlers were supposed to
     * just return in that situation, and the core handled the HEAD.  In
     * 2.0, if a handler returns, then the core sends an EOS bucket down
     * the filter stack, and the content-length filter computes a C-L of
     * zero and that gets put in the headers, and we end up sending a
     * zero C-L to the client.  We can't just remove the C-L filter,
     * because well behaved 2.0 handlers will send their data down the stack,
     * and we will compute a real C-L for the head request. RBB
     */
    if (r->header_only
        && (clheader = apr_table_get(r->headers_out, "Content-Length"))
        && !strcmp(clheader, "0")) {
        apr_table_unset(r->headers_out, "Content-Length");
    }

    b2 = apr_brigade_create(r->pool, c->bucket_alloc);
    basic_http_header(r, b2, protocol);

    h.pool = r->pool;
    h.bb = b2;

    if (r->status == HTTP_NOT_MODIFIED) {
        apr_table_do((int (*)(void *, const char *, const char *)) form_header_field,
                     (void *) &h, r->headers_out,
                     "Connection",
                     "Keep-Alive",
                     "ETag",
                     "Content-Location",
                     "Expires",
                     "Cache-Control",
                     "Vary",
                     "Warning",
                     "WWW-Authenticate",
                     "Proxy-Authenticate",
                     NULL);
    }
    else {
        apr_table_do((int (*) (void *, const char *, const char *)) form_header_field,
                     (void *) &h, r->headers_out, NULL);
    }

    terminate_header(b2);

    ap_pass_brigade(f->next, b2);

    if (r->header_only) {
        apr_brigade_destroy(b);
        ctx->headers_sent = 1;
        return SUCCESS;
    }

    r->sent_bodyct = 1;         /* Whatever follows is real body stuff... */

    if (r->chunked) {
        /* We can't add this filter until we have already sent the headers.
         * If we add it before this point, then the headers will be chunked
         * as well, and that is just wrong.
         */
        ap_add_output_filter("CHUNK", NULL, r, r->connection);
    }

    /* Don't remove this filter until after we have added the CHUNK filter.
     * Otherwise, f->next won't be the CHUNK filter and thus the first
     * brigade won't be chunked properly.
     */
    ap_remove_output_filter(f);
    return ap_pass_brigade(f->next, b);
}

/*****************************************************************************/
/* FILTER : byterange */

static int parse_byterange(char *range, apr_off_t clength,
                           apr_off_t *start, apr_off_t *end)
{
    char *dash = strchr(range, '-');

    if (!dash) {
        return 0;
    }

    if ((dash == range)) {
        /* In the form "-5" */
        *start = clength - atol(dash + 1);
        *end = clength - 1;
    }
    else {
        *dash = '\0';
        dash++;
        *start = atol(range);
        if (*dash) {
            *end = atol(dash);
        }
        else {                  /* "5-" */
            *end = clength - 1;
        }
    }

    if (*start < 0) {
        *start = 0;
    }

    if (*end >= clength) {
        *end = clength - 1;
    }

    if (*start > *end) {
        return -1;
    }

    return (*start > 0 || *end < clength);
}

static int ap_set_byterange(request_rec *r);

typedef struct byterange_ctx {
    apr_bucket_brigade *bb;
    int num_ranges;
    char *boundary;
    char *bound_head;
} byterange_ctx;

/*
 * Here we try to be compatible with clients that want multipart/x-byteranges
 * instead of multipart/byteranges (also see above), as per HTTP/1.1. We
 * look for the Request-Range header (e.g. Netscape 2 and 3) as an indication
 * that the browser supports an older protocol. We also check User-Agent
 * for Microsoft Internet Explorer 3, which needs this as well.
 */
static int use_range_x(request_rec *r)
{
    const char *ua;
    return (apr_table_get(r->headers_in, "Request-Range")
            || ((ua = apr_table_get(r->headers_in, "User-Agent"))
                && ap_strstr_c(ua, "MSIE 3")));
}

#define BYTERANGE_FMT "%" APR_OFF_T_FMT "-%" APR_OFF_T_FMT "/%" APR_OFF_T_FMT
#define PARTITION_ERR_FMT "apr_brigade_partition() failed " \
                          "[%" APR_OFF_T_FMT ",%" APR_OFF_T_FMT "]"

apr_status_t ap_byterange_filter(ap_filter_t *f, apr_bucket_brigade *bb)
{
#define MIN_LENGTH(len1, len2) ((len1 > len2) ? len2 : len1)
    request_rec *r = f->r;
    conn_rec *c = r->connection;
    byterange_ctx *ctx = f->ctx;
    apr_bucket *e;
    apr_bucket_brigade *bsend;
    apr_off_t range_start;
    apr_off_t range_end;
    char *current;
    apr_off_t bb_length;
    apr_off_t clength = 0;
    apr_status_t rv;
    int found = 0;

    if (!ctx) {
        int num_ranges = ap_set_byterange(r);

        /* We have nothing to do, get out of the way. */
        if (num_ranges == 0) {
            ap_remove_output_filter(f);
            return ap_pass_brigade(f->next, bb);
        }

        ctx = f->ctx = apr_pcalloc(r->pool, sizeof(*ctx));
        ctx->num_ranges = num_ranges;
        /* create a brigade in case we never call ap_save_brigade() */
        ctx->bb = apr_brigade_create(r->pool, c->bucket_alloc);

        if (ctx->num_ranges > 1) {
            /* Is ap_make_content_type required here? */
            const char *orig_ct = ap_make_content_type(r, r->content_type);
            ctx->boundary = apr_psprintf(r->pool, "%qx%lx",
                                         r->request_time, (long) getpid());

            ap_set_content_type(r, apr_pstrcat(r->pool, "multipart",
                                               use_range_x(r) ? "/x-" : "/",
                                               "byteranges; boundary=",
                                               ctx->boundary, NULL));

            ctx->bound_head = apr_pstrcat(r->pool,
                                    CRLF "--", ctx->boundary,
                                    CRLF "Content-type: ",
                                    orig_ct,
                                    CRLF "Content-range: bytes ",
                                    NULL);
			// FIXME
            //ap_xlate_proto_to_ascii(ctx->bound_head, strlen(ctx->bound_head));
        }
    }

    /* We can't actually deal with byte-ranges until we have the whole brigade
     * because the byte-ranges can be in any order, and according to the RFC,
     * we SHOULD return the data in the same order it was requested.
     */
    if (!APR_BUCKET_IS_EOS(APR_BRIGADE_LAST(bb))) {
        ap_save_brigade(f, &ctx->bb, &bb, r->pool);
        return APR_SUCCESS;
    }

    /* Prepend any earlier saved brigades. */
    APR_BRIGADE_PREPEND(bb, ctx->bb);

    /* It is possible that we won't have a content length yet, so we have to
     * compute the length before we can actually do the byterange work.
     */
    apr_brigade_length(bb, 1, &bb_length);
    clength = (apr_off_t)bb_length;

    /* this brigade holds what we will be sending */
    bsend = apr_brigade_create(r->pool, c->bucket_alloc);

    while ((current = ap_getword(r->pool, &r->range, ','))
           && (rv = parse_byterange(current, clength, &range_start,
                                    &range_end))) {
        apr_bucket *e2;
        apr_bucket *ec;

        if (rv == -1) {
            continue;
        }

        /* these calls to apr_brigade_partition() should theoretically
         * never fail because of the above call to apr_brigade_length(),
         * but what the heck, we'll check for an error anyway */
        if ((rv = apr_brigade_partition(bb, range_start, &ec)) != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                          PARTITION_ERR_FMT, range_start, clength);
            continue;
        }
        if ((rv = apr_brigade_partition(bb, range_end+1, &e2)) != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                          PARTITION_ERR_FMT, range_end+1, clength);
            continue;
        }

        found = 1;

        /* For single range requests, we must produce Content-Range header.
         * Otherwise, we need to produce the multipart boundaries.
         */
        if (ctx->num_ranges == 1) {
            apr_table_setn(r->headers_out, "Content-Range",
                           apr_psprintf(r->pool, "bytes " BYTERANGE_FMT,
                                        range_start, range_end, clength));
        }
        else {
            char *ts;

            e = apr_bucket_pool_create(ctx->bound_head, strlen(ctx->bound_head),
                                       r->pool, c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(bsend, e);

            ts = apr_psprintf(r->pool, BYTERANGE_FMT CRLF CRLF,
                              range_start, range_end, clength);
			// FIXME
            //ap_xlate_proto_to_ascii(ts, strlen(ts));
            e = apr_bucket_pool_create(ts, strlen(ts), r->pool,
                                       c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(bsend, e);
        }

        do {
            apr_bucket *foo;
            const char *str;
            apr_size_t len;

            if (apr_bucket_copy(ec, &foo) != APR_SUCCESS) {
                /* this shouldn't ever happen due to the call to
                 * apr_brigade_length() above which normalizes
                 * indeterminate-length buckets.  just to be sure,
                 * though, this takes care of uncopyable buckets that
                 * do somehow manage to slip through.
                 */
                /* XXX: check for failure? */
                apr_bucket_read(ec, &str, &len, APR_BLOCK_READ);
                apr_bucket_copy(ec, &foo);
            }
            APR_BRIGADE_INSERT_TAIL(bsend, foo);
            ec = APR_BUCKET_NEXT(ec);
        } while (ec != e2);
    }

    if (found == 0) {
        ap_remove_output_filter(f);
        r->status = HTTP_OK;
        /* bsend is assumed to be empty if we get here. */
        e = ap_bucket_error_create(HTTP_RANGE_NOT_SATISFIABLE, NULL,
                                   r->pool, c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bsend, e);
        e = apr_bucket_eos_create(c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bsend, e);
        return ap_pass_brigade(f->next, bsend);
    }

    if (ctx->num_ranges > 1) {
        char *end;

        /* add the final boundary */
        end = apr_pstrcat(r->pool, CRLF "--", ctx->boundary, "--" CRLF, NULL);
		// FIXME
        //ap_xlate_proto_to_ascii(end, strlen(end));
        e = apr_bucket_pool_create(end, strlen(end), r->pool, c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bsend, e);
    }

    e = apr_bucket_eos_create(c->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bsend, e);

    /* we're done with the original content - all of our data is in bsend. */
    apr_brigade_destroy(bb);

    /* send our multipart output */
    return ap_pass_brigade(f->next, bsend);
}

static int ap_set_byterange(request_rec *r)
{
    const char *range;
    const char *if_range;
    const char *match;
    const char *ct;
    int num_ranges;

    if (r->assbackwards) {
        return 0;
    }

    /* Check for Range request-header (HTTP/1.1) or Request-Range for
     * backwards-compatibility with second-draft Luotonen/Franks
     * byte-ranges (e.g. Netscape Navigator 2-3).
     *
     * We support this form, with Request-Range, and (farther down) we
     * send multipart/x-byteranges instead of multipart/byteranges for
     * Request-Range based requests to work around a bug in Netscape
     * Navigator 2-3 and MSIE 3.
     */

    if (!(range = apr_table_get(r->headers_in, "Range"))) {
        range = apr_table_get(r->headers_in, "Request-Range");
    }

    if (!range || strncasecmp(range, "bytes=", 6)) {
        return 0;
    }

    /* is content already a single range? */
    if (apr_table_get(r->headers_out, "Content-Range")) {
       return 0;
    }

    /* is content already a multiple range? */
    if ((ct = apr_table_get(r->headers_out, "Content-Type"))
        && (!strncasecmp(ct, "multipart/byteranges", 20)
            || !strncasecmp(ct, "multipart/x-byteranges", 22))) {
       return 0;
    }

    /* Check the If-Range header for Etag or Date.
     * Note that this check will return false (as required) if either
     * of the two etags are weak.
     */
    if ((if_range = apr_table_get(r->headers_in, "If-Range"))) {
        if (if_range[0] == '"') {
            if (!(match = apr_table_get(r->headers_out, "Etag"))
                || (strcmp(if_range, match) != 0)) {
                return 0;
            }
        }
        else if (!(match = apr_table_get(r->headers_out, "Last-Modified"))
                 || (strcmp(if_range, match) != 0)) {
            return 0;
        }
    }

    if (!ap_strchr_c(range, ',')) {
        /* a single range */
        num_ranges = 1;
    }
    else {
        /* a multiple range */
        num_ranges = 2;
    }

    r->status = HTTP_PARTIAL_CONTENT;
    r->range = range + 6;

    return num_ranges;
}

/*****************************************************************************/
/* FILTER : content_length */

struct content_length_ctx {
    apr_bucket_brigade *saved;
    int compute_len;
    apr_size_t curr_len;
};

/* This filter computes the content length, but it also computes the number
 * of bytes sent to the client.  This means that this filter will always run
 * through all of the buckets in all brigades
 */
apr_status_t ap_content_length_filter(ap_filter_t *f, apr_bucket_brigade *bb)
{
    request_rec *r = f->r;
    struct content_length_ctx *ctx;
    apr_status_t rv;
    apr_bucket *e;
    int eos = 0, flush = 0, partial_send_okay = 0;
    apr_bucket_brigade *more, *split;
    apr_read_type_e eblock = APR_NONBLOCK_READ;

    ctx = f->ctx;
    if (!ctx) { /* first time through */
        f->ctx = ctx = apr_pcalloc(r->pool, sizeof(struct content_length_ctx));
        ctx->compute_len = 1;   /* Assume we will compute the length */
        ctx->saved = apr_brigade_create(r->pool, f->c->bucket_alloc);
    }

    /* Humm, is this check the best it can be?
     * - protocol >= HTTP/1.1 implies support for chunking
     * - non-keepalive implies the end of byte stream will be signaled
     *    by a connection close
     * In both cases, we can send bytes to the client w/o needing to
     * compute content-length.
     * Todo:
     * We should be able to force connection close from this filter
     * when we see we are buffering too much.
     */
    if ((r->proto_num >= HTTP_VERSION(1, 1)) || (!r->connection->keepalive)) {
        partial_send_okay = 1;
    }

    more = bb;
    while (more) {
        bb = more;
        more = NULL;
        split = NULL;
        flush = 0;

        APR_BRIGADE_FOREACH(e, bb) {
            const char *ignored;
            apr_size_t len;
            len = 0;
            if (APR_BUCKET_IS_EOS(e)) {
                eos = 1;
            }
            else if (APR_BUCKET_IS_FLUSH(e)) {
                if (partial_send_okay) {
                    split = bb;
                    more = apr_brigade_split(bb, APR_BUCKET_NEXT(e));
                    break;
                }
            }
            else if ((ctx->curr_len > 4 * AP_MIN_BYTES_TO_WRITE)) {
                /* If we've accumulated more than 4xAP_MIN_BYTES_TO_WRITE and
                 * the client supports chunked encoding, send what we have
                 * and come back for more.
                 */
                if (partial_send_okay) {
                    split = bb;
                    more = apr_brigade_split(bb, e);
                    break;
                }
            }
            if (e->length == -1) { /* if length unknown */
                rv = apr_bucket_read(e, &ignored, &len, eblock);
                if (rv == APR_SUCCESS) {
                    /* Attempt a nonblocking read next time through */
                    eblock = APR_NONBLOCK_READ;
                }
                else if (APR_STATUS_IS_EAGAIN(rv)) {
                    /* Make the next read blocking.  If the client supports
                     * chunked encoding, flush the filter stack to the network.
                     */
                    eblock = APR_BLOCK_READ;
                    if (partial_send_okay) {
                        split = bb;
                        more = apr_brigade_split(bb, e);
                        flush = 1;
                        break;
                    }
                }
                else if (rv != APR_EOF) {
                    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                                  "ap_content_length_filter: "
                                  "apr_bucket_read() failed");
                    return rv;
                }
            }
            else {
                len = e->length;
            }

            ctx->curr_len += len;
            r->bytes_sent += len;
        }

        if (split) {
            ctx->compute_len = 0;  /* Ooops, can't compute the length now */
            ctx->curr_len = 0;

            APR_BRIGADE_PREPEND(split, ctx->saved);

            if (flush) {
                rv = ap_fflush(f->next, split);
            }
            else {
                rv = ap_pass_brigade(f->next, split);
            }

            if (rv != APR_SUCCESS)
                return rv;
        }
    }

    if ((ctx->curr_len < AP_MIN_BYTES_TO_WRITE) && !eos) {
        return ap_save_brigade(f, &ctx->saved, &bb,
                               (r->main) ? r->main->pool : r->pool);
    }

    if (ctx->compute_len) {
        /* save the brigade; we can't pass any data to the next
         * filter until we have the entire content length
         */
        if (!eos) {
            return ap_save_brigade(f, &ctx->saved, &bb, r->pool);
        }

        ap_set_content_length(r, r->bytes_sent);
    }

    APR_BRIGADE_PREPEND(bb, ctx->saved);

    ctx->curr_len = 0;
    return ap_pass_brigade(f->next, bb);
}



