/* $Id$ */
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H 1

#include "mod_http_client/mod_http_client.h"

SB_DECLARE_HOOK(http_client_t *, http_client_new, (const char *host, const char *port))
SB_DECLARE_HOOK(void, http_client_reset, (http_client_t *httpClt))
SB_DECLARE_HOOK(void, http_client_free, (http_client_t *httpClt))
SB_DECLARE_HOOK(void, http_client_conn_close, (http_client_t *httpClt))
SB_DECLARE_HOOK(int, http_client_makeRequest, (http_client_t *httpClt, memfile *req))
SB_DECLARE_HOOK(int, http_client_retrieve, (int num_clients, http_client_t **client_list));
SB_DECLARE_HOOK(int, http_client_connect, (http_client_t *httpClt))
SB_DECLARE_HOOK(int, http_client_sendRequest, (http_client_t *httpClt))
SB_DECLARE_HOOK(int, http_client_sendRequestMore,
	(http_client_t *this, char *buf, int buflen, int *sentlen))
SB_DECLARE_HOOK(int, http_client_recvResponse, (http_client_t *httpClt))
SB_DECLARE_HOOK(int, http_client_parseResponse, (http_client_t *httpClt))
SB_DECLARE_HOOK(int, http_client_flushBuffer, (http_client_t *httpClt))
	
#endif //_HTTP_CLIENT_H_
