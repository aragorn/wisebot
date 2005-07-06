/* $Id$ */
#include "softbot.h"
#include "mod_api/tcp.h"

#if defined(AF_UNIX) && !defined(SUN_LEN) /* From UNP V1 (2e) R.I.P. Rich Stevens */
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path)) 
#endif

typedef struct {
	int blocked_recv;
	int total_recv;
	int blocked_send;
	int total_send;
} socket_block_stat_t;
static socket_block_stat_t *sbs;

#define DEFAULT_SERVER_TIMEOUT (10) /* 10초 */
#define DEFAULT_CLIENT_TIMEOUT (90) /* 90초 */
int g_server_timeout = DEFAULT_SERVER_TIMEOUT;
int g_client_timeout = DEFAULT_CLIENT_TIMEOUT;

static int tcp_recv_nonb (int sockfd, void *data, int len, int timeout);
static int tcp_send_nonb (int sockfd, void *data, int len, int timeout);
/*****************************************************************************/
#if defined(TCP_NODELAY) && !defined(MPE) && !defined(TPF) && !defined(WIN32)
static void sock_disable_nagle(int s)
{
    /* The Nagle algorithm says that we should delay sending partial
     * packets in hopes of getting more data.  We don't want to do
     * this; we are not telnet.  There are bad interactions between
     * persistent connections and Nagle's algorithm that have very severe
     * performance penalties.  (Failing to disable Nagle is not much of a
     * problem with simple HTTP.)
     *
     * In spite of these problems, failure here is not a shooting offense.
     */
	int just_say_no = 1;

	debug("called (sock_disable_nagle)!");
	
	if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
			(char *) &just_say_no, sizeof(int)) < 0) {
		debug("setsockopt (TCP_NODELAY): %s", strerror(errno));
		/* not a fatal error */
	}
}
#else
#  define sock_disable_nagle(s)		/* NOOP */
#endif

/*****************************************************************************/
/* from apache */
/* refer to apache 2.0, server/connection.c */

/*
 * More machine-dependent networking gooo... on some systems,
 * you've got to be *really* sure that all the packets are acknowledged
 * before closing the connection, since the client will not be able
 * to see the last response if their TCP buffer is flushed by a RST
 * packet from us, which is what the server's TCP stack will send
 * if it receives any request data after closing the connection.
 *
 * In an ideal world, this function would be accomplished by simply
 * setting the socket option SO_LINGER and handling it within the
 * server's TCP stack while the process continues on to the next request.
 * Unfortunately, it seems that most (if not all) operating systems
 * block the server process on close() when SO_LINGER is used.
 * For those that don't, see USE_SO_LINGER below.  For the rest,
 * we have created a home-brew lingering_close.
 *
 * Many operating systems tend to block, puke, or otherwise mishandle
 * calls to shutdown only half of the connection.  You should define
 * NO_LINGCLOSE in ap_config.h if such is the case for your system.
 */

#ifndef MAX_SECS_TO_LINGER
#  define MAX_SECS_TO_LINGER 30
#endif

#undef USE_SO_LINGER
#ifdef USE_SO_LINGER
#  define NO_LINGCLOSE		/* The two lingering options are exclusive */

static void sock_enable_linger(int s)
{
	struct linger li;

	li.l_onoff = 1;
	li.l_linger = MAX_SECS_TO_LINGER;

	if (setsockopt(s, SOL_SOCKET, SO_LINGER,
			(char *) &li, sizeof(struct linger)) < 0) {
		warn("setsockopt (SO_LINGER): %s", strerror(errno));
		/* not a fatal error */
	}
}

#else
#  define sock_enable_linger(s)   /* NOOP */
#endif /* USE_SO_LINGER */
/*****************************************************************************/

#ifdef AIX5
/*
   aix5.1 is too stupid to use getaddrinfo, and we don't really need it
   so here is a version which doesn't use it, but may have problems
   under weird systems, and ipv6 specifically
*/

