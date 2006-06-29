/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "memory.h"

#include "mod_api/tokenizer.h"
#include "mod_api/index_word_extractor.h"

/* XXX: should be removed */
#include "lib/ma_code.h"

typedef	struct {
	tokenizer_t *tokenizer;
	token_t *token_array;
	int32_t done;
} kor2chn_translator_handle_t;

static void
append_index_word (index_word_t *index_word, token_t *token, int32_t pos)
{
	int left;
	index_word->pos = pos;
	left = MAX_WORD_LEN - index_word->len > 0 ? MAX_WORD_LEN - index_word->len : 0;
	strncat(index_word->word, token->string, left);
	if (left == 0) {
		index_word->word[MAX_WORD_LEN-1] = '\0';
	}

	index_word->len += token->len;
	if (index_word->len > MAX_WORD_LEN-1) index_word->len = MAX_WORD_LEN-1;

	return;
}

static index_word_extractor_t* new_kor2chn_translator(int id)
{
	index_word_extractor_t *extractor = NULL;
	kor2chn_translator_handle_t *handle=NULL;

	if (id != 2)
		return (index_word_extractor_t*)MINUS_DECLINE;

	extractor = sb_calloc(1, sizeof(index_word_extractor_t));
	if (extractor == NULL) {
		crit("cannot allocate index word extractor object");
		return NULL;
	}

	handle = sb_calloc(1, sizeof(kor2chn_translator_handle_t));
	if (handle == NULL) {
		crit("failed to malloc kor2chn_translator_handle");
		sb_free(extractor);
		return NULL;
	}

	handle->tokenizer = sb_run_new_tokenizer();
	handle->token_array = NULL;

	extractor->handle = handle;
	extractor->id = id;

	if (extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("extractor pointer is -99 (same as the MINUS_DECLINE)");
	}

	return extractor;
}

static int kor2chn_translator_set_text(index_word_extractor_t* extractor, char* text)
{
	kor2chn_translator_handle_t *handle = NULL;

	if (extractor->id != 2)
		return DECLINE;

	handle = extractor->handle;

	sb_run_tokenizer_set_text(handle->tokenizer, text);

	handle->done = 0;
	
	return SUCCESS;
}

static int kor2chn_translator_analyze(index_word_extractor_t *extractor, index_word_t *index_word, int max)
{
	kor2chn_translator_handle_t *handle=NULL;
	token_t *token_array = NULL;
	int32_t index_word_idx = 0;
	int32_t num_of_tokens = 0;
	int i=0;

	if (extractor->id != 2) return MINUS_DECLINE;

	handle = extractor->handle;

	if (handle->done) return 0;

	memset(&(index_word[0]), 0x00, sizeof(index_word_t));

	token_array = handle->token_array;
	handle->token_array = sb_realloc(token_array, max * sizeof(token_t));

	token_array = handle->token_array;

	num_of_tokens = sb_run_get_tokens(handle->tokenizer, token_array, max);

	for(i=0; i < num_of_tokens && token_array[i].type != TOKEN_END_OF_DOCUMENT ; i++) { 

		switch(token_array[i].type)
		{ 
			case TOKEN_CHINESE: 
				CDconvChn2Kor(token_array[i].string , token_array[i].string);

				// falls through
			case TOKEN_ALPHABET:
				strntoupper(token_array[i].string, MAX_WORD_LEN);

				// falls through
			case TOKEN_NUMBER:         
			case TOKEN_JAPANESE:
			case TOKEN_KOREAN: 
				append_index_word( &(index_word[index_word_idx]),
									&(token_array[i]), 1); 
				if (index_word[index_word_idx].len >= MAX_WORD_LEN-1)
					goto FINISH;
			
				break; 

		} // switch  

	} // loop 

FINISH:
	index_word_idx++;
	handle->done = 1;

	return index_word_idx;
}

static int delete_kor2chn_translator_analyzer(index_word_extractor_t *extractor)
{
	kor2chn_translator_handle_t *handle=NULL;

	if (extractor->id != 2) return DECLINE;

	handle = extractor->handle;
	
	sb_run_delete_tokenizer(handle->tokenizer);
	sb_free(handle->token_array);
	sb_free(handle);
	sb_free(extractor);

	return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_new_index_word_extractor(new_kor2chn_translator, NULL, NULL, HOOK_MIDDLE);
	sb_hook_index_word_extractor_set_text(kor2chn_translator_set_text, NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_index_words(kor2chn_translator_analyze, NULL, NULL, HOOK_MIDDLE);
	sb_hook_delete_index_word_extractor(delete_kor2chn_translator_analyzer, NULL, NULL, HOOK_MIDDLE);
}

module kor2chn_translator_module = {
    STANDARD_MODULE_STUFF,
    NULL, 	                /* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
