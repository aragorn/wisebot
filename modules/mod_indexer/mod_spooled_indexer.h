/* $Id$ */
#ifndef MOD_SPOOLED_INDEXER_H
#define MOD_SPOOLED_INDEXER_H 1

#include "softbot.h"
#include "mod_api/lexicon.h"
#include "hit.h" 

// XXX: hook?
extern int fill_dochit
	(doc_hit_t *dochits, int maxdochits, uint32_t docid, word_hit_t *wordhits, uint32_t nhitelm);

#endif
