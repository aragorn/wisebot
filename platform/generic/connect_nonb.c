/* $Id$ */
#include "connect_nonb.h"

int
connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec)
{
	int			flags, n, conn_error;
	socklen_t	len;
	fd_set		rset, wset;
	struct timeval	tval;

	flags = fcntl(sockfd, F_GETFL, 0);
	if ( flags == -1 ) {
		error("fcntl: %s", strerror(errno));
		return FAIL;
	}
	n = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	if ( n == -1 ) {
		error("fcntl: %s", strerror(errno));
		return FAIL;
	}

	conn_error = 0;
	if ( (n = connect(sockfd, saptr, salen)) < 0 )
#ifdef AIX5
		if (!(errno == EINPROGRESS || errno == EEXIST || errno == 17)) {
#else
		if (errno != EINPROGRESS) {
#endif
			error("connect: %s", strerror(errno));
			return FAIL;
		}

	/* connect(2) is under progress */

	if ( n == 0 ) goto done;	/* connect(2) completed immediately */

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ( (n = select(sockfd + 1, &rset, &wset, NULL,
						nsec ? &tval : NULL)) == 0 ) {
		close(sockfd);
		warn("select timedout(timeout = %d)", nsec);
		errno = ETIMEDOUT;
		return FAIL;
	} else if ( n == -1 ) {
		error("select: %s", strerror(errno));
		return FAIL;
	}

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
		len = sizeof(conn_error);
		// refer to connect(2) EINPROGRESS error part
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &conn_error, &len) < 0) {
			error("getsockopt: %s", strerror(errno));
			return FAIL;
		}
	} else {
		error("select: sockfd not set");
		return FAIL;
	}

  done:
	n = fcntl(sockfd, F_SETFL, flags); /* restore file status flags */
	if ( n == -1 ) {
		error("fcntl: %s", strerror(errno));
		return FAIL;
	}

	if (conn_error) {
		close(sockfd);
		errno = conn_error;
		error("%s", strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}


