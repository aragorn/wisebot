/* $Id$ */
#include "softbot.h"
#include "mod_api/indexer.h"
#include "mod_api/docattr.h"

HOOK_STRUCT(
	HOOK_LINK(index_each_spooled_doc)
	HOOK_LINK(index_each_doc)
	HOOK_LINK(index_one_doc)
	HOOK_LINK(last_indexed_did)
	HOOK_LINK(print_forwardidx)

	HOOK_LINK(get_para_position)
	HOOK_LINK(get_position)
	HOOK_LINK(cmp_field)
	HOOK_LINK(get_field)
	HOOK_LINK(get_indexer_socket_file)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,index_each_doc,\
		(void* word_db, uint32_t did, word_hit_t *wordhit, uint32_t hitsize,\
		 uint32_t *hitidx, void *data, int size),\
		(word_db,did,wordhit,hitsize,hitidx,data,size),DECLINE )

SB_IMPLEMENT_HOOK_RUN_FIRST(int,index_one_doc,\
		(uint32_t did,word_hit_t *wordhit,uint32_t hitsize,uint32_t *hitidx),\
		(did,wordhit,hitsize,hitidx),DECLINE )

SB_IMPLEMENT_HOOK_RUN_FIRST(uint32_t,last_indexed_did, (void),(),MINUS_DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,print_forwardidx,\
	(VariableRecordFile *v,DocId did,\
	 char *field, char bprinthits, FILE* stream),\
	(v,did,field,bprinthits,stream),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_para_position,(hit_t *hit), (hit), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(uint32_t,get_position,(hit_t *hit), (hit), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,cmp_field,(hit_t *u, hit_t *v), (u,v), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_field,(hit_t *hit), (hit), MINUS_DECLINE)
//SB_IMPLEMENT_HOOK_RUN_FIRST(char *,get_indexer_port, (void),(),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(const char*,get_indexer_socket_file,(void),(),DECLINE)
