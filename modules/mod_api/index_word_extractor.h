/* $Id$ */
#ifndef INDEX_WORD_EXTRACTOR_H
#define INDEX_WORD_EXTRACTOR_H 1

#include "auto_config.h"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

// XXX: attribute definition goes here
typedef struct {
	char word[MAX_WORD_LEN];
	uint32_t pos;
	uint32_t bytepos;
	uint32_t attribute;
	uint16_t field;
	uint16_t len;
} index_word_t;

typedef struct {
	int id;
	void *handle;
} index_word_extractor_t;

SB_DECLARE_HOOK(index_word_extractor_t*, new_index_word_extractor, (int id))
SB_DECLARE_HOOK(int, index_word_extractor_set_text, (index_word_extractor_t *extractor, const char *text))
SB_DECLARE_HOOK(int, delete_index_word_extractor, (index_word_extractor_t* extractor))
SB_DECLARE_HOOK(int, get_index_words, (index_word_extractor_t *extractor, index_word_t* index_word, int max))
#endif

