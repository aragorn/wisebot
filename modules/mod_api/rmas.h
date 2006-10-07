/* $Id$ */
#ifndef RMAS_H
#define RMAS_H 1

#include "morpheme.h"
#include "protocol4.h"


SB_DECLARE_HOOK(int, rmas_merge_index_word_array , \
		(sb4_merge_buffer_t *buf , void *catdata , int num_of_catdata))

SB_DECLARE_HOOK(int, rmas_morphological_analyzer , \
		(int field_id, const char *input, void **output, int *num_of_output, int morpheme_id))


#endif
