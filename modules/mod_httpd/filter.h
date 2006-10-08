/* $Id$ */
#ifndef FILTER_H
#define FILTER_H

#include "apr_buckets.h" /* apr_bucket_brigade */
#include "util_filter.h" /* ap_filter_rec_t */

typedef struct {
    apr_bucket_brigade *bb;
} old_write_filter_ctx;

/* Handles for core filters */
extern ap_filter_rec_t *ap_core_input_filter_handle;
extern ap_filter_rec_t *ap_core_output_filter_handle;
extern ap_filter_rec_t *ap_subreq_core_filter_handle;
extern ap_filter_rec_t *ap_content_length_filter_handle;
extern ap_filter_rec_t *ap_net_time_filter_handle;

apr_status_t ap_old_write_filter(ap_filter_t *f, apr_bucket_brigade *b);
void register_core_filters(void);

#endif

