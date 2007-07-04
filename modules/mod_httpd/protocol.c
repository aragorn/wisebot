/* $Id$ */
/*****************************************************************************/
/* protocol.c */
#include "common_core.h"
#include "mod_httpd.h"
#include "http_config.h"

#define CORE_PRIVATE
#include "http_core.h"
#include "protocol.h"
#include "filter.h"
#include "http_protocol.h"
#include "request.h"
#include "http_request.h"
#include "util.h"
#include "util_filter.h" // FIXME move to protocol.h??
#include "http_filter.h"
#include "http_util.h"
#include "util_string.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_strmatch.h" /* apr_strmatch_pattern */
#include "apr_version.h"
#include "log.h" /* ap_log_rerror */

#ifndef DEFAULT_LOCKFILE
#  define DEFAULT_LOCKFILE "/tmp/softbot_httpd_lock"
#endif

// FIXME move to http_protocol.c ??
HOOK_STRUCT(
    HOOK_LINK(post_read_request)
    HOOK_LINK(log_transaction)
    HOOK_LINK(http_method)
    HOOK_LINK(default_port)
)

SB_IMPLEMENT_HOOK_RUN_ALL(int, post_read_request, (request_rec * r), (r),
	SUCCESS, DECLINE) 
SB_IMPLEMENT_HOOK_RUN_ALL(int, log_transaction, (request_rec * r), (r),
	SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(const char *, http_method, (const request_rec * r),
	(r), NULL)
SB_IMPLEMENT_HOOK_RUN_FIRST(apr_port_t, default_port, (const request_rec * r),
	(r), 0)


ap_filter_rec_t *ap_old_write_func = NULL;

/*****************************************************************************/

/* Min # of bytes to allocate when reading a request line */
#define MIN_LINE_ALLOC 80

/* Get a line of protocol input, including any continuation lines
 * caused by MIME folding (or broken clients) if fold != 0, and place it
 * in the buffer s, of size n bytes, without the ending newline.
 *
 * If s is NULL, sb_rgetline will allocate necessary memory from r->pool.
 *
 * Returns APR_SUCCESS if there are no problems and sets *read to be
 * the full length of s.
 *
 * APR_ENOSPC is returned if there is not enough buffer space.
 * Other errors may be returned on other errors.
 *
 * The LF is *not* returned in the buffer.  Therefore, a *read of 0
 * indicates that an empty line was read.
 *
 * Notes: Because the buffer uses 1 char for NUL, the most we can return is
 *        (n - 1) actual characters.
 *
 *        If no LF is detected on the last line due to a dropped connection
 *        or a full buffer, that's considered an error.
 */

apr_status_t ap_rgetline(char **s, apr_size_t n,
			 apr_size_t * read, request_rec * r, int fold)
{
    apr_status_t rv;
    apr_bucket_brigade *b;
    apr_bucket *e;
    apr_size_t bytes_handled = 0, current_alloc = 0;
    char *pos, *last_char = *s;
    int do_alloc = (*s == NULL), saw_eos = 0;

    b = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    rv = ap_get_brigade(r->input_filters, b, AP_MODE_GETLINE,
			APR_BLOCK_READ, 0);
/*	debug("ap_get_brigade() returned rv[%d]", rv);*/

    if (rv != APR_SUCCESS) {
		apr_brigade_destroy(b);
		return rv;
    }

    /* Something horribly wrong happened.  Someone didn't block! */
    if (APR_BRIGADE_EMPTY(b)) {
		apr_brigade_destroy(b);
		return APR_EGENERAL;
    }

#if APR_MAJOR_VERSION == 1
	for (e = APR_BRIGADE_FIRST(b);
	     e != APR_BRIGADE_SENTINEL(b);
	     e = APR_BUCKET_NEXT(e))
#else
    APR_BRIGADE_FOREACH(e, b)
#endif
	{
		const char *str;
		apr_size_t len;

		/* If we see an EOS, don't bother doing anything more. */
		if (APR_BUCKET_IS_EOS(e)) {
			saw_eos = 1;
			break;
		}

		rv = apr_bucket_read(e, &str, &len, APR_BLOCK_READ);

		if (rv != APR_SUCCESS) {
			apr_brigade_destroy(b);
			return rv;
		}

		if (len == 0) {
			/* no use attempting a zero-byte alloc (hurts when
			 * using --with-efence --enable-pool-debug) or
			 * doing any of the other logic either
			 */
			continue;
		}

		/* Would this overrun our buffer?  If so, we'll die. */
		if (n < bytes_handled + len) {
			apr_brigade_destroy(b);
			return APR_ENOSPC;
		}

		/* Do we have to handle the allocation ourselves? */
		if (do_alloc) {
			/* We'll assume the common case where one bucket is enough. */
			if (!*s) {
			current_alloc = len;
			if (current_alloc < MIN_LINE_ALLOC) {
				current_alloc = MIN_LINE_ALLOC;
			}
			*s = apr_palloc(r->pool, current_alloc);
			}
			else if (bytes_handled + len > current_alloc) {
			/* Increase the buffer size */
			apr_size_t new_size = current_alloc * 2;
			char *new_buffer;

			if (bytes_handled + len > new_size) {
				new_size = (bytes_handled + len) * 2;
			}

			new_buffer = apr_palloc(r->pool, new_size);

			/* Copy what we already had. */
			memcpy(new_buffer, *s, bytes_handled);
			current_alloc = new_size;
			*s = new_buffer;
			}
		}

		/* Just copy the rest of the data to the end of the old buffer. */
		pos = *s + bytes_handled;
		memcpy(pos, str, len);
		last_char = pos + len - 1;

		/* We've now processed that new data - update accordingly. */
		bytes_handled += len;
    }

    /* We no longer need the returned brigade. */
    apr_brigade_destroy(b);

    /* We likely aborted early before reading anything or we read no
     * data.  Technically, this might be success condition.  But,
     * probably means something is horribly wrong.  For now, we'll
     * treat this as APR_SUCCESS, but it may be worth re-examining.
     */
    if (bytes_handled == 0) {
		*read = 0;
		return APR_SUCCESS;
    }

    /* If we didn't get a full line of input, try again. */
    if (*last_char != APR_ASCII_LF) {
	/* Do we have enough space? We may be full now. */
	if (bytes_handled < n) {
	    apr_size_t next_size, next_len;
	    char *tmp;

	    /* If we're doing the allocations for them, we have to
	     * give ourselves a NULL and copy it on return.
	     */
	    if (do_alloc) {
		tmp = NULL;
	    }
	    else {
		/* We're not null terminated yet. */
		tmp = last_char + 1;
	    }

	    next_size = n - bytes_handled;

	    rv = ap_rgetline(&tmp, next_size, &next_len, r, fold);

	    if (rv != APR_SUCCESS) {
		return rv;
	    }

	    /* XXX this code appears to be dead because the filter chain
	     * seems to read until it sees a LF or an error.  If it ever
	     * comes back to life, we need to make sure that:
	     * - we really alloc enough space for the trailing null
	     * - we don't allow the tail trimming code to run more than
	     *   once
	     */
	    if (do_alloc && next_len > 0) {
		char *new_buffer;
		apr_size_t new_size = bytes_handled + next_len;

		/* Again we need to alloc an extra two bytes for LF, null */
		new_buffer = apr_palloc(r->pool, new_size);

		/* Copy what we already had. */
		memcpy(new_buffer, *s, bytes_handled);
		memcpy(new_buffer + bytes_handled, tmp, next_len);
		current_alloc = new_size;
		*s = new_buffer;
	    }

	    bytes_handled += next_len;
	    last_char = *s + bytes_handled - 1;
	}
	else {
	    return APR_ENOSPC;
	}
    }

    /* We now go backwards over any CR (if present) or white spaces.
     *
     * Trim any extra trailing spaces or tabs except for the first
     * space or tab at the beginning of a blank string.  This makes
     * it much easier to check field values for exact matches, and
     * saves memory as well.  Terminate string at end of line.
     */
    pos = last_char;
    if (pos > *s && *(pos - 1) == APR_ASCII_CR) {
		--pos;
    }

    /* Trim any extra trailing spaces or tabs except for the first
     * space or tab at the beginning of a blank string.  This makes
     * it much easier to check field values for exact matches, and
     * saves memory as well.
     */
    while (pos > ((*s) + 1)
		   && (*(pos - 1) == APR_ASCII_BLANK
			   || *(pos - 1) == APR_ASCII_TAB)) {
		--pos;
    }

    /* Since we want to remove the LF from the line, we'll go ahead
     * and set this last character to be the term NULL and reset
     * bytes_handled accordingly.
     */
    *pos = '\0';
    last_char = pos;
    bytes_handled = pos - *s;

    /* If we're folding, we have more work to do.
     *
     * Note that if an EOS was seen, we know we can't have another line.
     */
    if (fold && bytes_handled && !saw_eos) {
		const char *str;
		apr_bucket_brigade *bb;
		apr_size_t len;
		char c;

		/* Create a brigade for this filter read. */
		bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);

		/* We only care about the first byte. */
		rv = ap_get_brigade(r->input_filters, bb, AP_MODE_SPECULATIVE,
					APR_BLOCK_READ, 1);

		if (rv != APR_SUCCESS) {
			apr_brigade_destroy(bb);
			return rv;
		}

		if (APR_BRIGADE_EMPTY(bb)) {
			*read = bytes_handled;
			apr_brigade_destroy(bb);
			return APR_SUCCESS;
		}

		e = APR_BRIGADE_FIRST(bb);

		/* If we see an EOS, don't bother doing anything more. */
		if (APR_BUCKET_IS_EOS(e)) {
			*read = bytes_handled;
			apr_brigade_destroy(bb);
			return APR_SUCCESS;
		}

		rv = apr_bucket_read(e, &str, &len, APR_BLOCK_READ);

		if (rv != APR_SUCCESS) {
			apr_brigade_destroy(bb);
			return rv;
		}

		/* When we call destroy, the buckets are deleted, so save that
		 * one character we need.  This simplifies our execution paths
		 * at the cost of one character read.
		 */
		c = *str;

		/* We no longer need the returned brigade. */
		apr_brigade_destroy(bb);

		/* Found one, so call ourselves again to get the next line.
		 *
		 * FIXME: If the folding line is completely blank, should we
		 * stop folding?  Does that require also looking at the next
		 * char?
		 */
		if (c == APR_ASCII_BLANK || c == APR_ASCII_TAB) {
			/* Do we have enough space? We may be full now. */
			if (bytes_handled < n) {
				apr_size_t next_size, next_len;
				char *tmp;

				/* If we're doing the allocations for them, we have to
				 * give ourselves a NULL and copy it on return.
				 */
				if (do_alloc) {
					tmp = NULL;
				}
				else {
					/* We're null terminated. */
					tmp = last_char;
				}

				next_size = n - bytes_handled;

				rv = ap_rgetline(&tmp, next_size, &next_len, r, fold);

				if (rv != APR_SUCCESS) {
					return rv;
				}

				if (do_alloc && next_len > 0) {
					char *new_buffer;
					apr_size_t new_size = bytes_handled + next_len + 1;

					/* we need to alloc an extra byte for a null */
					new_buffer = apr_palloc(r->pool, new_size);

					/* Copy what we already had. */
					memcpy(new_buffer, *s, bytes_handled);

					/* copy the new line, including the trailing null */
					memcpy(new_buffer + bytes_handled, tmp, next_len + 1);
					*s = new_buffer;
				}

				*read = bytes_handled + next_len;
				return APR_SUCCESS;
			}
			else {
				return APR_ENOSPC;
			}
		}
    }

    *read = bytes_handled;
    return APR_SUCCESS;
}



