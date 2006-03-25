/* $Id$ */
#include "listen.h"
#include "mod_httpd.h"
#include "apr_portable.h" // apr_os_sock_get()

listen_rec *listeners = NULL;

#if APR_HAVE_IPV6
static int default_family = APR_UNSPEC;
#else
static int default_family = APR_INET;
#endif
static int send_buffer_size = 0; /* 0: use system default */

static void find_default_family(apr_pool_t *p)
{
#if APR_HAVE_IPV6
	/* We know the platform supports IPv6, but this particular
	 * system may not have IPv6 enabled.  See if we can get an
	 * AF_INET6 socket.
	 */
	if (default_family == APR_UNSPEC) {
		apr_socket_t *tmp_sock;

		if (apr_socket_create(&tmp_sock, APR_INET6, SOCK_STREAM, p)
				== APR_SUCCESS) {
			apr_socket_close(tmp_sock);
			default_family = APR_INET6;
		}
		else {
			default_family = APR_INET;
		}
	}
#endif
}

static int alloc_listener(const char *addr,
						  apr_port_t port, int backlog, apr_pool_t *p)
{
	listen_rec *new;
	apr_status_t status;

	if (addr == NULL) { /* don't bind to specific interface */
		find_default_family(p);
		switch (default_family) {
		case APR_INET:
			addr = "0.0.0.0";
			break;
#if APR_HAVE_IPV6
		case APR_INET6:
			addr = "::";
			break;
#endif
		default:
			sb_assert(1 != 1); /* should not occur */
		}
	}

	new = apr_palloc(p, sizeof(listen_rec));
	new->active = 0;
	new->backlog = backlog;
	if ((status = apr_sockaddr_info_get(&new->bind_addr, addr, APR_UNSPEC,
										port, 0, p))
		!= APR_SUCCESS) {
		crit("failed to set up sockaddr for %s", addr);
		return FAIL;
	}
	if ((status = apr_socket_create(&new->sd,
									new->bind_addr->family,
									SOCK_STREAM, p))
		!= APR_SUCCESS) {
		crit("failed to get a socket for %s", addr);
		return FAIL;
	}

	new->next = listeners;
	listeners = new;
	return SUCCESS;
}

int set_listener(char *address, int backlog, apr_pool_t *p)
{
	char *host, *scope_id;
	apr_port_t port;
	apr_status_t rv;

	rv = apr_parse_addr_port(&host, &scope_id, &port, address, p);
	if (rv != APR_SUCCESS) {
		error("Invalid address or port");
		return FAIL;
	}

	if (host && (strcmp(host, "*")==0)) {
		host = NULL;
	}

	if (scope_id) {
		/* XXX scope id support is useful with link-local IPv6 addresses */
		error("Scope id is not supported");
		return FAIL;
	}

	if (!port) {
		error("Port must be specified");
		return FAIL;
	}

	return alloc_listener(host, port, backlog, p);
}



static apr_status_t make_sock(listen_rec *listener, apr_pool_t *p)
{
	apr_socket_t *s = listener->sd;
	int one = 1;
	apr_status_t stat;

#ifndef WIN32
	stat = apr_setsocketopt(s, APR_SO_REUSEADDR, one);
	if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
		crit("for address %pI, setsockopt: (SO_REUSEADDR)", listener->bind_addr);
		apr_socket_close(s);
		return stat;
	}
#endif

	stat = apr_setsocketopt(s, APR_SO_KEEPALIVE, one);
	if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
		crit("for address %pI, setsockopt: (SO_KEEPALIVE)", listener->bind_addr);
		apr_socket_close(s);
		return stat;
	}

    /*
     * To send data over high bandwidth-delay connections at full
     * speed we must force the TCP window to open wide enough to keep the
     * pipe full.  The default window size on many systems
     * is only 4kB.  Cross-country WAN connections of 100ms
     * at 1Mb/s are not impossible for well connected sites.
     * If we assume 100ms cross-country latency,
     * a 4kB buffer limits throughput to 40kB/s.
     *
     * To avoid this problem I've added the SendBufferSize directive
     * to allow the web master to configure send buffer size.
     *
     * The trade-off of larger buffers is that more kernel memory
     * is consumed.  YMMV, know your customers and your network!
     *
     * -John Heidemann <johnh@isi.edu> 25-Oct-96
     *
     * If no size is specified, use the kernel default.
     */
	/* default values of debian linux 2.4.17 in KB
	 *                     min     default  max
	 * net/ipv4/tcp_rmem = 4096    87380    174760
	 * net/ipv4/tcp_wmem = 4096    16384    131072
	 * net/ipv4/tcp_mem = 195584   196096   196608
	 */
	if (send_buffer_size) {
		stat = apr_setsocketopt(s, APR_SO_SNDBUF, send_buffer_size);
		if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
			warn("failed to set SendBufferSize for "
				 "address %pI, using default", listener->bind_addr);
			/* not a fatal error */
		}
	}
#if APR_TCP_NODELAY_INHERITED
	//sock_disable_nagle(s);
#endif

	if ((stat = apr_bind(s, listener->bind_addr)) != APR_SUCCESS) {
		crit("could not bind to address %pI", listener->bind_addr);
		apr_socket_close(s);
		return stat;
	}

	if ((stat = apr_listen(s, listener->backlog)) != APR_SUCCESS) {
		crit("unable to listen for connections "
			 "on address %pI", listener->bind_addr);
		apr_socket_close(s);
		return stat;
	}

