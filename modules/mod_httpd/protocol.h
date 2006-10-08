/* $Id$ */
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include "mod_httpd.h"
#include "util_filter.h"

/**
 * @package HTTP protocol handling
 */

/* This is an optimization.  We keep a record of the filter_rec that
 * stores the old_write filter, so that we can avoid strcmp's later.
 */
extern ap_filter_rec_t *ap_old_write_func;

/*
 * Prototypes for routines which either talk directly back to the user,
 * or control the ones that eventually do.
 */

/**
 * Read a request and fill in the fields.
 * @param c The current connection
 * @return The new request_rec
 */ 
request_rec *read_request(conn_rec *c);

/**
 * Read the mime-encoded headers.
 * @param r The current request
 */
void get_mime_headers(request_rec *r);

/* Finish up stuff after a request */

/**
 * Called at completion of sending the response.  It sends the terminating
 * protocol information.
 * @param r The current request
 * @deffunc void ap_finalize_request_protocol(request_rec *r)
 */
AP_DECLARE(void) ap_finalize_request_protocol(request_rec *r);

void ap_finalize_sub_req_protocol(request_rec *sub_r);

/*****************************************************************************/

/**
 * Precompile metadata structures used by ap_make_content_type()
 * @param r The pool to use for allocations
 * @deffunc void ap_setup_make_content_type(apr_pool_t *pool)
 */
AP_DECLARE(void) ap_setup_make_content_type(apr_pool_t *pool);

/**
 * Build the content-type that should be sent to the client from the
 * content-type specified.  The following rules are followed:
 *    - if type is NULL, type is set to ap_default_type(r)
 *    - if charset adding is disabled, stop processing and return type.
 *    - then, if there are no parameters on type, add the default charset
 *    - return type
 * @param r The current request
 * @return The content-type
 * @deffunc const char *ap_make_content_type(request_rec *r, const char *type);
 */ 
AP_DECLARE(const char *) ap_make_content_type(request_rec *r,
                                              const char *type);

/**
 * Set the content type for this request (r->content_type). 
 * Note:
 * This function must be called to set r->content_type in order 
 * for the AddOutputFilterByType directive to work correctly.
 * @param r The current request
 * @param length The new content type
 * @deffunc void ap_set_content_type(request_rec *r, const char* ct)
 */
AP_DECLARE(void) ap_set_content_type(request_rec *r, const char *ct);

/* Set last modified header line from the lastmod date of the associated file.
 * Also, set content length.
 *
 * May return an error status, typically HTTP_NOT_MODIFIED (that when the
 * permit_cache argument is set to one).
 */
AP_DECLARE(void) ap_set_last_modified(request_rec *r);

/**
 * Set the content length for this request
 * @param r The current request
 * @param length The new content length
 * @deffunc void ap_set_content_length(request_rec *r, apr_off_t length)
 */
AP_DECLARE(void) ap_set_content_length(request_rec *r, apr_off_t length);

//int ap_setup_client_block(request_rec *r, int read_policy);


int ap_rputc(int c, request_rec *r);
int ap_rputs(const char *str, request_rec *r);
int ap_rwrite(const void *buf, int nbyte, request_rec *r);
int ap_vrprintf(request_rec *r, const char *fmt, va_list va);
int ap_rprintf(request_rec *r, const char *fmt, ...);
int ap_rvputs(request_rec *r, ...);
int ap_rflush(request_rec *r);


/**
 * Get the next line of input for the request
 *
 * Note: on ASCII boxes, ap_rgetline is a macro which simply calls 
 *       ap_rgetline_core to get the line of input.
 * 
 *       on EBCDIC boxes, ap_rgetline is a wrapper function which
 *       translates ASCII protocol lines to the local EBCDIC code page
 *       after getting the line of input.
 *       
 * @param s Pointer to the pointer to the buffer into which the line
 *          should be read; if *s==NULL, a buffer of the necessary size
 *          to hold the data will be allocated from the request pool
 * @param n The size of the buffer
 * @param read The length of the line.
 * @param r The request
 * @param fold Whether to merge continuation lines
 * @return APR_SUCCESS, if successful
 *         APR_ENOSPC, if the line is too big to fit in the buffer
 *         Other errors where appropriate
 */
AP_DECLARE(apr_status_t) ap_rgetline(char **s, apr_size_t n, 
                                     apr_size_t *read,
                                     request_rec *r, int fold);




  /*
   * post_read_request --- run right after read_request or internal_redirect,
   *                  and not run during any subrequests.
   */
/**
 * This hook allows modules to affect the request immediately after the request
 * has been read, and before any other phases have been processes.  This allows
 * modules to make decisions based upon the input header fields
 * @param r The current request
 * @return OK or DECLINED
 * @deffunc ap_run_post_read_request(request_rec *r)
 */
SB_DECLARE_HOOK(int,post_read_request,(request_rec *r))

/**
 * This hook allows modules to perform any module-specific logging activities
 * over and above the normal server things.
 * @param r The current request
 * @return OK, DECLINED, or HTTP_...
 * @deffunc int ap_run_log_transaction(request_rec *r)
 */
SB_DECLARE_HOOK(int,log_transaction,(request_rec *r))

/**
 * This hook allows modules to retrieve the http method from a request.  This
 * allows Apache modules to easily extend the methods that Apache understands
 * @param r The current request
 * @return The http method from the request
 * @deffunc const char *ap_run_http_method(const request_rec *r)
 */
SB_DECLARE_HOOK(const char *,http_method,(const request_rec *r))

/**
 * Return the default port from the current request
 * @param r The current request
 * @return The current port
 * @deffunc apr_port_t ap_run_default_port(const request_rec *r)
 */
SB_DECLARE_HOOK(apr_port_t,default_port,(const request_rec *r))

#endif