static int tcp_connect(int *sockfd, char *host, char *port)
{
	struct sockaddr_in addr;
	int port_number, n, optval;
	struct hostent *host_entry;

	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( *sockfd == -1 ) {
		error("socket: %s", strerror(errno));
		return FAIL;
	}

	memset(&addr, 0x00, sizeof(addr));
	if ((host_entry=gethostbyname(host)) == NULL) {  // get the host info
		error("gethostbyname: %s", strerror(errno));
		return FAIL;
	}

	port_number = atoi(port);

	/* README connect(2) error occurs on AIX, 2004/08/02. -- aragorn
     * connect(2) returns error and errno is EEXIST(File exists), but
     * it's very weird error message.
     * So I steal codes form old softbot by kjb.
     */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_number);  // short, network byte order
	memcpy(&addr.sin_addr, *host_entry->h_addr_list, host_entry->h_length);
	/* below 2 lines are replace by upper one line, memcpy(...).
	addr.sin_addr = *((struct in_addr *)host_entry->h_addr);
	memset(&(addr.sin_zero), '\0', 8);  // zero the rest of the struct FIXME: why eight?
     */

	debug("trying to connect %s:%d...", host, ntohs(addr.sin_port) );
	n = connect_nonb(*sockfd, (struct sockaddr*)&addr, sizeof(addr), g_client_timeout);
	//n = connect(*sockfd, (struct sockaddr*)&addr, sizeof(addr));

	if ( n == SUCCESS ) { /* connected */
		debug("connected to %s:%d...", host, ntohs(addr.sin_port) );
		optval = 1;
		setsockopt(*sockfd, IPPROTO_TCP, TCP_NODELAY, (void*)&optval, sizeof(optval));
	} else {
		error("unable to connect %s:%d...", host, ntohs(addr.sin_port) );
		close(*sockfd);
		return FAIL;
	}

	return SUCCESS;
}

#else // AIX5
#ifdef CYGWIN
static int tcp_connect(int *sockfd, char *host, char *port)
{

	return SUCCESS;
}

#else
static int tcp_connect(int *sockfd, char *host, char *port)
{
	int n, optval;
	struct addrinfo hints, *res, *ressave;
	char buf[20];

	res = NULL; // init;

	memset(&hints, 0x00, sizeof(struct addrinfo));   
	hints.ai_family = AF_INET;   
	hints.ai_socktype = SOCK_STREAM;   
   
	/* XXX: keep pair of getaddrinfo(3) and freeaddrinfo(3) */   
	if ( (n = getaddrinfo(host, port, &hints, &res)) != 0 ) {   
		error("cannot getaddrinfo for %s:%s", host, port);   
		if ( n == EAI_SYSTEM )   
			error("getaddrinfo: %s - %s", gai_strerror(n), strerror(errno));   
		else error("getaddrinfo: %s", gai_strerror(n));   
		if ( res != NULL ) freeaddrinfo(res); // maybe always NULL?
		return FAIL;   
	}   
   
	ressave = res;

	do {
		*sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if ( *sockfd == -1 ) {
			error("socket: %s", strerror(errno));
			freeaddrinfo(ressave);
			return FAIL;
		}

		inet_ntop(AF_INET,
			(void *)&(((struct sockaddr_in *)(res->ai_addr))->sin_addr),
			buf, 20);
		debug("trying to connect %s:%d...", buf,
				ntohs(((struct sockaddr_in *)(res->ai_addr))->sin_port) );

		n = connect_nonb(*sockfd, (struct sockaddr*)res->ai_addr,
				res->ai_addrlen, g_client_timeout);

		if ( n == SUCCESS ) { /* connecting success */
			optval = 1;
			setsockopt(*sockfd, IPPROTO_TCP, TCP_NODELAY, (void*)&optval, sizeof(optval));
			break;
		} else {
			close(*sockfd);
		}
	} while ( (res = res->ai_next) != NULL );

	freeaddrinfo(ressave);

	if (res == NULL) {
		error("unable to connect %s:%s", host, port);
		close(*sockfd);
		return FAIL;
	}

	return SUCCESS;
}

#else

