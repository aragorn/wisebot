/* $Id$ */
#ifndef LISTEN_H
#define LISTEN_H

#include "apr_errno.h"
#include "apr_network_io.h"

/**
 * @package HTTPD Listeners Library
 */

typedef struct listen_rec listen_rec;
typedef apr_status_t 
	(*accept_function)(apr_socket_t **csd, listen_rec *lr, apr_pool_t *pool);

/* listeners record */
struct listen_rec {
	listen_rec *next; /* the next listener in the list */
	apr_socket_t *sd; /* actual socket */
	apr_sockaddr_t *bind_addr;
	accept_function accept_func;
	int active; /* is this socket currently active? */
	int backlog;
};

/* the global list of listen_rec structures */
extern listen_rec *listeners;

int set_listener(char *address, int backlog, apr_pool_t *p);
int setup_listeners(apr_pool_t *p);

apr_status_t unixd_accept(apr_socket_t **accepted, listen_rec *lr, apr_pool_t *ptrans);
#endif

