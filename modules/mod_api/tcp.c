/* $Id$ */
#include "common_core.h"
#include "tcp.h"

HOOK_STRUCT(
	HOOK_LINK(tcp_connect)
	HOOK_LINK(tcp_close)
	HOOK_LINK(tcp_lingering_close)
	HOOK_LINK(tcp_bind_listen)
	HOOK_LINK(tcp_select_accept)
	HOOK_LINK(tcp_recv)
	HOOK_LINK(tcp_send)
	HOOK_LINK(tcp_local_connect)
	HOOK_LINK(tcp_local_bind_listen)
	HOOK_LINK(tcp_server_timeout)
	HOOK_LINK(tcp_client_timeout)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, tcp_connect,\
	(int *sockfd, char *host, char *port),(sockfd, host, port),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, tcp_close,(int sockfd),(sockfd),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, tcp_lingering_close,
	(int sockfd),(sockfd),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, tcp_bind_listen,\
	(const char *host, const char *port, const int backlog, int *listenfd),\
	(host, port, backlog, listenfd),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, tcp_select_accept,\
	(int listenfd,int *sockfd,struct sockaddr *remote_addr,socklen_t *len),\
	(listenfd, sockfd, remote_addr, len),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,tcp_recv, \
	(int sockfd, void *data, int len, int timeout), \
	(sockfd, data, len, timeout), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,tcp_send, \
	(int sockfd, void *data, int len, int timeout), \
	(sockfd, data, len, timeout), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,tcp_local_connect, \
	(int *sockfd, char *path),(sockfd, path),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,tcp_local_bind_listen, \
	(const char *path, const int backlog, int *listenfd),\
	(path, backlog, listenfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,tcp_server_timeout,(void),(),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,tcp_client_timeout,(void),(),DECLINE)
