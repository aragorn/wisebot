/* $Id$ */
#include "mod_api/udp.h"

#if !defined(HAVE_GETADDRINFO)
/*
 * AIX 5.x and CYGWIN does not support getaddrinfo.
*/

static int
udp_connect(int *sockfd, const char *host, const char *port)
{
  sb_assert("udp_connect() is not supported on this platform.");

  return SUCCESS;
}
#else
static int
udp_connect(int *sockfd, const char *host, const char *port)
{
	int n;
	struct addrinfo hints, *res;
	char buf[20];

	memset(&hints, 0x00, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	/* XXX: keep pair of getaddrinfo(3) and freeaddrinfo(3) */
	if ( (n = getaddrinfo(host, port, &hints, &res)) != 0 ) {
		alert("cannot getaddrinfo for %s:%s", host, port);
		if ( n == EAI_SYSTEM )
			error("getaddrinfo: %s - %s", gai_strerror(n), strerror(errno));
		else error("getaddrinfo: %s", gai_strerror(n));
		return FAIL;
	}

	*sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if ( *sockfd == -1 ) {
		error("socket: %s", strerror(errno));
		freeaddrinfo(res);
		return FAIL;
	}


	inet_ntop(AF_INET,
		(void *)&(((struct sockaddr_in *)(res->ai_addr))->sin_addr),
		buf, 20);
	debug("trying to connect %s:%d...", buf,
			ntohs(((struct sockaddr_in *)(res->ai_addr))->sin_port) );

	n = connect(*sockfd, (struct sockaddr*)res->ai_addr,
			res->ai_addrlen);

	freeaddrinfo(res);

	return n;
}
#endif // #if !defined(HAVE_GETADDRINFO)

#if defined(CYGWIN) || defined(HPUX)
static int set_non_blocking(int s, int flag)
{
  int socket_flags;

  socket_flags = fcntl(s, F_GETFL);
  if (socket_flags == -1)
  {
    error("cannot fcntl(s, F_GETFL): %s", strerror(errno));
    return FAIL;
  }
  
  if (flag == TRUE)
    socket_flags |= O_NONBLOCK;
  else
    socket_flags &= ~O_NONBLOCK;

  if ( fcntl(s, F_SETFL, socket_flags) == -1 )
  {
    error("cannot fcntl(s, F_SETFL, %d): %s", socket_flags, strerror(errno));
    return FAIL;
  } else return SUCCESS;
}
#else
#  define set_non_blocking(s,flag) SUCCESS
#endif

static int
udp_recvfrom(int sockfd, void *buf, size_t len,
		struct sockaddr *from, socklen_t *fromlen, int timeout)
{
	fd_set rset;
	struct timeval tval;

    if (set_non_blocking(sockfd, TRUE) == FAIL)
	{
		error("cannot set socket non-blocking: %s", strerror(errno));
		return FAIL;
    }

	for ( ; ; ) {
		int n = 0;

#if defined(AIX5)
		n = recvfrom(sockfd, buf, len, MSG_NONBLOCK, from, fromlen);
#elif defined(CYGWIN) || defined(HPUX)
		n = recvfrom(sockfd, buf, len, 0, from, fromlen);
#else
		n = recvfrom(sockfd, buf, len, MSG_DONTWAIT, from, fromlen);
#endif
		if ( n > 0 ) {
			if ( n == len ) break;

			warn("recv(len=%d) returned %d: %s", (int)len, n, strerror(errno));
			return FAIL;
		} else if ( n == 0 ) {
			/* maybe connection error */
			warn("recv() returned 0");
			return FAIL;
		} else {
			/* retry or handler error */
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				warn("recv returned %d: %s", n, strerror(errno));
				return FAIL;
			}

			
			FD_ZERO(&rset);
			FD_SET(sockfd, &rset);
			tval.tv_sec = timeout;
			tval.tv_usec = 0;

			if (timeout==0) n = select(sockfd+1, &rset, NULL, NULL, NULL);
			else			n = select(sockfd+1, &rset, NULL, NULL, &tval);

			if ( n == 0 ) {
				error("recv timedout(timeout = %d)", UDP_TIMEOUT);
				errno = ETIMEDOUT;
				return FAIL;
			} else if ( n == -1 ) {
				error("select: %s", strerror(errno));
				return FAIL;
			} else {
				; /* socket is now readable, so continue the loop */
			}
		}
	} // for ( ; ; )

    if (set_non_blocking(sockfd, FALSE) == FAIL)
	{
		error("cannot unset socket non-blocking: %s", strerror(errno));
		return FAIL;
    }

	return SUCCESS;
}