/*
 * Return the latest rational time from a request/mtime (modification time)
 * pair.  We return the mtime unless it's in the future, in which case we
 * return the current time.  We use the request time as a reference in order
 * to limit the number of calls to time().  We don't check for futurosity
 * unless the mtime is at least as new as the reference.
 */
apr_time_t ap_rationalize_mtime(request_rec *r, apr_time_t mtime)
{
    apr_time_t now;

    /* For all static responses, it's almost certain that the file was
     * last modified before the beginning of the request.  So there's
     * no reason to call time(NULL) again.  But if the response has been
     * created on demand, then it might be newer than the time the request
     * started.  In this event we really have to call time(NULL) again
     * so that we can give the clients the most accurate Last-Modified.  If we
     * were given a time in the future, we return the current time - the
     * Last-Modified can't be in the future.
     */
    now = (mtime < r->request_time) ? r->request_time : apr_time_now();
    return (mtime > now) ? now : mtime;
}




static void end_output_stream(request_rec *r)
{
	conn_rec *c = r->connection;
	apr_bucket_brigade *bb;
	apr_bucket *b;

	bb = apr_brigade_create(r->pool, c->bucket_alloc);
	b = apr_bucket_eos_create(c->bucket_alloc);
	APR_BRIGADE_INSERT_TAIL(bb, b);
	ap_pass_brigade(r->output_filters, bb);
}

