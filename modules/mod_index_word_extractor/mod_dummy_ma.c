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
	int32_t position;
} dummy_handle_t;

/* obsolete code */
/*
static void
add_index_word (index_word_t *index_word, token_t *token, int32_t pos)
{
	index_word->pos = pos;
	index_word->len = token->len;
	strncpy(index_word->word, token->string, MAX_WORD_LEN);

	return;
}
*/

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

static index_word_extractor_t* new_dummy_analyzer(int id)
{
	index_word_extractor_t *extractor = NULL;
	dummy_handle_t *handle=NULL;

	if (id != 0)
		return (index_word_extractor_t*)MINUS_DECLINE;

	extractor = sb_calloc(1, sizeof(index_word_extractor_t));
	if (extractor == NULL) {
		crit("cannot allocate index word extractor object");
		return NULL;
	}

	handle = sb_calloc(1, sizeof(dummy_handle_t));
	if (handle == NULL) {
		crit("failed to malloc dummy_handle");
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

static int dummy_set_text(index_word_extractor_t* extractor, const char* text)
{
	dummy_handle_t *handle = NULL;

	if (extractor->id != 0)
		return DECLINE;

	handle = extractor->handle;

	sb_run_tokenizer_set_text(handle->tokenizer, text);
	
	return SUCCESS;
}

static int dummy_analyze(index_word_extractor_t *extractor, index_word_t *index_word, int max)
{
	dummy_handle_t *handle=NULL;
	token_t *token_array = NULL;
	token_t *current_token; // 임시 token 
	int32_t *pos = NULL;
	int32_t index_word_idx = 0; // index_word 구조체의 index.. 
	int32_t num_of_tokens = 0;
	/* avoid the first whitespace index_word */
	int prev_token_is_whitespace = 1;
	int i=0;

	if (extractor->id != 0) return MINUS_DECLINE;

	handle = extractor->handle;

	pos = &(handle->position);
	if ( *pos == 0 ) *pos = 1;
	memset(&(index_word[0]), 0x00, sizeof(index_word_t));

	token_array = handle->token_array;
	handle->token_array = sb_realloc(token_array, max * sizeof(token_t));

	token_array = handle->token_array;

	num_of_tokens = sb_run_get_tokens(handle->tokenizer, token_array, max);

	current_token = &(token_array[0]);
	
	for(i=0; i < num_of_tokens && current_token->type != TOKEN_END_OF_DOCUMENT ; i++) { 

		switch(current_token->type) 
		{ 
			case TOKEN_CHINESE: 

				CDconvChn2Kor(current_token->string , current_token->string);

				// falls through
			case TOKEN_NUMBER:         
			case TOKEN_SPECIAL_CHAR : 
			case TOKEN_JAPANESE:
			case TOKEN_KOREAN: 
			case TOKEN_ALPHABET:

				strntoupper(current_token->string, MAX_WORD_LEN);
				append_index_word( &(index_word[index_word_idx]),
									current_token, *pos); 
				
				if (prev_token_is_whitespace)
					index_word[index_word_idx].bytepos = current_token->byte_position;

				prev_token_is_whitespace = 0;
			
				break; 

			case TOKEN_END_OF_WORD:      // 종결 토큰 
			case TOKEN_END_OF_SENTENCE: 
			case TOKEN_END_OF_PARAGRAPH:
				if (! prev_token_is_whitespace) {
					(*pos)++;     // 종결 토큰이 나타나면 포지션 증가 
					(index_word_idx++);
					if ( index_word_idx < max-1 )
						memset(&(index_word[index_word_idx]), 0x00, sizeof(index_word_t));
					prev_token_is_whitespace = 1;
				}

				break; 

		} // switch  

		current_token++;

	} // loop 
	if ( prev_token_is_whitespace == 0 )
		index_word_idx++;
		/* 텍스트의 마지막이 whitespace로 끝나지 않으면 index_word count가
		 * 제대로 되지 않는다.
		 */

	return index_word_idx;
}

static int delete_dummy_analyzer(index_word_extractor_t *extractor)
{
	dummy_handle_t *handle=NULL;

	if (extractor->id != 0) return DECLINE;

	handle = extractor->handle;
	
	sb_run_delete_tokenizer(handle->tokenizer);
	sb_free(handle->token_array);
	sb_free(handle);
	sb_free(extractor);

	return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_new_index_word_extractor(new_dummy_analyzer, NULL, NULL, HOOK_MIDDLE);
	sb_hook_index_word_extractor_set_text(dummy_set_text, NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_index_words(dummy_analyze, NULL, NULL, HOOK_MIDDLE);
	sb_hook_delete_index_word_extractor(delete_dummy_analyzer, NULL, NULL, HOOK_MIDDLE);
}

module dummy_ma_module = {
    STANDARD_MODULE_STUFF,
    NULL, 	                /* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
