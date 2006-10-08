/* $Id$ */
/*
 * filter.c
 */
#include "common_core.h"
#include "filter.h"
#include "http_core.h"
#include "http_config.h"
#include "util_filter.h"

#define AP_MIN_SENDFILE_BYTES		(256)

/* Handles for core filters */
ap_filter_rec_t *ap_core_input_filter_handle;
ap_filter_rec_t *ap_core_output_filter_handle;
ap_filter_rec_t *ap_subreq_core_filter_handle;
ap_filter_rec_t *ap_content_length_filter_handle;
ap_filter_rec_t *ap_net_time_filter_handle;

/*****************************************************************************/
/* FILTER : core_input */

/**
 * Remove all zero length buckets from the brigade.
 */
#define BRIGADE_NORMALIZE(b) \
do { \
    apr_bucket *e = APR_BRIGADE_FIRST(b); \
    do {  \
        if (e->length == 0 && !APR_BUCKET_IS_METADATA(e)) { \
            apr_bucket *d; \
            d = APR_BUCKET_NEXT(e); \
            apr_bucket_delete(e); \
            e = d; \
        } \
        e = APR_BUCKET_NEXT(e); \
    } while (!APR_BRIGADE_EMPTY(b) && (e != APR_BRIGADE_SENTINEL(b))); \
} while (0)