void ap_finalize_sub_req_protocol(request_rec *sub)
{   
	/* tell the filter chain there is no more content coming */
	if (!sub->eos_sent) { 
		end_output_stream(sub);
	}
} 

/* finalize_request_protocol is called at completion of sending the
 * response.  Its sole purpose is to send the terminating protocol
 * information for any wrappers around the response message body
 * (i.e., transfer encodings).  It should have been named finalize_response.
 */
void ap_finalize_request_protocol(request_rec * r)
{
/* BTS removed this for now
    // FIXME function from http_protocol.c
    debug("ap_discard_request_body");
    (void) ap_discard_request_body(r);
*/

    while (r->next) {
        debug("r->next");
	r = r->next;
    }

    /* tell the filter chain there is no more content coming */
    debug("!r->eos_sent");
    if (!r->eos_sent) {
	debug("ap_finalize_request_protocol calls end_output_stream(r)");
	end_output_stream(r);
    }
}

/*****************************************************************************/
/* Patterns to match in ap_make_content_type() */
static const char *needcset[] = {
    "text/plain",
    "text/html",
    NULL
};
static const apr_strmatch_pattern **needcset_patterns;
static const apr_strmatch_pattern *charset_pattern;

AP_DECLARE(void) ap_setup_make_content_type(apr_pool_t *pool)
{
    int i;
    for (i = 0; needcset[i]; i++) {
        continue;
    }
    needcset_patterns = (const apr_strmatch_pattern **)
        apr_palloc(pool, (i + 1) * sizeof(apr_strmatch_pattern *));
    for (i = 0; needcset[i]; i++) {
        needcset_patterns[i] = apr_strmatch_precompile(pool, needcset[i], 0);
    }
    needcset_patterns[i] = NULL;
    charset_pattern = apr_strmatch_precompile(pool, "charset=", 0);
}

