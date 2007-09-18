/* $Id$ */
#ifndef MOD_KOMA_COMPLEX_NOUN_SUPPORT_H
#define MOD_KOMA_COMPLEX_NOUN_SUPPORT_H 1

#include "mod_api/index_word_extractor.h"
#include "mod_koma.h"

typedef struct {
	int id;
	koma_handle_t *koma;

	index_word_t *koma_words;
	int32_t koma_words_size;
	int32_t koma_idx;

	index_word_t *tmp_words;
	int32_t tmp_words_size;
	int32_t tmp_idx;

	int8_t end_of_text;
} koma_complex_noun_support_t;

#endif
