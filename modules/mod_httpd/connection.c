/* $Id$ */
#include "common_core.h"
#include "hook.h"
#include "mod_httpd.h"
#include "http_config.h"
#include "util_filter.h"
#include "connection.h"

HOOK_STRUCT(
	HOOK_LINK(create_connection)
	HOOK_LINK(pre_connection)
	HOOK_LINK(process_connection)
	HOOK_LINK(lingering_close_connection)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(conn_rec *,create_connection,
	(apr_socket_t *sock, server_rec *server,
	 slot_t *slot, apr_bucket_alloc_t *alloc, apr_pool_t *p),
	(sock, server, slot, alloc, p),NULL)
SB_IMPLEMENT_HOOK_RUN_ALL(int,pre_connection,
	(conn_rec *c, apr_socket_t *sock),
	(c, sock),SUCCESS,DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,process_connection,
	(conn_rec *c),
	(c),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,lingering_close_connection,
	(conn_rec *c),
	(c),DECLINE)


/*****************************************************************************/
/* from apache2, server/core.c */
static conn_rec
*create_connection(apr_socket_t *sock, server_rec *server,
					slot_t *slot, apr_bucket_alloc_t *alloc, apr_pool_t *p)
{
	apr_status_t rv;
	conn_rec *c = (conn_rec *) apr_palloc(p, sizeof(conn_rec));

	memset(c, '\0', sizeof *c);

	c->slot = slot;
	update_slot_state(c->slot, SLOT_READ, NULL);

	/* got a connection structure, so initialize what fields we can
	 * (the rest are zeroed out by palloc).
	 * ^-- palloc doesn't zero its result out - BTS
	 */
	c->conn_config = ap_create_conn_config(p);
	c->notes = apr_table_make(p, 5);

	c->pool = p;
	if ((rv = apr_socket_addr_get(&c->local_addr, APR_LOCAL, sock))
		!= APR_SUCCESS) {
		info("apr_socket_addr_get(APR_LOCAL): %s", strerror(errno));
		apr_socket_close(sock);
		return NULL;
	}
	apr_sockaddr_ip_get(&c->local_ip, c->local_addr);

	if ((rv = apr_socket_addr_get(&c->remote_addr, APR_REMOTE, sock))
		!= APR_SUCCESS) {
		info("apr_socket_addr_get(APR_REMOTE): %s", strerror(errno));
		apr_socket_close(sock);
		return NULL;
	}
	apr_sockaddr_ip_get(&c->remote_ip, c->remote_addr);
	c->base_server = server;

	c->id = slot->id;
	c->bucket_alloc = alloc;

	return c;
}

static int pre_connection(conn_rec *c, apr_socket_t *sock)
{
    core_net_rec *net = apr_palloc(c->pool, sizeof(*net));

    net->c = c;
    net->in_ctx = NULL;
    net->out_ctx = NULL;
    net->client_socket = sock;

    ap_set_module_config(net->c->conn_config, CORE_HTTPD_MODULE, sock);
    ap_add_input_filter("CORE_IN", net, NULL, net->c);
    ap_add_output_filter("CORE", net, NULL, net->c);
    return DONE;
}

void ap_flush_conn(conn_rec *c)
{
    apr_bucket_brigade *bb;
    apr_bucket *b;

    bb = apr_brigade_create(c->pool, c->bucket_alloc);
    b = apr_bucket_flush_create(c->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bb, b);
    ap_pass_brigade(c->output_filters, bb);
}

#define SECONDS_TO_LINGER  2
static int lingering_close_connection(conn_rec *c)
{
/* 아래 로직에 시간이 꽤 걸린다 --blueend */
#if 0
    char dummybuf[512];
    apr_size_t nbytes = sizeof(dummybuf);
    apr_status_t rc;
    apr_int32_t timeout;
    apr_int32_t total_linger_time = 0;
#endif
    apr_socket_t *csd = ap_get_module_config(c->conn_config, CORE_HTTPD_MODULE);

    if (!csd) {
        return SUCCESS;
    }

#if 0
	update_slot_state(c->slot, SLOT_CLOSING, NULL);

#ifdef NO_LINGCLOSE
    ap_flush_conn(c); /* just close it */
    apr_socket_close(csd);
    return SUCCESS;
#endif

    /* Close the connection, being careful to send out whatever is still
     * in our buffers.  If possible, try to avoid a hard close until the
     * client has ACKed our FIN and/or has stopped sending us data.
     */

    /* Send any leftover data to the client, but never try to again */
    ap_flush_conn(c);

    if (c->aborted) {
        apr_socket_close(csd);
        return SUCCESS;
    }

    /* Shut down the socket for write, which will send a FIN
     * to the peer.
     */
    if (apr_shutdown(csd, APR_SHUTDOWN_WRITE) != APR_SUCCESS
        || c->aborted) {
        apr_socket_close(csd);
        return SUCCESS;
    }

    /* Read all data from the peer until we reach "end-of-file" (FIN
     * from peer) or we've exceeded our overall timeout. If the client does
     * not send us bytes within 2 seconds (a value pulled from Apache 1.3
     * which seems to work well), close the connection.
     */
    timeout = SECONDS_TO_LINGER * APR_USEC_PER_SEC;
    apr_setsocketopt(csd, APR_SO_TIMEOUT, timeout);
    apr_setsocketopt(csd, APR_INCOMPLETE_READ, 1);
    while (1) {
        nbytes = sizeof(dummybuf);
        rc = apr_recv(csd, dummybuf, &nbytes);
        if (rc != APR_SUCCESS || nbytes == 0)
            break;

        total_linger_time += SECONDS_TO_LINGER;
        if (total_linger_time >= MAX_SECS_TO_LINGER) {
            break;
        }
    }
#endif
    apr_socket_close(csd);
    return SUCCESS;
}

static void register_hooks(void)
{
	/* create_connection is a hook that should always be HOOK_LAST
	 * to give other modules the opportunity to install alternate
	 * network transports and stop other functions from being run.
	 */
	sb_hook_create_connection(create_connection,NULL,NULL,HOOK_REALLY_LAST);
	sb_hook_pre_connection(pre_connection,NULL,NULL,HOOK_REALLY_LAST);
	sb_hook_lingering_close_connection(lingering_close_connection, NULL,NULL,HOOK_REALLY_LAST);
	return;
}

module httpd_connection_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};

