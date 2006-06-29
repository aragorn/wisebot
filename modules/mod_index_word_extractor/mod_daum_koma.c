/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "memory.h"

#include "mod_api/index_word_extractor.h"
#include "daum_koma/dha.h"

#include <string.h>

#define MY_EXTRACTOR_ID             (16)
#define DIC_PATH                    "dat/daum_koma"
#define CONF_PATH                    DIC_PATH"/CONFIG.HANL"
#define MAX_OUTPUT  4096

typedef	struct {
	char* text;
	char* next_text;
	char remain_token[MAX_OUTPUT];
	int32_t position;
	void* dha;
} dha_handle_t;

static char dha_config_path[STRING_SIZE] = CONF_PATH;
static index_word_extractor_t *extractor = NULL;

static index_word_extractor_t* create_daum_dha_handler(int id)
{
	dha_handle_t *handle = NULL;

	if (id != MY_EXTRACTOR_ID)
		return (index_word_extractor_t*)MINUS_DECLINE;

	if(extractor == NULL) {
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

		handle->dha = dha_initialize(DIC_PATH, CONF_PATH);
		if (handle == NULL) {
			crit("cannot allocate koma_handle_t object");
			sb_free(extractor);
			return NULL;
		}

		extractor->handle = handle;
		extractor->id = id;
	}

	return extractor;
}

static int daum_dha_set_text(index_word_extractor_t* extractor, char* text)
{
	dha_handle_t *handle = NULL;

	if (extractor->id != MY_EXTRACTOR_ID)
		return DECLINE;

	handle = extractor->handle;

	handle->text = text;
	handle->next_text = text;
	handle->position = 1;
	handle->remain_token[0] = '\0';
	
	return SUCCESS;
}

#define IS_WHITE(c)     ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))
static int daum_dha_analyze(index_word_extractor_t *extractor, index_word_t *index_word, int max)
{
	dha_handle_t *handle=NULL;
	int32_t *pos = NULL;
    char result[MAX_OUTPUT];
    char *word=NULL, *rptr=NULL;
	int index_word_idx = 0;

	if (extractor->id != MY_EXTRACTOR_ID) return MINUS_DECLINE;

	handle = extractor->handle;

	pos = &(handle->position);
	memset(&(index_word[0]), 0x00, sizeof(index_word_t));

	/* 처리를 다 못한 token 처리 */
	for (word=handle->remain_token, rptr=handle->remain_token; index_word_idx<max; index_word_idx++) {
		while (*rptr!=' ' && *rptr!='\0') rptr++;
		if (*rptr == '\0') break;
		*rptr = '\0';

		warn("word[%s]", word);
		strncpy(index_word[index_word_idx].word, strtoupper(word), MAX_WORD_LEN);
		index_word[index_word_idx].word[MAX_WORD_LEN-1] = '\0';
		index_word[index_word_idx].pos = *pos;
		index_word[index_word_idx].len = strlen(index_word[index_word_idx].word);
		
		rptr++;
		word = rptr;

			warn("add word[%s]", word);
		if(index_word_idx +1 >= max && strlen(rptr) > 0) {
			strncpy(handle->remain_token, rptr, MAX_WORD_LEN);
			return index_word_idx+1;
		}
	}

	// 나머지를 다 처리했으면 position 증가
	if(strlen(handle->remain_token) > 0) {
        (*pos)++;
	}

	handle->remain_token[0] = '\0';

	while(*handle->next_text != '\0') { 
	    char* curr_token = handle->next_text;

		if(*curr_token == '\0') break;

		// curr_token을 공백구분의 어절로 만든다.
		while(1) {
			if(IS_WHITE(*handle->next_text)) {
				*handle->next_text = '\0';
				handle->next_text++;
				break;
			} else {
			    handle->next_text++;
		        if(*handle->next_text == '\0') break;
			}
		}

		//warn("curr_token[%s]", curr_token);
		dha_analyze(extractor->handle, NULL, curr_token, MAX_OUTPUT, result);

		/*
		 * DHA의 출력은 공백으로 구분된 색인어 문자열입니다.
		 */
		for (word=result, rptr=result; index_word_idx<max; index_word_idx++) {
			while (*rptr!=' ' && *rptr!='\0') rptr++;
			if (*rptr == '\0') break;
			*rptr = '\0';

			//warn("word[%s]", word);
			strncpy(index_word[index_word_idx].word, strtoupper(word), MAX_WORD_LEN);
			index_word[index_word_idx].word[MAX_WORD_LEN-1] = '\0';
			index_word[index_word_idx].pos = *pos;
			index_word[index_word_idx].len = strlen(index_word[index_word_idx].word);
			
			rptr++;
			word = rptr;

		    if(index_word_idx + 1 >= max && strlen(rptr) > 0) {
				warn("save word[%s], index_word_idx[%d]", rptr, index_word_idx);
				strncpy(handle->remain_token, rptr, MAX_WORD_LEN);
			    return index_word_idx+1;
			}
		}
		
		(*pos)++;
	} // loop 

	handle->remain_token[0] = '\0';
	return index_word_idx;
}

static int destroy_daum_dha_handler(index_word_extractor_t *extractor)
{
//	dha_handle_t *handle=NULL;

	if (extractor->id != MY_EXTRACTOR_ID) return DECLINE;
/* static 하게 사용한다.
	handle = extractor->handle;
	
    dha_finalize(handle->dha);
	sb_free(handle);
	sb_free(extractor);
*/
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
