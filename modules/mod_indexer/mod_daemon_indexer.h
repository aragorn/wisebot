#ifndef MOD_DAEMON_INDEXER_H
#define MOD_DAEMON_INDEXER_H

#include "auto_config.h"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

int fill_dochit (doc_hit_t *dochits, int maxdochits,
				 uint32_t docid, word_hit_t *wordhits, uint32_t nhitelm);

#endif
