/* $Id$ */
#ifndef REQUEST_H
#define REQUEST_H

#include "util_filter.h" /* ap_filter_t */

AP_DECLARE(int) ap_directory_walk(request_rec *r);
AP_DECLARE(int) ap_location_walk(request_rec *r);
AP_DECLARE(int) ap_file_walk(request_rec *r);

/*****************************************************************************/
/* sub request */

/**
 * An output filter to strip EOS buckets from sub-requests.  This always
 * has to be inserted at the end of a sub-requests filter stack.
 * @param f The current filter
 * @param bb The brigade to filter
 * @deffunc apr_status_t ap_sub_req_output_filter(ap_filter_t *f, apr_bucket_brigade *bb)
 */
AP_CORE_DECLARE_NONSTD(apr_status_t) ap_sub_req_output_filter(ap_filter_t *f,
                                                        apr_bucket_brigade *bb);

/**
 * Can be used within any handler to determine if any authentication
 * is required for the current request
 * @param r The current request
 * @return 1 if authentication is required, 0 otherwise
 * @deffunc int ap_some_auth_required(request_rec *r)
 */
AP_DECLARE(int) ap_some_auth_required(request_rec *r);


/**
 * Create a sub request for the given URI using a specific method.  This
 * sub request can be inspected to find information about the requested URI
 * @param method The method to use in the new sub request
 * @param new_file The URI to lookup
 * @param r The current request
 * @param next_filter The first filter the sub_request should use.  If this is
 *                    NULL, it defaults to the first filter for the main request
 * @return The new request record
 * @deffunc request_rec * ap_sub_req_method_uri(const char *method, const char *new_file, const request_rec *r)
 */
AP_DECLARE(request_rec *) ap_sub_req_method_uri(const char *method,
                                                const char *new_file,
                                                const request_rec *r,
                                                ap_filter_t *next_filter);

/**
 * Create a sub request from the given URI.  This sub request can be
 * inspected to find information about the requested URI
 * @param new_file The URI to lookup
 * @param r The current request
 * @param next_filter The first filter the sub_request should use.  If this is
 *                    NULL, it defaults to the first filter for the main request
 * @return The new request record
 * @deffunc request_rec * ap_sub_req_lookup_uri(const char *new_file, const request_rec *r)
 */
AP_DECLARE(request_rec *) ap_sub_req_lookup_uri(const char *new_file,
                                                const request_rec *r,
                                                ap_filter_t *next_filter);

/**
 * Create a sub request for the given apr_dir_read result.  This sub request 
 * can be inspected to find information about the requested file
 * @param finfo The apr_dir_read result to lookup
 * @param r The current request
 * @param subtype What type of subrequest to perform, one of;
 * <PRE>
 *      AP_SUBREQ_NO_ARGS     ignore r->args and r->path_info
 *      AP_SUBREQ_MERGE_ARGS  merge r->args and r->path_info
 * </PRE>
 * @param next_filter The first filter the sub_request should use.  If this is
 *                    NULL, it defaults to the first filter for the main request
 * @return The new request record
 * @deffunc request_rec * ap_sub_req_lookup_dirent(apr_finfo_t *finfo, int subtype, const request_rec *r)
 * @tip The apr_dir_read flags value APR_FINFO_MIN|APR_FINFO_NAME flag is the 
 * minimum recommended query if the results will be passed to apr_dir_read.
 * The file info passed must include the name, and must have the same relative
 * directory as the current request.
 */
AP_DECLARE(request_rec *) ap_sub_req_lookup_dirent(const apr_finfo_t *finfo,
                                                   const request_rec *r,
                                                   int subtype,
                                                   ap_filter_t *next_filter);

/**
 * Create a sub request for the given file.  This sub request can be
 * inspected to find information about the requested file
 * @param new_file The URI to lookup
 * @param r The current request
 * @param next_filter The first filter the sub_request should use.  If this is
 *                    NULL, it defaults to the first filter for the main request
 * @return The new request record
 * @deffunc request_rec * ap_sub_req_lookup_file(const char *new_file, const request_rec *r)
 */
AP_DECLARE(request_rec *) ap_sub_req_lookup_file(const char *new_file,
                                              const request_rec *r,
                                              ap_filter_t *next_filter);

/**
 * Run the handler for the sub request
 * @param r The sub request to run
 * @return The return code for the sub request
 * @deffunc int ap_run_sub_req(request_rec *r)
 */
//AP_DECLARE(int) ap_run_sub_req(request_rec *r);

