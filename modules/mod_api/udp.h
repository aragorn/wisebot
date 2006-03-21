/* $Id$ */
#ifndef UDP_H
#define UDP_H 1

#include <sys/socket.h> /* struct sockaddr */

#define UDP_TIMEOUT (3)

SB_DECLARE_HOOK(int,udp_connect,(int *sockfd, const char *host, const char *port))
SB_DECLARE_HOOK(int,udp_bind,(int *sockfd, const char *host, const char *port))

SB_DECLARE_HOOK(int,udp_recvfrom,(int sockfd, void *buf, size_t len,\
		 struct sockaddr *from, socklen_t *fromlen, int timeout))
SB_DECLARE_HOOK(int,udp_recv,(int sockfd, void *buf, size_t len, int timeout))
SB_DECLARE_HOOK(int,udp_sendto,(int sockfd, const void *buf, size_t len,\
		 struct sockaddr *to, socklen_t tolen, int timeout))
SB_DECLARE_HOOK(int,udp_send,(int sockfd, const void *buf, size_t len, int timeout))

#endif
