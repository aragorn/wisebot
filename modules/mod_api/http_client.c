/* $Id$ */
#include "common_core.h"
#include "http_client.h"

HOOK_STRUCT(
	HOOK_LINK(http_client_new)
	HOOK_LINK(http_client_reset)
	HOOK_LINK(http_client_free)
	HOOK_LINK(http_client_conn_close)
	HOOK_LINK(http_client_makeRequest)
	HOOK_LINK(http_client_retrieve)
	HOOK_LINK(http_client_connect)
	HOOK_LINK(http_client_sendRequest)
	HOOK_LINK(http_client_sendRequestMore)
	HOOK_LINK(http_client_recvResponse)
	HOOK_LINK(http_client_parseResponse)
	HOOK_LINK(http_client_flushBuffer)
)

//FIXME : how should i do when return type is ptr? -- eerun
SB_IMPLEMENT_HOOK_RUN_FIRST(http_client_t *, http_client_new, \
					(const char *host, const char *port),  \
					(host, port), NULL)
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(http_client_reset, (http_client_t *httpClt), (httpClt))
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(http_client_free, (http_client_t *httpClt),  \
					(httpClt))
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(http_client_conn_close, (http_client_t *httpClt),  \
					(httpClt))
SB_IMPLEMENT_HOOK_RUN_FIRST(int, http_client_makeRequest, (http_client_t *httpClt, memfile *request),  \
					(httpClt, request), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, http_client_retrieve, \
	(int num_clients, http_client_t **client_list),  \
	(num_clients, client_list), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, http_client_connect, (http_client_t *httpClt),  \
					(httpClt), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, http_client_sendRequest, (http_client_t *httpClt),  \
					(httpClt), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, http_client_sendRequestMore, \
	(http_client_t *this, char *buf, int buflen, int *sentlen),
	(this, buf, buflen, sentlen), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, http_client_recvResponse, (http_client_t *httpClt),  \
					(httpClt), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, http_client_parseResponse, (http_client_t *httpClt),  \
					(httpClt), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, http_client_flushBuffer, (http_client_t *httpClt),  \
					(httpClt), DECLINE)

