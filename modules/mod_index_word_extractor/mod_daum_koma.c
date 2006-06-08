/* $Id$ */
#include "softbot.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/tokenizer.h"
#include "daum_koma/dha.h"

#define MY_EXTRACTOR_ID             (16)
#define DIC_PATH                    "dat/daum_koma"
#define CONF_PATH                    DIC_PATH"/CONFIG.HANL"
#define MAX_OUTPUT  4096

typedef	struct {
	tokenizer_t *tokenizer;
	token_t *token_array;
	int last_token_idx;
	int num_of_tokens;
	int32_t position;
	void* dha;
} dha_handle_t;

static char dha_config_path[STRING_SIZE] = CONF_PATH;

static index_word_extractor_t* create_daum_dha_handler(int id)
{
	index_word_extractor_t *extractor = NULL;
	dha_handle_t *handle=NULL;

	if (id != MY_EXTRACTOR_ID)
		return (index_word_extractor_t*)MINUS_DECLINE;

	extractor = sb_calloc(1, sizeof(index_word_extractor_t));
	if (extractor == NULL) {
		crit("cannot allocate index word extractor object");
		return NULL;
	}

	handle = sb_calloc(1, sizeof(dha_handle_t));
	if (handle == NULL) {
		crit("failed to malloc dha_handle");
		sb_free(extractor);
		return NULL;
	}

	handle->tokenizer = sb_run_new_tokenizer();
	handle->token_array = NULL;

    handle->dha = dha_initialize(DIC_PATH, CONF_PATH);
    if (handle == NULL) {
        crit("cannot allocate koma_handle_t object");
        sb_free(extractor);
        return NULL;
    }

	extractor->handle = handle;
	extractor->id = id;

	if (extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("extractor pointer is -99 (same as the MINUS_DECLINE)");
	}

	return extractor;
}

static int daum_dha_set_text(index_word_extractor_t* extractor, char* text)
{
	dha_handle_t *handle = NULL;

	if (extractor->id != MY_EXTRACTOR_ID)
		return DECLINE;

	handle = extractor->handle;

	sb_run_tokenizer_set_text(handle->tokenizer, text);
	
	return SUCCESS;
}

static int daum_dha_analyze(index_word_extractor_t *extractor, index_word_t *index_word, int max)
{
	dha_handle_t *handle=NULL;
	token_t *token_array = NULL;
	token_t *current_token; // 임시 token 
	int32_t *pos = NULL;
	int32_t index_word_idx = 0; // index_word 구조체의 index.. 
	int32_t num_of_tokens = 0;
	int i=0;
	int count = 0;
    char result[MAX_OUTPUT];
    char *word=NULL, *rptr=NULL;

	if (extractor->id != MY_EXTRACTOR_ID) return MINUS_DECLINE;

	handle = extractor->handle;

	pos = &(handle->position);
	if ( *pos == 0 ) *pos = 1;
	memset(&(index_word[0]), 0x00, sizeof(index_word_t));

	// 추출한 토큰을 다 처리했을 경우, 다음 토큰들을 가져온다.
	if(handle->num_of_tokens <= handle->last_token_idx) {
		handle->token_array = sb_realloc(token_array, max * sizeof(token_t));
		token_array = handle->token_array;
		num_of_tokens = sb_run_get_tokens(handle->tokenizer, token_array, max);
		handle->num_of_tokens = num_of_tokens;
		current_token = &(token_array[0]);
		handle->last_token_idx = 0;
		i = 0;
	} else {
	// 추출한 토큰을 다처리 못한 경우. 나머지 처리.
		token_array = handle->token_array;
		num_of_tokens = handle->num_of_tokens;
		current_token = &(token_array[handle->last_token_idx]);
		i = handle->last_token_idx;
	}
	
	for(; i < num_of_tokens && current_token->type != TOKEN_END_OF_DOCUMENT ; i++) { 

		switch(current_token->type) 
		{ 
			case TOKEN_CHINESE: 
			case TOKEN_NUMBER:         
			case TOKEN_SPECIAL_CHAR : 
			case TOKEN_JAPANESE:
			case TOKEN_KOREAN: 
			case TOKEN_ALPHABET:

				// analyze
				dha_analyze(extractor->handle, NULL, current_token->string, MAX_OUTPUT, result);

				/*
				 * DHA의 출력은 공백으로 구분된 색인어 문자열입니다.
				 */
				for (word=result, rptr=result; count<max; count++) {

					while (*rptr!=' ' && *rptr!='\0') rptr++;
					if (*rptr == '\0') break;
					*rptr = '\0';

	                strncpy(index_word[count].word, strtoupper(word), MAX_WORD_LEN);
 	                index_word[count].word[MAX_WORD_LEN-1] = '\0';
	                index_word[count].pos = *pos;
	                index_word[count].len = strlen(index_word[count].word);
					
					rptr++;
					word = rptr;

					//index_word[index_word_idx].bytepos = current_token->byte_position;
					index_word_idx++;
				}
				
				break; 

			case TOKEN_END_OF_WORD:      // 종결 토큰 
			case TOKEN_END_OF_SENTENCE: 
			case TOKEN_END_OF_PARAGRAPH:
				// word 포지션 증가.
				(*pos)++;
				break; 

		} // switch  

		current_token++;

        /*
         * 토큰이 남아 있는데 index_word가 0이면 다시 가져와 본다.
         */
		if( num_of_tokens > 0 &&
            ((i+1) >= num_of_tokens || current_token->type == TOKEN_END_OF_DOCUMENT) &&
            index_word_idx == 0) {
			handle->token_array = sb_realloc(token_array, max * sizeof(token_t));
			token_array = handle->token_array;
			num_of_tokens = sb_run_get_tokens(handle->tokenizer, token_array, max);
            if(num_of_tokens == 0) { // 그래도 없으면 리턴
                break;
            }

			handle->num_of_tokens = num_of_tokens;
			current_token = &(token_array[0]);
		    handle->last_token_idx = 0;
			i = 0;
        }
	} // loop 

	handle->last_token_idx = i;
	return index_word_idx;
}

static int destroy_daum_dha_handler(index_word_extractor_t *extractor)
{
	dha_handle_t *handle=NULL;

	if (extractor->id != MY_EXTRACTOR_ID) return DECLINE;

	handle = extractor->handle;
	
	sb_run_delete_tokenizer(handle->tokenizer);
	sb_free(handle->token_array);
    dha_finalize(handle->dha);
	sb_free(handle);
	sb_free(extractor);

	return SUCCESS;
}

static void register_hooks(void)
{
    sb_hook_new_index_word_extractor(create_daum_dha_handler, NULL, NULL, HOOK_MIDDLE);
    sb_hook_index_word_extractor_set_text(daum_dha_set_text, NULL, NULL, HOOK_MIDDLE);
    sb_hook_get_index_words(daum_dha_analyze, NULL, NULL, HOOK_MIDDLE); 
    sb_hook_delete_index_word_extractor(destroy_daum_dha_handler, NULL, NULL, HOOK_MIDDLE);
}

static void get_dha_config_file (configValue v)
{
    strncpy(dha_config_path, v.argument[0], 512);
    dha_config_path[511] = '\0';
}

static config_t config[] = {
    CONFIG_GET("DhaConfigFile",get_dha_config_file,1, ""),
    {NULL}
};

module dha_module = {
    STANDARD_MODULE_STUFF,
    config,                 /* config */
    NULL,                   /* registry */
    NULL,                   /* module initialize */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
