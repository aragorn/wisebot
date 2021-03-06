/* $Id$ */
#ifndef CONNECTION_H
#define CONNECTION_H

/* This is a workaround for exports.c which does not include "common_core.h"
 * by default. */
#include "common_core.h"
#include "hook.h"
#include "mod_httpd.h"

SB_DECLARE_HOOK(conn_rec *,create_connection,
	(apr_socket_t *sock, server_rec *server,
	 slot_t *slot, apr_bucket_alloc_t *alloc, apr_pool_t *p))
SB_DECLARE_HOOK(int,pre_connection,(conn_rec *c, apr_socket_t *sock))
SB_DECLARE_HOOK(int,process_connection,(conn_rec *c))
SB_DECLARE_HOOK(int,lingering_close_connection,(conn_rec *c))

#endif
