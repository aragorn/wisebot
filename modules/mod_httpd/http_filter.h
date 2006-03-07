/* $Id$ */
#ifndef __HTTP_FILTER_H__
#define __HTTP_FILTER_H__

#include "util_filter.h"

/* Handles for http core filters */
extern ap_filter_rec_t *ap_http_input_filter_handle;
extern ap_filter_rec_t *ap_http_header_filter_handle;
extern ap_filter_rec_t *ap_chunk_filter_handle;
extern ap_filter_rec_t *ap_byterange_filter_handle;

/*
 * These (input) filters are internal to the mod_core operation.
 */
apr_status_t ap_http_filter(ap_filter_t *f, apr_bucket_brigade *b,
                            ap_input_mode_t mode, apr_read_type_e block,
                            apr_off_t readbytes);
apr_status_t ap_http_header_filter(ap_filter_t *f, apr_bucket_brigade *b);
apr_status_t ap_byterange_filter(ap_filter_t *f, apr_bucket_brigade *bb);
apr_status_t ap_content_length_filter(ap_filter_t *f, apr_bucket_brigade *bb);

#endif

