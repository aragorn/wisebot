/* $Id$ */
#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "hook.h"
#include "softbot.h"
#include "apr_network_io.h"
#include "apr_buckets.h"
#include "mod_httpd.h"

SB_DECLARE_HOOK(conn_rec *,create_connection,
	(apr_socket_t *sock, server_rec *server,
	 slot_t *slot, apr_bucket_alloc_t *alloc, apr_pool_t *p))
SB_DECLARE_HOOK(int,pre_connection,(conn_rec *c, apr_socket_t *sock))
SB_DECLARE_HOOK(int,process_connection,(conn_rec *c))
SB_DECLARE_HOOK(int,lingering_close_connection,(conn_rec *c))

#endif
