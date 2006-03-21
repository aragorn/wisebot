/* $Id$ */
#include "common_core.h"
#include "udp.h"

HOOK_STRUCT(
	HOOK_LINK(udp_connect)
	HOOK_LINK(udp_send)
	HOOK_LINK(udp_recv)
	HOOK_LINK(udp_bind)
	HOOK_LINK(udp_recvfrom)
	HOOK_LINK(udp_sendto)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, udp_connect,\
		(int *sockfd, const char *host, const char *port),\
		(sockfd, host, port), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, udp_bind,\
		(int *sockfd, const char *host, const char *port),\
		(sockfd, host, port),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, udp_recvfrom,\
		(int sockfd, void *buf, size_t len,\
		 struct sockaddr *from, socklen_t *fromlen, int timeout),\
		(sockfd, buf, len, from, fromlen, timeout),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, udp_recv,\
		(int sockfd, void *buf, size_t len, int timeout),\
		(sockfd, buf, len, timeout),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, udp_sendto,\
		(int sockfd, const void *buf, size_t len,\
		 struct sockaddr *to, socklen_t tolen, int timeout),\
		(sockfd, buf, len, to, tolen, timeout),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, udp_send,\
		(int sockfd, const void *buf, size_t len, int timeout),\
		(sockfd, buf, len, timeout),DECLINE)