static int tcp_local_connect(int *sockfd, char *path)
{
	struct sockaddr_un addr;

	memset( &addr, 0, sizeof(addr) );
	addr.sun_family = AF_UNIX;
	if ( path[0] != '/' ) {
		snprintf( addr.sun_path, sizeof(addr.sun_path), "%s/%s", gSoftBotRoot, path );
	}
	else strncpy( addr.sun_path, path, sizeof(addr.sun_path) );

	*sockfd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if ( *sockfd == -1 ) {
		error("socket: %s", strerror(errno));
		return FAIL;
	}

	debug("trying to connect %s", addr.sun_path);

	if ( connect( *sockfd, (struct sockaddr*)&addr, sizeof(addr) ) == -1 ) {
		error("connect error:%s", strerror(errno));
		close(*sockfd);
		return FAIL;
	}

	return SUCCESS;
}

static int tcp_close(int sockfd)
{
	if ( close(sockfd) == 0 ) 
		return SUCCESS;
	else
		warn("close(sockfd[%d]) returned error: %s", sockfd, strerror(errno));

	return FAIL;
}

/* we now proceed to read from the client until we get EOF, or until
 * MAX_SECS_TO_LINGER has passed.  the reasons for doing this are
 * documented in a draft:
 *
 * http://www.ics.uci.edu/pub/ietf/http/draft-ietf-http-connection-00.txt
 *
 * in a nutshell -- if we don't make this effort we risk causing
 * TCP RST packets to be sent which can tear down a connection before
 * all the response data has been sent to the client.
 */
#define SECONDS_TO_LINGER  2
static int tcp_lingering_close(int sockfd)
{
	char dummybuf[512];
	int nbytes = sizeof(dummybuf);
	int rc;
	int timeout;
	int total_linger_time = 0;

	debug("shutting down");
#ifdef NO_LINGCLOSE
	return tcp_close(sockfd); /* just close it */
#endif

	/* shutdown the socket for write, which will send a FIN to the peer.
	 */
	if ( shutdown(sockfd, SHUT_WR) != 0 ) {
		warn("shutdown failed: %s", strerror(errno));
		return tcp_close(sockfd);
	}
	
	/* Read all data from the peer until we reach "end-of-file" (FIN
	 * from peer) or we've exceeded our overall timeout. If the client does
	 * not send us bytes within 2 seconds (a value pulled from Apache 1.3
	 * which seems to work well), close the connection.
	 */
	timeout = SECONDS_TO_LINGER;
	while ( 1 ) {
		nbytes = sizeof(dummybuf);
		rc = tcp_recv_nonb(sockfd, dummybuf, nbytes, timeout);
		if (rc != SUCCESS)
			break;

		total_linger_time += SECONDS_TO_LINGER;
		if (total_linger_time >= MAX_SECS_TO_LINGER)
			break;
	}

	return tcp_close(sockfd);
}


