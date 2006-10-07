/* $Id$ */
#include "common_core.h"
#include "rmas.h"

HOOK_STRUCT(
		HOOK_LINK(rmas_merge_index_word_array)
		HOOK_LINK(rmas_morphological_analyzer)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, rmas_merge_index_word_array, \
	(sb4_merge_buffer_t *mbuf, void *catdata, int num_of_catdata), \
	(mbuf, catdata, num_of_catdata),DECLINE)


SB_IMPLEMENT_HOOK_RUN_FIRST(int, rmas_morphological_analyzer, \
	(int field_id, const char *input, void **output, int *num_of_output, int morpheme_id), \
	(field_id, input, output, num_of_output, morpheme_id), DECLINE)