/**
 * Free the memory associated with a sub request
 * @param r The sub request to finish
 * @deffunc void ap_destroy_sub_req(request_rec *r)
 */
AP_DECLARE(void) ap_destroy_sub_req(request_rec *r);

/**
 * Function to set the r->mtime field to the specified value if it's later
 * than what's already there.
 * @param r The current request
 * @param dependency_time Time to set the mtime to
 * @deffunc void ap_update_mtime(request_rec *r, apr_time_t dependency_mtime)
 */
AP_DECLARE(void) ap_update_mtime(request_rec *r, apr_time_t dependency_mtime);

/**
 * Determine if the current request is the main request or a sub requests
 * @param r The current request
 * @retrn 1 if this is a main request, 0 otherwise
 * @deffunc int ap_is_initial_req(request_rec *r)
 */
AP_DECLARE(int) ap_is_initial_req(request_rec *r);


/* FIXME to be moved to protocol.h */
void ap_set_content_type(request_rec * r, const char *ct);
int ap_setup_client_block(request_rec *r, int read_policy);

// http_protocol.h
int ap_discard_request_body(request_rec *r);

// protocol.h
int ap_rputc(int c, request_rec *r);
int ap_rputs(const char *str, request_rec *r);
int ap_rwrite(const void *buf, int nbyte, request_rec *r);
int ap_vrprintf(request_rec *r, const char *fmt, va_list va);
int ap_rprintf(request_rec *r, const char *fmt, ...);
int ap_rvputs(request_rec *r, ...);
int ap_rflush(request_rec *r);
void ap_set_last_modified(request_rec *r);


/*****************************************************************************/
/* Hooks */

/**
 * This hook allow modules an opportunity to translate the URI into an
 * actual filename.  If no modules do anything special, the server's default
 * rules will be followed.
 * @param r The current request
 * @return OK, DECLINED, or HTTP_...
 * @ingroup hooks
 */
SB_DECLARE_HOOK(int,translate_name,(request_rec *r))

/**
 * This hook allow modules to set the per_dir_config based on their own
 * context (such as <Proxy > sections) and responds to contextless requests 
 * such as TRACE that need no security or filesystem mapping.
 * based on the filesystem.
 * @param r The current request
 * @return DONE (or HTTP_) if this contextless request was just fulfilled 
 * (such as TRACE), OK if this is not a file, and DECLINED if this is a file.
 * The core map_to_storage (HOOK_RUN_LAST) will directory_walk and file_walk
 * the r->filename.
 * 
 * @ingroup hooks
 */
SB_DECLARE_HOOK(int,map_to_storage,(request_rec *r))

/**
 * This hook allows modules to check the authentication information sent with
 * the request.
 * @param r The current request
 * @return OK, DECLINED, or HTTP_...
 * @ingroup hooks
 */
SB_DECLARE_HOOK(int,check_user_id,(request_rec *r))

/**
 * Allows modules to perform module-specific fixing of header fields.  This
 * is invoked just before any content-handler
 * @param r The current request
 * @return OK, DECLINED, or HTTP_...
 * @ingroup hooks
 */
SB_DECLARE_HOOK(int,fixups,(request_rec *r))
 
/**
 * This routine is called to determine and/or set the various document type
 * information bits, like Content-type (via r->content_type), language, et
 * cetera.
 * @param r the current request
 * @return OK, DECLINED, or HTTP_...
 * @ingroup hooks
 */
SB_DECLARE_HOOK(int,type_checker,(request_rec *r))

/**
 * This routine is called to check for any module-specific restrictions placed
 * upon the requested resource.
 * @param r the current request
 * @return OK, DECLINED, or HTTP_...
 * @ingroup hooks
 */
SB_DECLARE_HOOK(int,access_checker,(request_rec *r))

/**
 * This routine is called to check to see if the resource being requested
 * requires authorisation.
 * @param r the current request
 * @return OK, DECLINED, or HTTP_...
 * @ingroup hooks
 */
SB_DECLARE_HOOK(int,auth_checker,(request_rec *r))

/**
 * This hook allows modules to insert filters for the current request
 * @param r the current request
 * @ingroup hooks
 */
SB_DECLARE_HOOK(void,insert_filter,(request_rec *r))

/**
 * Gives modules a chance to create their request_config entry when the
 * request is created.
 * @param r The current request
 * @ingroup hooks
 */
SB_DECLARE_HOOK(int,create_request,(request_rec *r))

#endif