#ifndef AIX5
static int 
tcp_bind_listen (const char *host, const char *port, const int backlog, int *listenfd)
{
	int n;
	const int on = 1;
	const char *node = host;
	struct addrinfo hints, *res;

	memset(&hints, 0x00, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (strcmp(host, "*") == 0) {
		hints.ai_flags = AI_PASSIVE;
		node = NULL;
	}

	if ( (n = getaddrinfo(node, port, &hints, &res)) != 0 ) {
		alert("cannot getaddrinfo for %s:%s", host, port);
		if ( n == EAI_SYSTEM )
			error("getaddrinfo: %s - %s", gai_strerror(n), strerror(errno));
		else error("getaddrinfo: %s, errno:%d, %s", gai_strerror(n), errno, strerror(errno));
		return FAIL;
	}

	*listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if ( *listenfd == -1 ) {
		error("socket: %s", strerror(errno));
		return FAIL;
	}

	if ( setsockopt(*listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0 ) {
		close(*listenfd);
		alert("setsockopt: %s", strerror(errno));
		return FAIL;
	}

	/* XXX Some OSes allow listening socket attributes to be inherited
	 * by the accept sockets which means this call only needs to be
	 * made once on the listener.
	 * or should be called with every accept sockets.
	 */
	sock_disable_nagle(*listenfd);

	debug("binding %s:%s", host, port);
	if ( bind(*listenfd, res->ai_addr, res->ai_addrlen) != 0 ) {
		close(*listenfd);
		alert("bind[%s:%s] error: %s", host, port, strerror(errno));
		return FAIL;
	}

	if ( listen(*listenfd, backlog) != 0 ) {
		close(*listenfd);
		alert("listen: %s", strerror(errno));
		return FAIL;
	}

	freeaddrinfo(res);

	return SUCCESS;
}
#else

/*
   aix5.1 is too stupid to use getaddrinfo, and we don't really need it
   so here is a version which doesn't use it, but may have problems
   under weird systems, and ipv6 specifically
*/

static int 
tcp_bind_listen (const char *host, const char *port, const int backlog, int *listenfd)
{
	const int on = 1;
	struct sockaddr_in addr;
	struct hostent *host_entry = NULL;
	int port_number;
    int hostaddr_any = 0;
    const char *name = host;

	if (strcmp(host, "*") == 0) {
        hostaddr_any = 1;
		name = NULL;
	} else if ((host_entry=gethostbyname(name)) == NULL) {  // get the host info
		error("gethostbyname: %s", strerror(errno));
		return FAIL;
	}

	*listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( *listenfd == -1 ) {
		error("socket: %s", strerror(errno));
		return FAIL;
	}

	if ( setsockopt(*listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0 ) {
		close(*listenfd);
		alert("setsockopt: %s", strerror(errno));
		return FAIL;
	}

	/* XXX Some OSes allow listening socket attributes to be inherited
	 * by the accept sockets which means this call only needs to be
	 * made once on the listener.
	 * or should be called with every accept sockets.
	 */
	sock_disable_nagle(*listenfd);

	port_number = atoi(port); // zero if bad

	addr.sin_family = AF_INET;    	   // host byte order
	addr.sin_port = htons(port_number);  // short, network byte order
    if (hostaddr_any)
	    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else
	    addr.sin_addr = *((struct in_addr *)host_entry->h_addr);
	memset(&(addr.sin_zero), '\0', 8);  // zero the rest of the struct FIXME: why eight?


	debug("binding %s:%s", host, port);
	if ( bind(*listenfd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) {
		close(*listenfd);
		alert("bind[%s:%s] error: %s", host, port, strerror(errno));
		return FAIL;
	}

	if ( listen(*listenfd, backlog) != 0 ) {
		close(*listenfd);
		alert("listen: %s", strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}
#endif // STUPIDAIX

static int 
tcp_local_bind_listen (const char *path, const int backlog, int *listenfd)
{
	struct sockaddr_un addr;
	char lock_path[MAX_PATH_LEN];
	int fd;

	*listenfd = -1;

	memset( &addr, 0, sizeof(addr) );
	addr.sun_family = AF_UNIX;

	if ( path[0] != '/' ) {
		snprintf( addr.sun_path, sizeof(addr.sun_path), "%s/%s", gSoftBotRoot, path );
	}
	else strncpy( addr.sun_path, path, sizeof(addr.sun_path) );

	// sb_open은 상대경로를 요구한다.
	snprintf( lock_path, sizeof(lock_path), "%s.lock", path );

	// path로 사용할 파일에 대한 처리...
	// lock file을 잡아보면 이미 bind 되어있는지 알 수있다.
	// fd 는 process가 끝날 때까지 닫지 않고 lock도 풀지 않는다.
	fd = sb_lockfile( lock_path );
	if ( fd == -1 ) {
		error("socket lock file[%s] failed: %s", lock_path, strerror(errno));
		return FAIL;
	}

	do {
		unlink( addr.sun_path );
		*listenfd = socket( AF_UNIX, SOCK_STREAM, 0 );
		if ( *listenfd == -1 ) {
			error("socket: %s", strerror(errno));
			break;
		}

		info("binding:%s", addr.sun_path);
		if ( bind(*listenfd, (struct sockaddr*)&addr, SUN_LEN(&addr)) != 0 ) {
			alert("bind[%s] error: %s", addr.sun_path, strerror(errno));
			break;
		}

		if ( listen(*listenfd, backlog) != 0 ) {
			alert("listen: %s", strerror(errno));
			break;
		}

		return SUCCESS;
	} while (0);

	if ( *listenfd != -1 ) close(*listenfd);
	if ( sb_unlockfile( fd ) != SUCCESS )
		error("socket file unlock failed: %s", strerror(errno));
	close(fd);

	return FAIL;
}

static int 
tcp_select_accept(int listenfd, int *sockfd,
					struct sockaddr *remote_addr, socklen_t *len)
{
	int n;
	fd_set rset;

	for ( ; ; ) {
		FD_ZERO(&rset);
		FD_SET(listenfd, &rset);
		n = select(listenfd+1, &rset, NULL, NULL, NULL);

		if ( n == -1 && errno == EINTR ) {
			/* got a signal */

			debug("got a signal");
			/* this signal could be shutdown/restart signal,
			 * so we get out of this function
			 * and then caller should check the scoreboard->shutdown.
			 * if the signal is not shutdown/restart signal,
			 * it's fine for this function to return FAIL,
			 * for there would not be much signals.
			 * CAUTION: caller should not ignore SIGPIPE and other signals.
			 * if caller ignores them, tcp_select_accept won't work properly.
			 * see mod_softbot4.c as an example. */
			return FAIL;
		}
		else if ( n == -1 ) { /* unexpected error */
			warn("unexpected error with select()");
			return FAIL;
		}
		else { /* ok. we got a connection */
			DEBUG("select() got a request");
			break;
		}
	}

	/* FIXME make accept() non-blocking */
	*sockfd = accept(listenfd, remote_addr, len);
	if ( *sockfd < 0 ) {
		warn("accept() returned invalid sockfd(%d)", *sockfd);
		return FAIL;
	}

	/* XXX Some OSes allow listening socket attributes to be inherited
	 * by the accept sockets which means this call only needs to be
	 * made once on the listener.
	 * or should be called with every accept sockets.
	 */
	sock_disable_nagle(*sockfd);
	//sock_enable_linger(*sockfd);

	/* FIXME we have to consider SO_LINGER option. see below. */

	return SUCCESS;
}

static int tcp_recv_nonb (int sockfd, void *data, int len, int timeout)
{
	int size = len;
	char *ptr = data;
	fd_set		rset;
	struct timeval	tval;

	if (len == 0) 
		return SUCCESS;

	while ( 1 ) {
		int n = 0;
#ifdef AIX5
		n = recv(sockfd, ptr, size, MSG_NONBLOCK);
#else
		n = recv(sockfd, ptr, size, MSG_DONTWAIT);
#endif

		sbs->total_recv++;
		if ( n > 0 ) {
			if ( n == size ) break;
			
			/* partly received */
			ptr += n;
			size -= n;
			continue; // retry to receive
		} else if ( n == 0 ) {
			/* maybe connection error */
			warn("recv() returned 0");
			return FAIL;
		} else {
			/* retry or handle error */
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
#ifdef AIX5
				if (errno != 17) /* FIXME errno[17] => "File exists" */
				return FAIL;
#else
				warn("recv() returned %d: %s[%d]", n, strerror(errno), errno);
				return FAIL;
#endif
			}

			sbs->blocked_recv++;
			/* we have to retry, so wait for socket to be ready */
			/* XXX: we HAVE TO set fd_set and timeval every time 
			 *      before calling select. see select(2)
			 *      (timeval is linux specific problem)
			 */
			FD_ZERO(&rset);
			FD_SET(sockfd, &rset);
			tval.tv_sec = timeout;
			tval.tv_usec = 0;

			if ( (n = select(sockfd +1, &rset, NULL, NULL, &tval)) == 0 ) {
				error("recv timedout(timeout = %d)", timeout);
				errno = ETIMEDOUT;
				return FAIL;
			} else if ( n == -1 ) {
				error("select: %s", strerror(errno));
				return FAIL;
			} else {
#if 0
//#ifdef DEBUG_SOFTBOTD
				static pid = 0;

				if (pid == 0) pid = getpid();
				INFO("socket is now readable, so continue the loop. pid[%d]", pid);
#endif
				; /* socket is now readable, so continue the loop */
			}
		}

	} // while ( 1 )

	return SUCCESS;
}

static int tcp_send_nonb (int sockfd, void *data, int len, int timeout)
{
	int size = len;
	char *ptr = data;
	fd_set		wset;
	struct timeval	tval;


	while ( 1 ) {
		int n = 0;

#ifdef AIX5
		n = send(sockfd, ptr, size, MSG_NONBLOCK);
#else
		n = send(sockfd, ptr, size, MSG_DONTWAIT);
#endif

		sbs->total_send++;
		if ( n > 0 ) {
			if ( n == size ) break;
			
			/* partly sent */
			ptr += n;
			size -= n;
			continue; // retry to send
		} else if ( n == 0 ) {
			/* maybe connection error */
			warn("send() returned 0");
			return FAIL;
		} else {
			/* retry or handle error */
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
#ifdef AIX5
				if (errno != 17) /* FIXME errno[17] => "File exists" */
				return FAIL;
#else
				warn("send() returned %d: %s", n, strerror(errno));
				return FAIL;
#endif
			}

			sbs->blocked_send++;
			/* we have to retry, so wait for socket to be ready */
			/* XXX: we HAVE TO set fd_set and timeval every time 
			 *      before calling select. see select(2)
			 *      (timeval is linux specific problem)
			 */
			FD_ZERO(&wset);
			FD_SET(sockfd, &wset);
			tval.tv_sec = timeout;
			tval.tv_usec = 0;

			if ( (n = select(sockfd +1, NULL, &wset, NULL, &tval)) == 0 ) {
				error("send timedout(timeout = %d)", timeout);
				errno = ETIMEDOUT;
				return FAIL;
			} else if ( n == -1 ) {
				error("select: %s", strerror(errno));
				return FAIL;
			} else {
				; /* socket is now writable, so continue the loop */
			}
		}

	} // while ( 1 )

	return SUCCESS;
}

static int tcp_server_timeout()
{
	return g_server_timeout;
}

static int tcp_client_timeout()
{
	return g_client_timeout;
}

/*****************************************************************************/
static void init_socket_block_stat(void *data)
{
	sbs = data;
	memset(sbs, 0x00, sizeof(socket_block_stat_t));
}

static char* get_socket_block_stat()
{
	socket_block_stat_t *p = sbs;

	static char buf[SHORT_STRING_SIZE*2];
	sprintf(buf, "recv %d/%d %02.1f%%, send %d/%d %02.1f%%",
		p->blocked_recv, p->total_recv,
		(p->total_recv == 0) ? 0 : (float)p->blocked_recv/p->total_recv*100,
		p->blocked_send, p->total_send,
		(p->total_send == 0) ? 0 : (float)p->blocked_send/p->total_send*100);
	return buf;
}

static registry_t registry[] = {
	RUNTIME_REGISTRY("SocketBlockStat", "blocked recv/send statistics",\
		4*sizeof(int), init_socket_block_stat, get_socket_block_stat, NULL),
	NULL_REGISTRY
};

static void register_hooks(void)
{
	sb_hook_tcp_connect(tcp_connect,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_close(tcp_close,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_lingering_close(tcp_lingering_close,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_bind_listen(tcp_bind_listen,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_select_accept(tcp_select_accept,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_recv(tcp_recv_nonb,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_send(tcp_send_nonb,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_local_connect(tcp_local_connect,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_local_bind_listen(tcp_local_bind_listen,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_server_timeout(tcp_server_timeout,NULL,NULL,HOOK_MIDDLE);
	sb_hook_tcp_client_timeout(tcp_client_timeout,NULL,NULL,HOOK_MIDDLE);
}

static void set_server_timeout(configValue v) {
	g_server_timeout = atoi(v.argument[0]);
	info("server time out is setted as %d", g_server_timeout);
}

static void set_client_timeout(configValue v) {
	g_client_timeout = atoi(v.argument[0]);
	info("client time out is setted as %d", g_client_timeout);
}

static config_t config[] = {
	CONFIG_GET("TimeOut",       set_server_timeout, 1, "tcp timeout"),
	CONFIG_GET("ServerTimeOut", set_server_timeout, 1, "server side timeout (==TimeOut, recommend 10 seconds)"),
	CONFIG_GET("ClientTimeOut", set_client_timeout, 1, "client side timeout (recommended 90 seconds)"),
	{NULL}
};

module tcp_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	registry,				/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};

