/* $Id$ */
#include "common_core.h"
#include "index_word_extractor.h"

HOOK_STRUCT(
	HOOK_LINK(new_index_word_extractor)
	HOOK_LINK(index_word_extractor_set_text)
	HOOK_LINK(delete_index_word_extractor)
	HOOK_LINK(get_index_words)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(index_word_extractor_t*, new_index_word_extractor, (int id), \
							(id), (index_word_extractor_t*)MINUS_DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, index_word_extractor_set_text, \
				(index_word_extractor_t *extractor, const char *text), (extractor,text), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, delete_index_word_extractor,\
				(index_word_extractor_t *extractor), (extractor), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, get_index_words, \
				(index_word_extractor_t *extractor, index_word_t *index_word, int max), \
				(extractor, index_word, max), MINUS_DECLINE)
