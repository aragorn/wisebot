/* $Id$ */
#ifndef _MOD_INDEXER_H_
#define _MOD_INDEXER_H_ 1

#include "softbot.h"
//#include "mod_api/register.h"


#include "hit.h" /* XXX: inv_idx_header_t */

/* XXX: the module which hooks index_one_doc interface should also
 *      implement following functions.
 */
//extern cdm_db_t *m_cdmdb;
extern void fill_dochit
	(doc_hit_t *dochit, DocId did, uint32_t hitnum, word_hit_t *wordhit);


#endif
