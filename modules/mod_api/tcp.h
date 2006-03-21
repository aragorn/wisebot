/* $Id$ */
#ifndef TCP_H
#define TCP_H 1

#include <sys/socket.h> /* struct sockaddr */

SB_DECLARE_HOOK(int,tcp_connect,(int *sockfd, char *host, char *port))
SB_DECLARE_HOOK(int,tcp_close,(int sockfd))
SB_DECLARE_HOOK(int,tcp_lingering_close,(int sockfd))
SB_DECLARE_HOOK(int,tcp_bind_listen,
	(const char *host, const char *port, const int backlog, int *listenfd))
SB_DECLARE_HOOK(int,tcp_select_accept,
	(int listenfd, int *sockfd, struct sockaddr *remote_addr, socklen_t *len))
SB_DECLARE_HOOK(int,tcp_recv,(int sockfd, void *data, int len, int timeout))
SB_DECLARE_HOOK(int,tcp_send,(int sockfd, void *data, int len, int timeout))
SB_DECLARE_HOOK(int,tcp_local_connect,(int *sockfd, char *path))
SB_DECLARE_HOOK(int,tcp_local_bind_listen,
	(const char *path, const int backlog, int *listenfd))
SB_DECLARE_HOOK(int,tcp_server_timeout,(void))
SB_DECLARE_HOOK(int,tcp_client_timeout,(void))

#endif