static int core_input_filter(ap_filter_t *f, apr_bucket_brigade *b,
                             ap_input_mode_t mode, apr_read_type_e block,
                             apr_off_t readbytes)
{
    apr_bucket *e;
    apr_status_t rv;
    core_net_rec *net = f->ctx;
    core_ctx_t *ctx = net->in_ctx;
    const char *str;
    apr_size_t len;

//    debug("1");

    if (mode == AP_MODE_INIT) {
//        debug("2");
        /*
         * this mode is for filters that might need to 'initialize'
         * a connection before reading request data from a client.
         * NNTP over SSL for example needs to handshake before the
         * server sends the welcome message.
         * such filters would have changed the mode before this point
         * is reached.  however, protocol modules such as NNTP should
         * not need to know anything about SSL.  given the example, if
         * SSL is not in the filter chain, AP_MODE_INIT is a noop.
         */
        return APR_SUCCESS;
    }

//    debug("3");
    if (!ctx)
    {
//        debug("4");
        ctx = apr_pcalloc(f->c->pool, sizeof(*ctx));
        ctx->b = apr_brigade_create(f->c->pool, f->c->bucket_alloc);

        /* seed the brigade with the client socket. */
        e = apr_bucket_socket_create(net->client_socket, f->c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(ctx->b, e);
        net->in_ctx = ctx;
//        debug("5");
    }
    else if (APR_BRIGADE_EMPTY(ctx->b)) {
//        debug("6");
        return APR_EOF;
    }

    /* ### This is bad. */
//    debug("7");
    BRIGADE_NORMALIZE(ctx->b);

    /* check for empty brigade again *AFTER* BRIGADE_NORMALIZE()
     * If we have lost our socket bucket (see above), we are EOF.
     *
     * Ideally, this should be returning SUCCESS with EOS bucket, but
     * some higher-up APIs (spec. read_request_line via ap_rgetline)
     * want an error code. */
//    debug("8");
    if (APR_BRIGADE_EMPTY(ctx->b)) {
//        debug("9");
        return APR_EOF;
    }

    /* ### AP_MODE_PEEK is a horrific name for this mode because we also
     * eat any CRLFs that we see.  That's not the obvious intention of
     * this mode.  Determine whether anyone actually uses this or not. */
//    debug("10");
    if (mode == AP_MODE_EATCRLF) {
        apr_bucket *e;
        const char *c;

//        debug("11");
        /* The purpose of this loop is to ignore any CRLF (or LF) at the end
         * of a request.  Many browsers send extra lines at the end of POST
         * requests.  We use the PEEK method to determine if there is more
         * data on the socket, so that we know if we should delay sending the
         * end of one request until we have served the second request in a
         * pipelined situation.  We don't want to actually delay sending a
         * response if the server finds a CRLF (or LF), becuause that doesn't
         * mean that there is another request, just a blank line.
         */
        while (1) {
//            debug("12");
            if (APR_BRIGADE_EMPTY(ctx->b))
                return APR_EOF;

            e = APR_BRIGADE_FIRST(ctx->b);

            rv = apr_bucket_read(e, &str, &len, APR_NONBLOCK_READ);

            if (rv != APR_SUCCESS)
                return rv;

            c = str;
            while (c < str + len) {
                if (*c == APR_ASCII_LF)
                    c++;
                else if (*c == APR_ASCII_CR && *(c + 1) == APR_ASCII_LF)
                    c += 2;
                else
                    return APR_SUCCESS;
            }

            /* If we reach here, we were a bucket just full of CRLFs, so
             * just toss the bucket. */
            /* FIXME: Is this the right thing to do in the core? */
            apr_bucket_delete(e);
//            debug("13");
        }
    }

//    debug("14");
    /* If mode is EXHAUSTIVE, we want to just read everything until the end
     * of the brigade, which in this case means the end of the socket.
     * To do this, we attach the brigade that has currently been setaside to
     * the brigade that was passed down, and send that brigade back.
     *
     * NOTE:  This is VERY dangerous to use, and should only be done with
     * extreme caution.  However, the Perchild MPM needs this feature
     * if it is ever going to work correctly again.  With this, the Perchild
     * MPM can easily request the socket and all data that has been read,
     * which means that it can pass it to the correct child process.
     */
    if (mode == AP_MODE_EXHAUSTIVE) {
        apr_bucket *e;
//        debug("15");

        /* Tack on any buckets that were set aside. */
        APR_BRIGADE_CONCAT(b, ctx->b);

        /* Since we've just added all potential buckets (which will most
         * likely simply be the socket bucket) we know this is the end,
         * so tack on an EOS too. */
        /* We have read until the brigade was empty, so we know that we
         * must be EOS. */
        e = apr_bucket_eos_create(f->c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(b, e);
//        debug("16");
        return APR_SUCCESS;
    }

    /* read up to the amount they specified. */
    if (mode == AP_MODE_READBYTES || mode == AP_MODE_SPECULATIVE) {
        apr_off_t total;
        apr_bucket *e;
        apr_bucket_brigade *newbb;
//        debug("17");

        SB_DEBUG_ASSERT(readbytes > 0);

//        debug("17a");
        e = APR_BRIGADE_FIRST(ctx->b);
//        debug("17b");
        rv = apr_bucket_read(e, &str, &len, block);

//        debug("18");
        if (APR_STATUS_IS_EAGAIN(rv)) {
//            debug("19");
            return APR_SUCCESS;
        }
        else if (rv != APR_SUCCESS) {
//            debug("20:%d", rv);
            return rv;
        }
        else if (block == APR_BLOCK_READ && len == 0) {
            /* We wanted to read some bytes in blocking mode.  We read
             * 0 bytes.  Hence, we now assume we are EOS.
             *
             * When we are in normal mode, return an EOS bucket to the
             * caller.
             * When we are in speculative mode, leave ctx->b empty, so
             * that the next call returns an EOS bucket.
             */
//            debug("21");
            apr_bucket_delete(e);

            //debug("22");
            if (mode == AP_MODE_READBYTES) {
                //debug("23");
                e = apr_bucket_eos_create(f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(b, e);
            }
            //debug("24");
            return APR_SUCCESS;
        }

        /* We can only return at most what we read. */
        //debug("25");
        if (len < readbytes) {
            //debug("26");
            readbytes = len;
        }

        //debug("27");
        apr_brigade_partition(ctx->b, readbytes, &e);

        //debug("28");
        /* Must do split before CONCAT */
        newbb = apr_brigade_split(ctx->b, e);

        //debug("29");
        if (mode == AP_MODE_READBYTES) {
            //debug("30");
            APR_BRIGADE_CONCAT(b, ctx->b);
            //debug("31");
        }
        else if (mode == AP_MODE_SPECULATIVE) {
            apr_bucket *copy_bucket;
            //debug("32");
            APR_BRIGADE_FOREACH(e, ctx->b) {
                //debug("33");
                rv = apr_bucket_copy(e, &copy_bucket);
                if (rv != APR_SUCCESS) {
                    //debug("34");
                    return rv;
                }
                //debug("35");
                APR_BRIGADE_INSERT_TAIL(b, copy_bucket);
            }
        }

        /* Take what was originally there and place it back on ctx->b */
        //debug("36");
        APR_BRIGADE_CONCAT(ctx->b, newbb);

        /* XXX: Why is this here? We never use 'total'! */
        //debug("38");
        apr_brigade_length(b, 1, &total);

        //debug("39");
        return APR_SUCCESS;
    }

    //debug("40");
    /* we are reading a single LF line, e.g. the HTTP headers */
    rv = apr_brigade_split_line(b, ctx->b, block, HUGE_STRING_LEN);

    //debug("41");
    /* We should treat EAGAIN here the same as we do for EOF (brigade is
     * empty).  We do this by returning whatever we have read.  This may
     * or may not be bogus, but is consistent (for now) with EOF logic.
     */
    if (APR_STATUS_IS_EAGAIN(rv)) {
        //debug("42");
        rv = APR_SUCCESS;
    }

    //debug("43");
    return rv;
}

/*****************************************************************************/
/* FILTER : core_output */
static apr_status_t writev_it_all(apr_socket_t *s,
                                  struct iovec *vec, int nvec,
                                  apr_size_t len, apr_size_t *nbytes)
{
    apr_size_t bytes_written = 0;
    apr_status_t rv;
    apr_size_t n = len;
    int i = 0;

    *nbytes = 0;

    /* XXX handle checking for non-blocking socket */
    while (bytes_written != len) {
        rv = apr_sendv(s, vec + i, nvec - i, &n);
        bytes_written += n;
        if (rv != APR_SUCCESS)
            return rv;

        *nbytes += n;

        /* If the write did not complete, adjust the iovecs and issue
         * apr_sendv again
         */
        if (bytes_written < len) {
            /* Skip over the vectors that have already been written */
            apr_size_t cnt = vec[i].iov_len;
            while (n >= cnt && i + 1 < nvec) {
                i++;
                cnt += vec[i].iov_len;
            }

            if (n < cnt) {
                /* Handle partial write of vec i */
                vec[i].iov_base = (char *) vec[i].iov_base +
                    (vec[i].iov_len - (cnt - n));
                vec[i].iov_len = cnt -n;
            }
        }

        n = len - bytes_written;
    }

    return APR_SUCCESS;
}

/* sendfile_it_all()
 *  send the entire file using sendfile()
 *  handle partial writes
 *  return only when all bytes have been sent or an error is encountered.
 */

#if APR_HAS_SENDFILE
static apr_status_t sendfile_it_all(core_net_rec *c,
                                    apr_file_t *fd,
                                    apr_hdtr_t *hdtr,
                                    apr_off_t   file_offset,
                                    apr_size_t  file_bytes_left,
                                    apr_size_t  total_bytes_left,
                                    apr_int32_t flags)
{
    apr_status_t rv;
    apr_int32_t timeout = 0;

    SB_DEBUG_ASSERT((apr_getsocketopt(c->client_socket, APR_SO_TIMEOUT,
                                      &timeout) == APR_SUCCESS)
                    && timeout > 0);  /* socket must be in timeout mode */

    do {
        apr_size_t tmplen = file_bytes_left;

        rv = apr_sendfile(c->client_socket, fd, hdtr, &file_offset, &tmplen,
                          flags);
        total_bytes_left -= tmplen;
        if (!total_bytes_left || rv != APR_SUCCESS) {
            return rv;        /* normal case & error exit */
        }

        SB_DEBUG_ASSERT(total_bytes_left > 0 && tmplen > 0);

        /* partial write, oooh noooo...
         * Skip over any header data which was written
         */
        while (tmplen && hdtr->numheaders) {
            if (tmplen >= hdtr->headers[0].iov_len) {
                tmplen -= hdtr->headers[0].iov_len;
                --hdtr->numheaders;
                ++hdtr->headers;
            }
            else {
                char *iov_base = (char *)hdtr->headers[0].iov_base;

                hdtr->headers[0].iov_len -= tmplen;
                iov_base += tmplen;
                hdtr->headers[0].iov_base = iov_base;
                tmplen = 0;
            }
        }

        /* Skip over any file data which was written */

        if (tmplen <= file_bytes_left) {
            file_offset += tmplen;
            file_bytes_left -= tmplen;
            continue;
        }

        tmplen -= file_bytes_left;
        file_bytes_left = 0;
        file_offset = 0;

        /* Skip over any trailer data which was written */

        while (tmplen && hdtr->numtrailers) {
            if (tmplen >= hdtr->trailers[0].iov_len) {
                tmplen -= hdtr->trailers[0].iov_len;
                --hdtr->numtrailers;
                ++hdtr->trailers;
            }
            else {
                char *iov_base = (char *)hdtr->trailers[0].iov_base;

                hdtr->trailers[0].iov_len -= tmplen;
                iov_base += tmplen;
                hdtr->trailers[0].iov_base = iov_base;
                tmplen = 0;
            }
        }
    } while (1);
}
#endif

/*
 * emulate_sendfile()
 * Sends the contents of file fd along with header/trailer bytes, if any,
 * to the network. emulate_sendfile will return only when all the bytes have been
 * sent (i.e., it handles partial writes) or on a network error condition.
 */
static apr_status_t emulate_sendfile(core_net_rec *c, apr_file_t *fd,
                                     apr_hdtr_t *hdtr, apr_off_t offset,
                                     apr_size_t length, apr_size_t *nbytes)
{
    apr_status_t rv = APR_SUCCESS;
    apr_int32_t togo;        /* Remaining number of bytes in the file to send */
    apr_size_t sendlen = 0;
    apr_size_t bytes_sent;
    apr_int32_t i;
    apr_off_t o;             /* Track the file offset for partial writes */
    char buffer[8192];

    *nbytes = 0;

    /* Send the headers
     * writev_it_all handles partial writes.
     * XXX: optimization... if headers are less than MIN_WRITE_SIZE, copy
     * them into buffer
     */
    if (hdtr && hdtr->numheaders > 0 ) {
        for (i = 0; i < hdtr->numheaders; i++) {
            sendlen += hdtr->headers[i].iov_len;
        }

        rv = writev_it_all(c->client_socket, hdtr->headers, hdtr->numheaders,
                           sendlen, &bytes_sent);
        if (rv == APR_SUCCESS)
            *nbytes += bytes_sent;     /* track total bytes sent */
    }

    /* Seek the file to 'offset' */
    if (offset != 0 && rv == APR_SUCCESS) {
        rv = apr_file_seek(fd, APR_SET, &offset);
    }

    /* Send the file, making sure to handle partial writes */
    togo = length;
    while (rv == APR_SUCCESS && togo) {
        sendlen = togo > sizeof(buffer) ? sizeof(buffer) : togo;
        o = 0;
        rv = apr_file_read(fd, buffer, &sendlen);
        while (rv == APR_SUCCESS && sendlen) {
            bytes_sent = sendlen;
            rv = apr_send(c->client_socket, &buffer[o], &bytes_sent);
            if (rv == APR_SUCCESS) {
                sendlen -= bytes_sent; /* sendlen != bytes_sent ==> partial write */
                o += bytes_sent;       /* o is where we are in the buffer */
                *nbytes += bytes_sent;
                togo -= bytes_sent;    /* track how much of the file we've sent */
            }
        }
    }

    /* Send the trailers
     * XXX: optimization... if it will fit, send this on the last send in the
     * loop above
     */
    sendlen = 0;
    if ( rv == APR_SUCCESS && hdtr && hdtr->numtrailers > 0 ) {
        for (i = 0; i < hdtr->numtrailers; i++) {
            sendlen += hdtr->trailers[i].iov_len;
        }
        rv = writev_it_all(c->client_socket, hdtr->trailers, hdtr->numtrailers,
                           sendlen, &bytes_sent);
        if (rv == APR_SUCCESS)
            *nbytes += bytes_sent;
    }

    return rv;
}

/* Default filter.  This filter should almost always be used.  Its only job
 * is to send the headers if they haven't already been sent, and then send
 * the actual data.
 */
#define MAX_IOVEC_TO_WRITE 16

static apr_status_t core_output_filter(ap_filter_t *f, apr_bucket_brigade *b)
{
    apr_status_t rv;
    conn_rec *c = f->c;
    core_net_rec *net = f->ctx;
    core_output_filter_ctx_t *ctx = net->out_ctx;

    if (ctx == NULL) {
        ctx = apr_pcalloc(c->pool, sizeof(*ctx));
        net->out_ctx = ctx;
    }

    /* If we have a saved brigade, concatenate the new brigade to it */
    if (ctx->b) {
        APR_BRIGADE_CONCAT(ctx->b, b);
        b = ctx->b;
        ctx->b = NULL;
    }

    /* Perform multiple passes over the brigade, sending batches of output
       to the connection. */
    while (b) {
        apr_size_t nbytes = 0;
        apr_bucket *last_e = NULL; /* initialized for debugging */
        apr_bucket *e;

        /* tail of brigade if we need another pass */
        apr_bucket_brigade *more = NULL;

        /* one group of iovecs per pass over the brigade */
        apr_size_t nvec = 0;
        apr_size_t nvec_trailers = 0;
        struct iovec vec[MAX_IOVEC_TO_WRITE];
        struct iovec vec_trailers[MAX_IOVEC_TO_WRITE];

        /* one file per pass over the brigade */
        apr_file_t *fd = NULL;
        apr_size_t flen = 0;
        apr_off_t foffset = 0;

        /* keep track of buckets that we've concatenated
         * to avoid small writes
         */
        apr_bucket *last_merged_bucket = NULL;

        /* Iterate over the brigade: collect iovecs and/or a file */
        APR_BRIGADE_FOREACH(e, b) {
            /* keep track of the last bucket processed */
            last_e = e;
            if (APR_BUCKET_IS_EOS(e) || APR_BUCKET_IS_FLUSH(e)) {
                break;
            }

            /* It doesn't make any sense to use sendfile for a file bucket
             * that represents 10 bytes.
             */
            else if (APR_BUCKET_IS_FILE(e)
                     && (e->length >= AP_MIN_SENDFILE_BYTES)) {
                apr_bucket_file *a = e->data;

                /* We can't handle more than one file bucket at a time
                 * so we split here and send the file we have already
                 * found.
                 */
                if (fd) {
                    more = apr_brigade_split(b, e);
                    break;
                }

                fd = a->fd;
                flen = e->length;
                foffset = e->start;
            }
            else {
                const char *str;
                apr_size_t n;

                rv = apr_bucket_read(e, &str, &n, APR_BLOCK_READ);
                if (n) {
                    if (!fd) {
                        if (nvec == MAX_IOVEC_TO_WRITE) {
                            /* woah! too many. buffer them up, for use later. */
                            apr_bucket *temp, *next;
                            apr_bucket_brigade *temp_brig;

                            if (nbytes >= AP_MIN_BYTES_TO_WRITE) {
                                /* We have enough data in the iovec
                                 * to justify doing a writev
                                 */
                                more = apr_brigade_split(b, e);
                                break;
                            }

                            /* Create a temporary brigade as a means
                             * of concatenating a bunch of buckets together
                             */
                            if (last_merged_bucket) {
                                /* If we've concatenated together small
                                 * buckets already in a previous pass,
                                 * the initial buckets in this brigade
                                 * are heap buckets that may have extra
                                 * space left in them (because they
                                 * were created by apr_brigade_write()).
                                 * We can take advantage of this by
                                 * building the new temp brigade out of
                                 * these buckets, so that the content
                                 * in them doesn't have to be copied again.
                                 */
                                apr_bucket_brigade *bb;
                                bb = apr_brigade_split(b,
                                         APR_BUCKET_NEXT(last_merged_bucket));
                                temp_brig = b;
                                b = bb;
                            }
                            else {
                                temp_brig = apr_brigade_create(f->c->pool,
                                                           f->c->bucket_alloc);
                            }

                            temp = APR_BRIGADE_FIRST(b);
                            while (temp != e) {
                                apr_bucket *d;
                                rv = apr_bucket_read(temp, &str, &n, APR_BLOCK_READ);
                                apr_brigade_write(temp_brig, NULL, NULL, str, n);
                                d = temp;
                                temp = APR_BUCKET_NEXT(temp);
                                apr_bucket_delete(d);
                            }

                            nvec = 0;
                            nbytes = 0;
                            temp = APR_BRIGADE_FIRST(temp_brig);
                            APR_BUCKET_REMOVE(temp);
                            APR_BRIGADE_INSERT_HEAD(b, temp);
                            apr_bucket_read(temp, &str, &n, APR_BLOCK_READ);
                            vec[nvec].iov_base = (char*) str;
                            vec[nvec].iov_len = n;
                            nvec++;

                            /* Just in case the temporary brigade has
                             * multiple buckets, recover the rest of
                             * them and put them in the brigade that
                             * we're sending.
                             */
                            for (next = APR_BRIGADE_FIRST(temp_brig);
                                 next != APR_BRIGADE_SENTINEL(temp_brig);
                                 next = APR_BRIGADE_FIRST(temp_brig)) {
                                APR_BUCKET_REMOVE(next);
                                APR_BUCKET_INSERT_AFTER(temp, next);
                                temp = next;
                                apr_bucket_read(next, &str, &n,
                                                APR_BLOCK_READ);
                                vec[nvec].iov_base = (char*) str;
                                vec[nvec].iov_len = n;
                                nvec++;
                            }

                            apr_brigade_destroy(temp_brig);

                            last_merged_bucket = temp;
                            e = temp;
                            last_e = e;
                        }
                        else {
                            vec[nvec].iov_base = (char*) str;
                            vec[nvec].iov_len = n;
                            nvec++;
                        }
                    }
                    else {
                        /* The bucket is a trailer to a file bucket */

                        if (nvec_trailers == MAX_IOVEC_TO_WRITE) {
                            /* woah! too many. stop now. */
                            more = apr_brigade_split(b, e);
                            break;
                        }

                        vec_trailers[nvec_trailers].iov_base = (char*) str;
                        vec_trailers[nvec_trailers].iov_len = n;
                        nvec_trailers++;
                    }

                    nbytes += n;
                }
            }
        }


        /* Completed iterating over the brigades, now determine if we want
         * to buffer the brigade or send the brigade out on the network.
         *
         * Save if:
         *
         *   1) we didn't see a file, we don't have more passes over the
         *      brigade to perform, we haven't accumulated enough bytes to
         *      send, AND we didn't stop at a FLUSH bucket.
         *      (IOW, we will save away plain old bytes)
         * or
         *   2) we hit the EOS and have a keep-alive connection
         *      (IOW, this response is a bit more complex, but we save it
         *       with the hope of concatenating with another response)
         */
        if ((!fd && !more
             && (nbytes + flen < AP_MIN_BYTES_TO_WRITE)
             && !APR_BUCKET_IS_FLUSH(last_e))
            || (nbytes + flen < AP_MIN_BYTES_TO_WRITE 
                && APR_BUCKET_IS_EOS(last_e) && c->keepalive)) {

            /* NEVER save an EOS in here.  If we are saving a brigade with
             * an EOS bucket, then we are doing keepalive connections, and
             * we want to process to second request fully.
             */
            if (APR_BUCKET_IS_EOS(last_e)) {
                apr_bucket *bucket = NULL;
                /* If we are in here, then this request is a keepalive.  We
                 * need to be certain that any data in a bucket is valid
                 * after the request_pool is cleared.
                 */
                if (ctx->b == NULL) {
                    ctx->b = apr_brigade_create(net->c->pool,
                                                net->c->bucket_alloc);
                }

                APR_BRIGADE_FOREACH(bucket, b) {
                    const char *str;
                    apr_size_t n;

                    rv = apr_bucket_read(bucket, &str, &n, APR_BLOCK_READ);

                    /* This apr_brigade_write does not use a flush function
                       because we assume that we will not write enough data
                       into it to cause a flush. However, if we *do* write
                       "too much", then we could end up with transient
                       buckets which would suck. This works for now, but is
                       a bit shaky if changes are made to some of the
                       buffering sizes. Let's do an assert to prevent
                       potential future problems... */
                    SB_DEBUG_ASSERT(AP_MIN_BYTES_TO_WRITE <=
                                    APR_BUCKET_BUFF_SIZE);
                    if (rv != APR_SUCCESS) {
                        error("core_output_filter: Error reading from bucket.");
                        return HTTP_INTERNAL_SERVER_ERROR;
                    }

                    apr_brigade_write(ctx->b, NULL, NULL, str, n);
                }

                apr_brigade_destroy(b);
            }
            else {
                ap_save_brigade(f, &ctx->b, &b, c->pool);
            }

            return APR_SUCCESS;
        }

        if (fd) {
            apr_hdtr_t hdtr;
#if APR_HAS_SENDFILE
            apr_int32_t flags = 0;
#endif

            memset(&hdtr, '\0', sizeof(hdtr));
            if (nvec) {
                hdtr.numheaders = nvec;
                hdtr.headers = vec;
            }

            if (nvec_trailers) {
                hdtr.numtrailers = nvec_trailers;
                hdtr.trailers = vec_trailers;
            }

#if APR_HAS_SENDFILE
            if (!c->keepalive && APR_BUCKET_IS_EOS(last_e)) {
                /* Prepare the socket to be reused */
                flags |= APR_SENDFILE_DISCONNECT_SOCKET;
            }

            rv = sendfile_it_all(net,      /* the network information   */
                                 fd,       /* the file to send          */
                                 &hdtr,    /* header and trailer iovecs */
                                 foffset,  /* offset in the file to begin
                                              sending from              */
                                 flen,     /* length of file            */
                                 nbytes + flen, /* total length including
                                                   headers                */
                                 flags);   /* apr_sendfile flags        */

            /* If apr_sendfile() returns APR_ENOTIMPL, call emulate_sendfile().
             * emulate_sendfile() is useful to enable the same Apache binary
             * distribution to support Windows NT/2000 (supports TransmitFile)
             * and Win95/98 (do not support TransmitFile)
             */
            if (rv == APR_ENOTIMPL)
#endif
            {
                apr_size_t unused_bytes_sent;
                rv = emulate_sendfile(net, fd, &hdtr, foffset, flen,
                                      &unused_bytes_sent);
            }

            fd = NULL;
        }
        else {
            apr_size_t unused_bytes_sent;

            rv = writev_it_all(net->client_socket,
                               vec, nvec,
                               nbytes, &unused_bytes_sent);
        }

        apr_brigade_destroy(b);
        if (rv != APR_SUCCESS) {
            info("core_output_filter: writing data to the network");

            if (more)
                apr_brigade_destroy(more);

            if (APR_STATUS_IS_ECONNABORTED(rv)
                || APR_STATUS_IS_ECONNRESET(rv)
                || APR_STATUS_IS_EPIPE(rv)) {
                c->aborted = 1;
            }

            /* The client has aborted, but the request was successful. We
             * will report success, and leave it to the access and error
             * logs to note that the connection was aborted.
             */
            return APR_SUCCESS;
        }

        b = more;
        more = NULL;
    }  /* end while () */

    return APR_SUCCESS;
}

/*****************************************************************************/
/* FILTER : net_time */

static int net_time_filter(ap_filter_t *f, apr_bucket_brigade *b,
                           ap_input_mode_t mode, apr_read_type_e block,
                           apr_off_t readbytes)
{
    int keptalive = f->c->keepalive == 1;
    apr_socket_t *csd = ap_get_module_config(f->c->conn_config, CORE_HTTPD_MODULE);
    int *first_line = f->ctx;

    //debug("1");
    if (!f->ctx) {
        f->ctx = first_line = apr_palloc(f->r->pool, sizeof(*first_line));
        *first_line = 1;
    }

    if (mode != AP_MODE_INIT && mode != AP_MODE_EATCRLF) {
        if (*first_line) {
            apr_setsocketopt(csd, APR_SO_TIMEOUT,
                             (int)(keptalive ? f->c->base_server->keep_alive_timeout
                      			     : f->c->base_server->timeout));
            *first_line = 0;
        }
        else {
            if (keptalive) {
                apr_setsocketopt(csd, APR_SO_TIMEOUT,
                                 (int)(f->c->base_server->timeout));
            }
        }
    }
    //debug("2");
    return ap_get_brigade(f->next, b, mode, block, readbytes);
}

/*****************************************************************************/
/* FILTER : old_write */

apr_status_t ap_old_write_filter(ap_filter_t *f, apr_bucket_brigade *bb)
{
    old_write_filter_ctx *ctx = f->ctx;

    SB_DEBUG_ASSERT(ctx);

    if (ctx->bb != 0) {
        /* whatever is coming down the pipe (we don't care), we
         * can simply insert our buffered data at the front and
         * pass the whole bundle down the chain.
         */
        APR_BRIGADE_CONCAT(ctx->bb, bb);
        bb = ctx->bb;
        ctx->bb = NULL;
    }

    return ap_pass_brigade(f->next, bb);
}

/*****************************************************************************/
void register_core_filters(void)
{
    ap_core_input_filter_handle =
        ap_register_input_filter("CORE_IN", core_input_filter,
                                 AP_FTYPE_NETWORK);
    ap_net_time_filter_handle =
        ap_register_input_filter("NET_TIME", net_time_filter,
                                 AP_FTYPE_PROTOCOL);

    ap_core_output_filter_handle =
        ap_register_output_filter("CORE", core_output_filter,
                                  AP_FTYPE_NETWORK);
	return;
}


