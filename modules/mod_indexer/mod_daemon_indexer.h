#ifndef MOD_DAEMON_INDEXER_H
#define MOD_DAEMON_INDEXER_H

#include <stdint.h>

int fill_dochit (doc_hit_t *dochits, int maxdochits,
				 uint32_t docid, word_hit_t *wordhits, uint32_t nhitelm);

#endif