/*
 * Builds the content-type that should be sent to the client from the
 * content-type specified.  The following rules are followed:
 *    - if type is NULL, type is set to ap_default_type(r)
 *    - if charset adding is disabled, stop processing and return type.
 *    - then, if there are no parameters on type, add the default charset
 *    - return type
 */
AP_DECLARE(const char *)ap_make_content_type(request_rec *r, const char *type)
{
    const apr_strmatch_pattern **pcset;
    core_dir_config *conf =
        (core_dir_config *)ap_get_module_config(r->per_dir_config,
                                                CORE_HTTPD_MODULE);
    apr_size_t type_len;

    if (!type) {
        type = ap_default_type(r);
    }

    if (conf->add_default_charset != ADD_DEFAULT_CHARSET_ON) {
        return type;
    }

    type_len = strlen(type);

    if (apr_strmatch(charset_pattern, type, type_len) != NULL) {
        /* already has parameter, do nothing */
        /* XXX we don't check the validity */
        ;
    }
    else {
        /* see if it makes sense to add the charset. At present,
         * we only add it if the Content-type is one of needcset[]
         */
        for (pcset = needcset_patterns; *pcset ; pcset++) {
            if (apr_strmatch(*pcset, type, type_len) != NULL) {
                struct iovec concat[3];
                concat[0].iov_base = (void *)type;
                concat[0].iov_len = type_len;
                concat[1].iov_base = (void *)"; charset=";
                concat[1].iov_len = sizeof("; charset=") - 1;
                concat[2].iov_base = (void *)(conf->add_default_charset_name);
                concat[2].iov_len = strlen(conf->add_default_charset_name);
                type = apr_pstrcatv(r->pool, concat, 3, NULL);
                break;
            }
        }
    }

    return type;
}

/*
 * This function sets the Last-Modified output header field to the value
 * of the mtime field in the request structure - rationalized to keep it from
 * being in the future.
 */
void ap_set_last_modified(request_rec *r)
{
    if (!r->assbackwards) {
        apr_time_t mod_time = ap_rationalize_mtime(r, r->mtime);
        char *datestr = apr_palloc(r->pool, APR_RFC822_DATE_LEN);

        apr_rfc822_date(datestr, mod_time);
        apr_table_setn(r->headers_out, "Last-Modified", datestr);
    }
}


void ap_set_content_length(request_rec *r, apr_off_t clength)
{
    r->clength = clength;
    apr_table_setn(r->headers_out, "Content-Length",
                   apr_off_t_toa(r->pool, clength));
}


/*****************************************************************************/

static apr_status_t buffer_output(request_rec *r,
                                  const char *str, apr_size_t len)
{
    conn_rec *c = r->connection;
    ap_filter_t *f;
    old_write_filter_ctx *ctx;

    if (len == 0)
        return APR_SUCCESS;

    /* future optimization: record some flags in the request_rec to
     * say whether we've added our filter, and whether it is first.
     */

    /* this will typically exit on the first test */
    for (f = r->output_filters; f != NULL; f = f->next) {
        if (ap_old_write_func == f->frec)
            break;
    }

    if (f == NULL) {
        /* our filter hasn't been added yet */
        ctx = apr_pcalloc(r->pool, sizeof(*ctx));
        ap_add_output_filter("OLD_WRITE", ctx, r, r->connection);
        f = r->output_filters;
    }

    /* if the first filter is not our buffering filter, then we have to
     * deliver the content through the normal filter chain
     */
    if (f != r->output_filters) {
        apr_bucket_brigade *bb = apr_brigade_create(r->pool, c->bucket_alloc);
        apr_bucket *b = apr_bucket_transient_create(str, len, c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, b);

        return ap_pass_brigade(r->output_filters, bb);
    }

    /* grab the context from our filter */
    ctx = r->output_filters->ctx;

    { /* very kludgy - FIXME */
      struct apr_bucket_brigade *bb;
      int ret;

      if (ctx == NULL) {
	  /* patch a ctx into the filter, is this correct? - FIXME */
	  bb = apr_brigade_create(r->pool, c->bucket_alloc);
      } else {
    	  if (ctx->bb == NULL)
        	  bb = ctx->bb = apr_brigade_create(r->pool, c->bucket_alloc);
	  else    bb = ctx->bb;
      }

      ret = ap_fwrite(f->next, bb, str, len);

      if (ctx == NULL)
	  apr_brigade_destroy(bb);

      return ret;
    }
}

