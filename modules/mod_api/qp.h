/* $Id$ */
#ifndef _QP_H_
#define _QP_H_ 1

#include "softbot.h"
#include "mod_api/docattr.h"

typedef struct request_t request_t;
typedef struct filter_t filter_t;
typedef struct rank_t rank_t;
typedef struct index_list_t index_list_t;

SB_DECLARE_HOOK(int, qp_init,(void))
SB_DECLARE_HOOK(int, qp_light_search, (void* word_db, request_t *r))
SB_DECLARE_HOOK(int, qp_full_search, (void* word_db, request_t *r))
SB_DECLARE_HOOK(int, qp_abstract_info, (request_t *r))
SB_DECLARE_HOOK(int, qp_full_info, (request_t *r))
SB_DECLARE_HOOK(int, qp_finalize_search, (request_t *r))

SB_DECLARE_HOOK(int, qp_docattr_query_process, (docattr_cond_t *cond, char *querystring)) // AT
SB_DECLARE_HOOK(int, qp_docattr_query2_process, (docattr_cond_t *cond, char *querystring)) // AT2
SB_DECLARE_HOOK(int, qp_docattr_group_query_process, (docattr_cond_t *cond, char *querystring)) // GR

#if 0
SB_DECLARE_HOOK(int, qp_filter,\
		(index_list_t *s1, index_list_t *s2, index_list_t *d, filter_t *f, \
		 int rank_func(rank_t*,rank_t*,uint32_t*)))
SB_DECLARE_HOOK(int, qp_rank, \
		(rank_t *r1, rank_t *r2, uint32_t *relevancy))
#endif	
#endif