#ifndef WIN32
	/* from apache2, server/listen.c :
	 *
     * I seriously doubt that this would work on Unix; I have doubts that
     * it entirely solves the problem on Win32.  However, since setting
     * reuseaddr on the listener -prior- to binding the socket has allowed
     * us to attach to the same port as an already running instance of
     * Apache, or even another web server, we cannot identify that this
     * port was exclusively granted to this instance of Apache.
     *
     * So set reuseaddr, but do not attempt to do so until we have the
     * parent listeners successfully bound.
     */
	stat = apr_setsocketopt(s, APR_SO_REUSEADDR, one);
	if (stat != APR_SUCCESS && stat != APR_ENOTIMPL) {
		crit("for address %pI, setsockopt: (SO_REUSEADDR)", listener->bind_addr);
		apr_socket_close(s);
		return stat;
	}
#endif

	listener->active = 1;
	listener->accept_func = unixd_accept;
	// FIXME was NULL. is this ok?

	return APR_SUCCESS;
}

int setup_listeners(apr_pool_t *p)
{
	listen_rec *lr;
	int num_listeners = 0;
	int num_open;

	/* Don't allocate a default listener. If we need to listen to a port,
	 * then the user needs to have a Listen directive in their config file.
	 */
	num_open = 0;
	for (lr = listeners; lr; lr = lr->next) {
		if (lr->active) {
			++num_open;
		}
		else {
			/* create socket, bind and listen */
			if (make_sock(lr, p) == APR_SUCCESS) {
				++num_open;
				lr->active = 1;
			} else {
				/* fatal error */
				return -1;
			}
		}
	}


	for (lr = listeners; lr; lr = lr->next) {
		num_listeners++;
	}

	return num_listeners;
}


/*****************************************************************************/
apr_status_t unixd_accept(apr_socket_t **accepted, listen_rec *lr,
                                        apr_pool_t *ptrans)
{
    apr_socket_t *csd;
    apr_status_t status;
    int sockdes;

    *accepted = NULL;
    status = apr_accept(&csd, lr->sd, ptrans);
    if (status == APR_SUCCESS) { 
        *accepted = csd;
        apr_os_sock_get(&sockdes, csd);
        if (sockdes >= FD_SETSIZE) {
			warn("new file descriptor %d is too large; you probably need "
                 "to rebuild SoftBot with a larger FD_SETSIZE "
                 "(currently %d)",
                 sockdes, FD_SETSIZE);
            apr_socket_close(csd);
            return APR_EINTR;
        } 
#ifdef TPF
        if (sockdes == 0) {                  /* 0 is invalid socket for TPF */
            return APR_EINTR;
        }
#endif
        return status;
    }

    if (APR_STATUS_IS_EINTR(status)) {
        return status;
    }
    /* Our old behaviour here was to continue after accept()
     * errors.  But this leads us into lots of troubles
     * because most of the errors are quite fatal.  For
     * example, EMFILE can be caused by slow descriptor
     * leaks (say in a 3rd party module, or libc).  It's
     * foolish for us to continue after an EMFILE.  We also
     * seem to tickle kernel bugs on some platforms which
     * lead to never-ending loops here.  So it seems best
     * to just exit in most cases.
     */
    switch (status) {
#if defined(HPUX11) && defined(ENOBUFS)
        /* On HPUX 11.x, the 'ENOBUFS, No buffer space available'
         * error occurs because the accept() cannot complete.
         * You will not see ENOBUFS with 10.20 because the kernel
         * hides any occurrence from being returned to user space.
         * ENOBUFS with 11.x's TCP/IP stack is possible, and could
         * occur intermittently. As a work-around, we are going to
         * ignore ENOBUFS.
         */
        case ENOBUFS:
#endif

#ifdef EPROTO
        /* EPROTO on certain older kernels really means
         * ECONNABORTED, so we need to ignore it for them.
         * See discussion in new-httpd archives nh.9701
         * search for EPROTO.
         *
         * Also see nh.9603, search for EPROTO:
         * There is potentially a bug in Solaris 2.x x<6,
         * and other boxes that implement tcp sockets in
         * userland (i.e. on top of STREAMS).  On these
         * systems, EPROTO can actually result in a fatal
         * loop.  See PR#981 for example.  It's hard to
         * handle both uses of EPROTO.
         */
        case EPROTO:
#endif
#ifdef ECONNABORTED
        case ECONNABORTED:
#endif
        /* Linux generates the rest of these, other tcp
         * stacks (i.e. bsd) tend to hide them behind
         * getsockopt() interfaces.  They occur when
         * the net goes sour or the client disconnects
         * after the three-way handshake has been done
         * in the kernel but before userland has picked
         * up the socket.
         */
#ifdef ECONNRESET
        case ECONNRESET:
#endif
#ifdef ETIMEDOUT
        case ETIMEDOUT:
#endif
#ifdef EHOSTUNREACH
        case EHOSTUNREACH:
#endif
#ifdef ENETUNREACH
        case ENETUNREACH:
#endif
            break;
#ifdef ENETDOWN
        case ENETDOWN:
            /*
             * When the network layer has been shut down, there
             * is not much use in simply exiting: the parent
             * would simply re-create us (and we'd fail again).
             * Use the CHILDFATAL code to tear the server down.
             * @@@ Martin's idea for possible improvement:
             * A different approach would be to define
             * a new APEXIT_NETDOWN exit code, the reception
             * of which would make the parent shutdown all
             * children, then idle-loop until it detected that
             * the network is up again, and restart the children.
             * Ben Hyde noted that temporary ENETDOWN situations
             * occur in mobile IP.
             */
			emerg("apr_accept: giving up.");
            return APR_EGENERAL;
#endif /*ENETDOWN*/

#ifdef TPF
        case EINACT:
			emerg("offload device inactive");
            return APR_EGENERAL;
            break;
        default:
			error("select/accept error (%d)", status);
            return APR_EGENERAL;
#else
        default:
			error("apr_accept: (client socket)");
            return APR_EGENERAL;
#endif
    }
    return status;
}