int ap_rputc(int c, request_rec *r)
{
    char c2 = (char)c;

    if (r->connection->aborted) {
        return -1;
    }

    if (buffer_output(r, &c2, 1) != APR_SUCCESS)
        return -1;

    return c;
}

int ap_rputs(const char *str, request_rec *r)
{
    apr_size_t len;

    if (r->connection->aborted)
        return -1;

    if (buffer_output(r, str, len = strlen(str)) != APR_SUCCESS)
        return -1;

    return len;
}

int ap_rwrite(const void *buf, int nbyte, request_rec *r)
{
    if (r->connection->aborted)
        return -1;

    if (buffer_output(r, buf, nbyte) != APR_SUCCESS)
        return -1;

    return nbyte;
}

struct ap_vrprintf_data {
    apr_vformatter_buff_t vbuff;
    request_rec *r;
    char *buff;
};

static apr_status_t r_flush(apr_vformatter_buff_t *buff)
{
    /* callback function passed to ap_vformatter to be called when
     * vformatter needs to write into buff and buff.curpos > buff.endpos */

    /* ap_vrprintf_data passed as a apr_vformatter_buff_t, which is then
     * "downcast" to an ap_vrprintf_data */
    struct ap_vrprintf_data *vd = (struct ap_vrprintf_data*)buff;

    if (vd->r->connection->aborted)
        return -1;

    /* r_flush is called when vbuff is completely full */
    if (buffer_output(vd->r, vd->buff, IOBUFSIZE)) {
        return -1;
    }

    /* reset the buffer position */
    vd->vbuff.curpos = vd->buff;
    vd->vbuff.endpos = vd->buff + IOBUFSIZE;

    return APR_SUCCESS;
}

int ap_vrprintf(request_rec *r, const char *fmt, va_list va)
{
    apr_size_t written;
    struct ap_vrprintf_data vd;
    char vrprintf_buf[IOBUFSIZE];

    vd.vbuff.curpos = vrprintf_buf;
    vd.vbuff.endpos = vrprintf_buf + IOBUFSIZE;
    vd.r = r;
    vd.buff = vrprintf_buf;

    if (r->connection->aborted)
        return -1;

    written = apr_vformatter(r_flush, &vd.vbuff, fmt, va);

    /* tack on null terminator on remaining string */
    *(vd.vbuff.curpos) = '\0';

    if (written != -1) {
        int n = vd.vbuff.curpos - vrprintf_buf;

        /* last call to buffer_output, to finish clearing the buffer */
        if (buffer_output(r, vrprintf_buf,n) != APR_SUCCESS)
            return -1;

        written += n;
    }

    return written;
}

int ap_rprintf(request_rec *r, const char *fmt, ...)
{
    va_list va;
    int n;

    if (r->connection->aborted)
        return -1;

    va_start(va, fmt);
    n = ap_vrprintf(r, fmt, va);
    va_end(va);

    return n;
}

int ap_rvputs(request_rec *r, ...)
{
    va_list va;
    const char *s;
    apr_size_t len;
    apr_size_t written = 0;

    if (r->connection->aborted)
        return -1;

    /* ### TODO: if the total output is large, put all the strings
     * ### into a single brigade, rather than flushing each time we
     * ### fill the buffer
     */
    va_start(va, r);
    while (1) {
        s = va_arg(va, const char *);
        if (s == NULL)
            break;

        len = strlen(s);
        if (buffer_output(r, s, len) != APR_SUCCESS) {
            return -1;
        }

        written += len;
    }
    va_end(va);

    return written;
}

int ap_rflush(request_rec *r)
{
    conn_rec *c = r->connection;
    apr_bucket_brigade *bb;
    apr_bucket *b;

    bb = apr_brigade_create(r->pool, c->bucket_alloc);
    b = apr_bucket_flush_create(c->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bb, b);
    if (ap_pass_brigade(r->output_filters, bb) != APR_SUCCESS)
        return -1;

    return 0;
}

