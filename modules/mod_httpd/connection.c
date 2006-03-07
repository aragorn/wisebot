/* $Id$ */
#include "softbot.h"
#include "connection.h"
#include "core.h"
#include "config.h"

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

static void flush_conn(conn_rec *c)
{
	// see apache server/connection.c
}

static int lingering_close_connection(conn_rec *c)
{
	flush_conn(c);

	// FIXME howto get c->sockfd??
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

