/* $Id$ */
#ifndef _RMAS_H_
#define _RMAS_H_ 1

#include "softbot.h"
#include "mod_api/morpheme.h"
#include "mod_api/protocol4.h"


SB_DECLARE_HOOK(int, rmas_merge_index_word_array , \
		(sb4_merge_buffer_t *buf , void *catdata , int num_of_catdata))

SB_DECLARE_HOOK(int, rmas_morphological_analyzer , \
		(int field_id, char *input, void **output, int *num_of_output, int morpheme_id))


#endif