/*****************************************************************************/
/* parse_uri: break apart the uri
 * Side Effects:
 * - sets r->args to rest after '?' (or NULL if no '?')
 * - sets r->uri to request uri (without r->args part)
 * - sets r->hostname (if not set already) from request (scheme://host:port)
 */
void ap_parse_uri(request_rec * r, const char *uri)
{
    int status = HTTP_OK;

    r->unparsed_uri = apr_pstrdup(r->pool, uri);

    if (r->method_number == M_CONNECT) {
		status = apr_uri_parse_hostinfo(r->pool, uri, &r->parsed_uri);
    }
    else {
		/* Simple syntax Errors in URLs are trapped by
		 * parse_uri_components().
		 */
		status = apr_uri_parse(r->pool, uri, &r->parsed_uri);
    }

    if (status == APR_SUCCESS) {
		/* if it has a scheme we may need to do absoluteURI vhost stuff */
		if (r->parsed_uri.scheme
			&& !strcasecmp(r->parsed_uri.scheme, sb_run_http_method(r))) {
			r->hostname = r->parsed_uri.hostname;
		}
		else if (r->method_number == M_CONNECT) {
			r->hostname = r->parsed_uri.hostname;
		}

		r->args = r->parsed_uri.query;
		r->uri = r->parsed_uri.path ? r->parsed_uri.path
			: apr_pstrdup(r->pool, "/");

#if defined(OS2) || defined(WIN32)
		/* Handle path translations for OS/2 and plug security hole.
		 * This will prevent "http://www.wherever.com/..\..\/" from
		 * returning a directory for the root drive.
		 */
		{
			char *x;

			for (x = r->uri; (x = strchr(x, '\\')) != NULL;)
			*x = '/';
		}
#endif				/* OS2 || WIN32 */

		debug("r->hostname = '%s', r->uri = '%s'", r->hostname, r->uri);
    }
    else {
		r->args = NULL;
		r->hostname = NULL;
		r->status = HTTP_BAD_REQUEST;	/* set error status */
		r->uri = apr_pstrdup(r->pool, uri);
		debug("r->uri = '%s', r->status = HTTP_BAD_REQUEST(%d)",
				r->uri, r->status);
    }
}


static int read_request_line(request_rec * r)
{
    const char *ll;
    const char *uri;
    const char *pro;
    int major = 1, minor = 0;	/* Assume HTTP/1.0 if non-"HTTP" protocol */
    apr_size_t len;

	debug("started");
    /* Read past empty lines until we get a real request line,
     * a read error, the connection closes (EOF), or we timeout.
     *
     * We skip empty lines because browsers have to tack a CRLF on to the end
     * of POSTs to support old CERN webservers.  But note that we may not
     * have flushed any previous response completely to the client yet.
     * We delay the flush as long as possible so that we can improve
     * performance for clients that are pipelining requests.  If a request
     * is pipelined then we won't block during the (implicit) read() below.
     * If the requests aren't pipelined, then the client is still waiting
     * for the final buffer flush from us, and we will block in the implicit
     * read().  B_SAFEREAD ensures that the BUFF layer flushes if it will
     * have to block during a read.
     */

    do {
		apr_status_t rv;

		/* insure sb_rgetline allocates memory each time thru the loop
		 * if there are empty lines
		 */
		r->the_request = NULL;
		rv = ap_rgetline(&(r->the_request), DEFAULT_LIMIT_REQUEST_LINE + 2,
				 &len, r, 0);

		if (rv != APR_SUCCESS) {
			r->request_time = apr_time_now();
			debug("ap_rgetline() returned rv[%d]", rv);
			return FAIL;
		}
    } while (len <= 0);

    /* we've probably got something to do, ignore graceful restart requests */

    r->request_time = apr_time_now();
    ll = r->the_request;
    r->method = ap_getword_white(r->pool, &ll);

#if 0
/* XXX If we want to keep track of the Method, the protocol module should do
 * it.  That support isn't in the scoreboard yet.  Hopefully next week
 * sometime.   rbb */
    ap_update_connection_status(AP_CHILD_THREAD_FROM_ID(conn->id),
				"Method", r->method);
#endif

    uri = ap_getword_white(r->pool, &ll);

    /* Provide quick information about the request method as soon as known */

    r->method_number = ap_method_number_of(r->method);
    if (r->method_number == M_GET && r->method[0] == 'H') {
		r->header_only = 1;
    }

    ap_parse_uri(r, uri);

    /* sb_getline returns (size of max buffer - 1) if it fills up the
     * buffer before finding the end-of-line.  This is only going to
     * happen if it exceeds the configured limit for a request-line.
     * The cast is safe, limit_req_line cannot be negative
     */
    if (len > (apr_size_t) r->server->limit_req_line) {
		r->status = HTTP_REQUEST_URI_TOO_LARGE;
		r->proto_num = HTTP_VERSION(1, 0);
		r->protocol = apr_pstrdup(r->pool, "HTTP/1.0");
		return FAIL;
    }

    if (ll[0]) {
		r->assbackwards = 0;
		pro = ll;
		len = strlen(ll);
    }
    else {
		r->assbackwards = 1;
		pro = "HTTP/0.9";
		len = 8;
    }
    r->protocol = apr_pstrmemdup(r->pool, pro, len);

    /* XXX ap_update_connection_status(conn->id, "Protocol", r->protocol); */

    /* Avoid sscanf in the common case */
    if (len == 8
		&& pro[0] == 'H' && pro[1] == 'T' && pro[2] == 'T' && pro[3] == 'P'
		&& pro[4] == '/' && apr_isdigit(pro[5]) && pro[6] == '.'
		&& apr_isdigit(pro[7])) {
		r->proto_num = HTTP_VERSION(pro[5] - '0', pro[7] - '0');
    }
    else if (2 == sscanf(r->protocol, "HTTP/%u.%u", &major, &minor)
			 && minor < HTTP_VERSION(1, 0))	/* don't allow HTTP/0.1000 */
		r->proto_num = HTTP_VERSION(major, minor);
    else
		r->proto_num = HTTP_VERSION(1, 0);

	debug("r->proto_num = %d", r->proto_num);
	debug("ended");
    return SUCCESS;
}



