/* $Id$ */
#include "softbot.h"
#include "mod_api/qp.h"

HOOK_STRUCT(
	HOOK_LINK(qp_init)
	HOOK_LINK(qp_full_search)
	HOOK_LINK(qp_light_search)
	HOOK_LINK(qp_abstract_info)
	HOOK_LINK(qp_full_info)
	HOOK_LINK(qp_finalize_search)

	HOOK_LINK(qp_docattr_query_process)

	// XXX: qp 내에서 쓰임. 여기서 선언??.. --jiwon
	HOOK_LINK(qp_filter)
	HOOK_LINK(qp_rank)
)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_init, (void), (), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_full_search, (void* word_db, request_t *r), (word_db,r), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_light_search, (void* word_db, request_t *r), (word_db,r), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_abstract_info, (request_t *r), (r), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_full_info, (request_t *r), (r), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_finalize_search, (request_t *r), (r), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_docattr_query_process, \
	(docattr_cond_t *cond, char *querystring), (cond, querystring), DECLINE)

#if 0
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_filter,  \
		(index_list_t *s1,index_list_t *s2,index_list_t *d,filter_t *f,\
		 int rank_func(rank_t*,rank_t*,uint32_t*)), \
		(s1,s2,d,f,rank_func), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, qp_rank, \
		(rank_t *r1,rank_t *r2,uint32_t *relevancy), \
		(r1,r2,relevancy), DECLINE)
#endif	
