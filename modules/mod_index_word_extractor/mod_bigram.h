/* $Id$ */
#ifndef BIGRAM_H
#define BIGRAM_H 1

#include "mod_api/tokenizer.h"
#include "mod_api/index_word_extractor.h"

#define MAX_TOKENS	32
typedef struct {
	token_t last_token;     // 지난번 토큰

	tokenizer_t *tokenizer;

	token_t last_tokens[MAX_TOKENS];
	int32_t last_token_idx;
	int32_t num_of_tokens;

	int32_t position;
} bigram_t;

extern bigram_t* new_bigram();
extern void bigram_set_text(bigram_t *handle, char* text);
extern void delete_bigram_generator(bigram_t *handle);
extern int bigram_generator(bigram_t *handle, index_word_t index_word[], int32_t max_index_word);

#endif