void get_mime_headers(request_rec *r)
{
    char* field;
    char *value;
    apr_size_t len;
    int fields_read = 0;
    apr_table_t *tmp_headers;

	debug("started");
    /* We'll use apr_table_overlap later to merge these into r->headers_in. */
    tmp_headers = apr_table_make(r->pool, 50);

    /*
     * Read header lines until we get the empty separator line, a read error,
     * the connection closes (EOF), reach the server limit, or we timeout.
     */
    while(1) {
        apr_status_t rv;

        field = NULL;
        rv = ap_rgetline(&field, DEFAULT_LIMIT_REQUEST_FIELDSIZE + 2,
                         &len, r, 1);

        /* ap_rgetline returns APR_ENOSPC if it fills up the buffer before
         * finding the end-of-line.  This is only going to happen if it
         * exceeds the configured limit for a field size.
         * The cast is safe, limit_req_fieldsize cannot be negative
         */
        if (rv == APR_ENOSPC
            || (rv == APR_SUCCESS 
                && len > (apr_size_t)r->server->limit_req_fieldsize)) {
            r->status = HTTP_BAD_REQUEST;
            apr_table_setn(r->notes, "error-notes",
                           apr_pstrcat(r->pool,
                                       "Size of a request header field "
                                       "exceeds server limit.<br />\n"
                                       "<pre>\n",
                                       sb_escape_html(r->pool, field),
                                       "</pre>\n", NULL));
            return;
        }

        if (rv != APR_SUCCESS) {
            r->status = HTTP_BAD_REQUEST;
            return;
        }

        /* Found a blank line, stop. */
        if (len == 0) {
            break;
        }

        if (r->server->limit_req_fields
            && (++fields_read > r->server->limit_req_fields)) {
            r->status = HTTP_BAD_REQUEST;
            apr_table_setn(r->notes, "error-notes",
                           "The number of request header fields exceeds "
                           "this server's limit.");
            return;
        }

        if (!(value = strchr(field, ':'))) {    /* Find the colon separator */
            r->status = HTTP_BAD_REQUEST;       /* or abort the bad request */
            apr_table_setn(r->notes, "error-notes",
                           apr_pstrcat(r->pool,
                                       "Request header field is missing "
                                       "colon separator.<br />\n"
                                       "<pre>\n",
                                       sb_escape_html(r->pool, field),
                                       "</pre>\n", NULL));
            return;
        }

        *value = '\0';
        ++value;
        while (*value == ' ' || *value == '\t') {
            ++value;            /* Skip to start of value   */
        }

		debug("header - `%s': `%s'", field, value);
        apr_table_addn(tmp_headers, field, value);
    }

    apr_table_overlap(r->headers_in, tmp_headers, APR_OVERLAP_TABLES_MERGE);
	debug("ended");
}

