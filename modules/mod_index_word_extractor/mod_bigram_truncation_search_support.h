/* $Id$ */
#ifndef BIGRAM_TRUNCATION_SUPPORT_H
#define BIGRAM_TRUNCATION_SUPPORT_H 1

#include "mod_api/index_word_extractor.h"
#include "mod_bigram.h"

typedef struct {
	int32_t last_position;
	index_word_t last_index_word;
	char end_of_extraction;
	bigram_t *bigram;
} bigram_wrapper_t;

#endif
