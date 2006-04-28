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
	HOOK_LINK(qp_do_filter_operate)
	HOOK_LINK(qp_finalize_search)

	HOOK_LINK(qp_cb_orderby_virtual_document)
	HOOK_LINK(qp_cb_where_virtual_document)
	HOOK_LINK(qp_set_where_expression)
)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_init, (void), (), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_init_request, (request_t * req, char* query), (req, query), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_init_response, (response_t * res), (res), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_light_search, (request_t *req, response_t *res), (req,res), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_full_search, (request_t *req, response_t *res), (req,res), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_abstract_search, (request_t *req, response_t *res), (req,res), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_do_filter_operate, \
		(request_t *req, response_t *res, enum doc_type type), (req,res,type), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_finalize_search, (request_t *req, response_t *res), (req,res), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_cb_orderby_virtual_document, \
	(const void *dest, const void *sour, void *userdata), \
	(dest, sour, userdata), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_cb_where_virtual_document, \
	(const void *dest), (dest), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_set_where_expression, \
	(char *clause), (clause), DECLINE)