request_rec *read_request(conn_rec * c)
{
    request_rec *r;
    apr_pool_t *p;
    const char *expect;
    int access_status;

    apr_pool_create(&p, c->pool);
    r = apr_palloc(p, sizeof(request_rec));

    memset(r, '\0', sizeof *r); /* it is assumed that r is zeroed by code below */

    r->pool            = p;
    r->connection      = c;
    r->server          = c->base_server;

    r->user            = NULL;
    r->auth_type       = NULL;

    r->allowed_methods = make_method_list(p, 2);

    r->headers_in      = apr_table_make(r->pool, 25);
    r->subprocess_env  = apr_table_make(r->pool, 25);
    r->headers_out     = apr_table_make(r->pool, 12);
    r->err_headers_out = apr_table_make(r->pool, 5);

    r->notes           = apr_table_make(r->pool, 5);

    r->request_config  = ap_create_request_config(r->pool);
    /* Must be set before we run create request hook */

    r->proto_output_filters = c->output_filters;
    r->output_filters  = r->proto_output_filters;
    r->proto_input_filters = c->input_filters;
    r->input_filters   = r->proto_input_filters;

    sb_run_create_request(r);
    r->per_dir_config  = r->server->lookup_defaults;

    r->sent_bodyct     = 0;		/* bytect isn't for body */
    r->read_length     = 0;
    r->read_body       = REQUEST_NO_BODY;
    r->status          = HTTP_REQUEST_TIME_OUT;	/* until we get a request */
    r->the_request     = NULL;

    /* get the request */
    if (read_request_line(r) != SUCCESS) {
		debug("read_request_line(r) returned error, r->status = %d", r->status);
		if (r->status == HTTP_REQUEST_URI_TOO_LARGE) {
				ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
							  "request failed: URI too long");
			ap_send_error_response(r, 0);
			//sb_run_log_transaction(r);
			return r;
		}

		return NULL;
    }

    if (!r->assbackwards) {
		get_mime_headers(r);
		if (r->status != HTTP_REQUEST_TIME_OUT) {
				ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
							  "request failed: error reading the headers");
			ap_send_error_response(r, 0);
			//sb_run_log_transaction(r);
			return r;
		}
    }
    else {
		if (r->header_only) {
			/*
			 * Client asked for headers only with HTTP/0.9, which doesn't send
			 * headers! Have to dink things just to make sure the error message
			 * comes through...
			 */
			ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
						  "client sent invalid HTTP/0.9 request: HEAD %s",
						  r->uri);
			r->header_only = 0;
			r->status = HTTP_BAD_REQUEST;
			ap_send_error_response(r, 0);
			//sb_run_log_transaction(r);
			return r;
		}
    }

	(r->hostname || (r->hostname = apr_table_get(r->headers_in, "Host")));

	debug("apr_table_get('Host') = '%s'", apr_table_get(r->headers_in, "Host"));
	debug("r->hostname = '%s'", r->hostname);

    r->status = HTTP_OK;	/* until further notice */
    if ((!r->hostname && (r->proto_num >= HTTP_VERSION(1, 1)))
		|| ((r->proto_num == HTTP_VERSION(1, 1))
	    	&& !apr_table_get(r->headers_in, "Host"))) {
		/*
		 * Client sent us an HTTP/1.1 or later request without telling us the
		 * hostname, either with a full URL or a Host: header. We therefore
		 * need to (as per the 1.1 spec) send an error.  As a special case,
		 * HTTP/1.1 mentions twice (S9, S14.23) that a request MUST contain
		 * a Host: header, and the server MUST respond with 400 if it doesn't.
		 */
		r->status = HTTP_BAD_REQUEST;
			ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
						  "client sent HTTP/1.1 request without hostname "
						  "(see RFC2616 section 14.23): %s", r->uri);
    }

    if (r->status != HTTP_OK) {
		ap_send_error_response(r, 0);
		//sb_run_log_transaction(r);
		return r;
    }

    if (((expect = apr_table_get(r->headers_in, "Expect")) != NULL)
		&& (expect[0] != '\0')) {
		/*
		 * The Expect header field was added to HTTP/1.1 after RFC 2068
		 * as a means to signal when a 100 response is desired and,
		 * unfortunately, to signal a poor man's mandatory extension that
		 * the server must understand or return 417 Expectation Failed.
		 */
		if (strcasecmp(expect, "100-continue") == 0) {
			r->expecting_100 = 1;
		}
		else {
			r->status = HTTP_EXPECTATION_FAILED;
				ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
							  "client sent an unrecognized expectation value of "
							  "Expect: %s", expect);
			ap_send_error_response(r, 0);
			//sb_run_log_transaction(r);
			return r;
		}
    }
	ap_add_input_filter_handle(ap_http_input_filter_handle,
			NULL, r, r->connection);

    // FIXME
//    if (((access_status = sb_run_post_read_request(r))) != SUCCESS) {
//		ap_die(access_status, r);
		//sb_run_log_transaction(r);
//		return NULL;
//    }

	debug("ended");
    return r;
}