static int
udp_recv(int sockfd, void *buf, size_t len, int timeout)
{
	return udp_recvfrom(sockfd, buf, len, NULL, NULL, timeout);
}

static int
udp_sendto(int sockfd, const void *buf, size_t len,
		struct sockaddr *to, socklen_t tolen, int timeout)
{
	fd_set wset;
	struct timeval tval;

    if (set_non_blocking(sockfd, TRUE) == FAIL)
	{
		error("cannot set socket non-blocking: %s", strerror(errno));
		return FAIL;
    }

	for ( ; ; ) {
		int n = 0;

#if defined(AIX5)
		n = sendto(sockfd, buf, len, MSG_NONBLOCK, to, tolen);
#elif defined(CYGWIN) || defined(HPUX)
		n = sendto(sockfd, buf, len, 0, to, tolen);
#else
		n = sendto(sockfd, buf, len, MSG_DONTWAIT, to, tolen);
#endif
		if ( n > 0 ) {
			if ( n == len ) break;

			warn("send(len=%d) returned %d: %s", (int)len, n, strerror(errno));
			return FAIL;
		} else if ( n == 0 ) {
			/* maybe connection error */
			warn("send() returned 0");
			return FAIL;
		} else {
			/* retry or handler error */
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				warn("send returned %d: %s", n, strerror(errno));
				return FAIL;
			}
			
			FD_ZERO(&wset);
			FD_SET(sockfd, &wset);
			tval.tv_sec = timeout;
			tval.tv_usec = 0;

			if (timeout==0) n = select(sockfd+1, NULL, &wset, NULL, NULL);
			else			n = select(sockfd+1, NULL, &wset, NULL, &tval);

			if ( n == 0 ) {
				error("send timedout(timeout = %d)", UDP_TIMEOUT);
				errno = ETIMEDOUT;
				return FAIL;
			} else if ( n == -1 ) {
				error("select: %s", strerror(errno));
				return FAIL;
			} else {
				; /* socket is now writable, so continue the loop */
			}
		}
	} // for ( ; ; )

    if (set_non_blocking(sockfd, FALSE) == FAIL)
	{
		error("cannot unset socket non-blocking: %s", strerror(errno));
		return FAIL;
    }

	return SUCCESS;
}

static int
udp_send(int sockfd, const void *buf, size_t len, int timeout)
{
	return udp_sendto(sockfd, buf, len, NULL, 0, timeout);
}


#if !defined(HAVE_GETADDRINFO)
/*
 * AIX 5.x and CYGWIN does not support getaddrinfo.
*/

static int 
udp_bind(int *sockfd, const char *host, const char *port)
{
  sb_assert("udp_bind() is not supported on this platform.");

  return SUCCESS;
}
#else

static int 
udp_bind(int *sockfd, const char *host, const char *port)
{
	int n;
/*	const int on = 1;*/
	struct addrinfo hints, *res;

	memset(&hints, 0x00, sizeof(struct addrinfo));
	if ( host == NULL ) hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ( (n = getaddrinfo(host, port, &hints, &res)) != 0 ) {
		alert("cannot getaddrinfo for %s:%s", host, port);
		if ( n == EAI_SYSTEM )
			error("getaddrinfo: %s - %s", gai_strerror(n), strerror(errno));
		else error("getaddrinfo: %s", gai_strerror(n));
		return FAIL;
	}

	*sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if ( *sockfd == -1 ) {
		error("socket: %s", strerror(errno));
		return FAIL;
	}

	/* useless option for UDP */
	/*
	if ( setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0 ) {
		alert("setsockopt: %s", strerror(errno));
		return FAIL;
	}
	*/

	debug("binding %s:%s", host, port);
	if ( bind(*sockfd, res->ai_addr, res->ai_addrlen) != 0 ) {
		alert("bind[%s:%s] error: %s", host, port, strerror(errno));
		return FAIL;
	}

	freeaddrinfo(res);

	return SUCCESS;
}
#endif // #if !defined(HAVE_GETADDRINFO)

/*****************************************************************************/

static void register_hooks(void)
{
	sb_hook_udp_connect(udp_connect,NULL,NULL,HOOK_MIDDLE);
	sb_hook_udp_send(udp_send,NULL,NULL,HOOK_MIDDLE);
	sb_hook_udp_recv(udp_recv,NULL,NULL,HOOK_MIDDLE);
	sb_hook_udp_bind(udp_bind,NULL,NULL,HOOK_MIDDLE);
	sb_hook_udp_recvfrom(udp_recvfrom,NULL,NULL,HOOK_MIDDLE);
	sb_hook_udp_sendto(udp_sendto,NULL,NULL,HOOK_MIDDLE);
}

module udp_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};


