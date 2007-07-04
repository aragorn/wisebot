/* $Id$ */
/*
 * http_core.c
 */
#include "common_core.h"
#include "mod_httpd.h"
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
#include "apr_strings.h"

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

		for (e = APR_BRIGADE_FIRST(b);
		     e != APR_BRIGADE_SENTINEL(b);
		     e = APR_BUCKET_NEXT(e))
		{
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

        /* FIXME : always! connection close -blueend */
		c->keepalive = 0;
        debug("c->keepalive[%d]", c->keepalive);

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
	NULL,					/* config */
	NULL,					/* registry */
	module_init,			/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};

