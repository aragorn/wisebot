/* $Id$ */
#ifndef INDEXER_H
#define INDEXER_H 1

#include "softbot.h"
#include "mod_indexer/hit.h"
#include "mod_api/docattr.h"


#ifndef WORD_HIT_T_DEFINED
	typedef struct word_hit_t word_hit_t;
	#define WORD_HIT_T_DEFINED
#endif

#ifndef VARIABLE_RECORD_FILE_DEFINED
	typedef struct _VariableRecordFile VariableRecordFile;
	#define VARIABLE_RECORD_FILE_DEFINED
#endif

#define EXTHIT(type,hit)	((type*)& ((hit).ext_hit))

#define ERROR_DOCUMENT	 (-101)
#define INDEXER_WAIT	 (-102)
#define DELETED_DOCUMENT (-103)
	
//extern cdm_db_t *m_cdmdb;

SB_DECLARE_HOOK(int,index_each_doc, \
	(void* word_db, uint32_t did,word_hit_t *wordhit,uint32_t hitsize,
	 uint32_t *hitidx, void *data, int size) )

SB_DECLARE_HOOK(int,index_one_doc, \
	(uint32_t did,word_hit_t *wordhit,uint32_t hitsize,uint32_t *hitidx) )
SB_DECLARE_HOOK(uint32_t,last_indexed_did,(void))
SB_DECLARE_HOOK(int, print_forwardidx,\
  (VariableRecordFile *v, DocId did, \
   char *field, char bprinthits, FILE* stream))

SB_DECLARE_HOOK(int,get_para_position,(hit_t *hit))
SB_DECLARE_HOOK(uint32_t,get_position,(hit_t *hit))
SB_DECLARE_HOOK(int,cmp_field,(hit_t *u, hit_t *v))
SB_DECLARE_HOOK(int,get_field,(hit_t *hit))
SB_DECLARE_HOOK(const char*,get_indexer_socket_file,(void))

#endif
