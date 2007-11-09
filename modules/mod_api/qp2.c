/* $Id$ */
#include "common_core.h"
#include "qp2.h"

HOOK_STRUCT(
	HOOK_LINK(qp_init)
	HOOK_LINK(qp_init_request)
	HOOK_LINK(qp_init_response)
	HOOK_LINK(qp_light_search)
	HOOK_LINK(qp_full_search)
	HOOK_LINK(qp_abstract_search)
	HOOK_LINK(qp_do_filter_operation)
	HOOK_LINK(qp_get_query_string)
	HOOK_LINK(qp_finalize_search)

	HOOK_LINK(qp_cb_orderby_virtual_document)
	HOOK_LINK(qp_cb_orderby_document)
	HOOK_LINK(qp_cb_where)
	HOOK_LINK(qp_set_where_expression)

	HOOK_LINK(qp_get_max_doc_hits_size)
	HOOK_LINK(qp_get_comment_list_size)
)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_init, (void), (), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_init_request, (request_t * req, char* query), (req, query), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_init_response, (response_t * res), (res), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_light_search, (request_t *req, response_t *res), (req,res), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_full_search, (request_t *req, response_t *res), (req,res), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_abstract_search, (request_t *req, response_t *res), (req,res), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_do_filter_operation, \
		(request_t *req, response_t *res, enum doc_type type), (req,res,type), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_get_query_string, \
		(request_t *req, char query[MAX_QUERY_STRING_SIZE]), (req,query), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_finalize_search, (request_t *req, response_t *res), (req,res), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_cb_orderby_virtual_document, \
	(const void *dest, const void *sour, void *userdata), \
	(dest, sour, userdata), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_cb_orderby_document, \
	(const void *dest, const void *sour, void *userdata), \
	(dest, sour, userdata), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_cb_where, \
	(const void *dest), (dest), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_set_where_expression, \
	(char *clause), (clause), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_get_max_doc_hits_size, (int *size), (size), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_get_comment_list_size, (int *size), (size), DECLINE)
