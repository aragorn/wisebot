/* $Id$ */
#ifndef _UDP_H_
#define _UDP_H_ 1

#include "softbot.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#define UDP_TIMEOUT (3)

SB_DECLARE_HOOK(int,udp_connect,\
		(int *sockfd, const char *host, const char *port))
SB_DECLARE_HOOK(int,udp_bind,\
		(int *sockfd, const char *host, const char *port))

SB_DECLARE_HOOK(int,udp_recvfrom,\
		(int sockfd, void *buf, size_t len,\
		 struct sockaddr *from, socklen_t *fromlen, int timeout))
SB_DECLARE_HOOK(int,udp_recv,\
		(int sockfd, void *buf, size_t len, int timeout))
SB_DECLARE_HOOK(int,udp_sendto,\
		(int sockfd, const void *buf, size_t len,\
		 struct sockaddr *to, socklen_t tolen, int timeout))
SB_DECLARE_HOOK(int,udp_send,\
		(int sockfd, const void *buf, size_t len, int timeout))
#endif

