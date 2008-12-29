/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "memory.h"
#include "util.h"
#include "utf8_analyzer.h"

#include "mod_api/index_word_extractor.h"

#include <string.h>

#define MY_EXTRACTOR_ID             (30)
#define MAX_OUTPUT  4096

typedef	struct {
	char* text;
	char* next_text;
	char remain_token[MAX_OUTPUT];
	int32_t position;
	void* unia;
} unia_handle_t;

static index_word_extractor_t *extractor = NULL;

static index_word_extractor_t* create_unia_handler(int id)
{
	unia_handle_t *handle = NULL;

	if (id != MY_EXTRACTOR_ID)
		return (index_word_extractor_t*)MINUS_DECLINE;

	if(extractor == NULL) {
		extractor = sb_calloc(1, sizeof(index_word_extractor_t));
		if (extractor == NULL) {
			crit("cannot allocate index word extractor object");
			return NULL;
		}

		handle = sb_calloc(1, sizeof(unia_handle_t));
		if (handle == NULL) {
			crit("failed to malloc dha_handle");
			sb_free(extractor);
			return NULL;
		}

		handle->unia = utf8_initialize();
		if (handle == NULL) {
			crit("failed to malloc unia_handle");
			sb_free(extractor);
			return NULL;
		}

		extractor->handle = handle;
		extractor->id = id;
	}

	return extractor;
}

static int unia_set_text(index_word_extractor_t* extractor, const char* text)
{
	unia_handle_t *handle = NULL;

	if (extractor->id != MY_EXTRACTOR_ID)
		return DECLINE;

	handle = extractor->handle;

	handle->text = sb_trim(text);
	handle->next_text = sb_trim(text);
	handle->position = 1;
	handle->remain_token[0] = '\0';
	
	return SUCCESS;
}

#define IS_WHITE(c)     ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))
static int unia_analyze(index_word_extractor_t *extractor, index_word_t *index_word, int max)
{
	unia_handle_t *handle=NULL;
	int32_t *pos = NULL;
    char result[MAX_OUTPUT];
    char *s=NULL, *e=NULL;
	int index_word_idx = 0;

	if (extractor->id != MY_EXTRACTOR_ID) return MINUS_DECLINE;

	handle = extractor->handle;

	pos = &(handle->position);
	memset(&(index_word[0]), 0x00, sizeof(index_word_t));

	/* 처리를 다 못한 token 처리 */
	for (s=handle->remain_token; index_word_idx<max; ) {
		e = strchr(s, ' ');
		if(e == NULL && strlen(s) == 0) break;

		if(e == NULL) {
			// 마지막 word도 처리하기 위해.
		} else {
			*e = '\0';
		}

		if(strlen(s) == 0) {
			if(e == NULL) break;

			s = e+1;
			continue;
		}

		//warn("add word[%s]", s);
		strncpy(index_word[index_word_idx].word, strtoupper(s), MAX_WORD_LEN);
		index_word[index_word_idx].word[MAX_WORD_LEN-1] = '\0';
		index_word[index_word_idx].pos = *pos;
		index_word[index_word_idx].len = strlen(index_word[index_word_idx].word);
		index_word_idx++;
		
		if(e == NULL) break;
		s = e+1;

		//warn("add word[%s]", s);
		if(index_word_idx +1 >= max && strlen(s) > 0) {
			strncpy(handle->remain_token, s, MAX_WORD_LEN);
			return index_word_idx;
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

		if(strlen(curr_token) == 0) continue;

		//warn("curr_token[%s]", curr_token);
		utf8_analyze(handle->unia, curr_token, result, MAX_OUTPUT-1);

		//warn("result[%s], index_word_idx[%d], max[%d], [%s]", result, index_word_idx, max, e);

		/*
		 * 출력은 공백으로 구분된 색인어 문자열입니다.
		 */
		for (s=result; index_word_idx<max;) {
			e = strchr(s, ' ');
			if(e == NULL && strlen(s) == 0) break;

			if(e == NULL) {
				// 마지막 word도 처리하기 위해.
			} else {
				*e = '\0';
			}

			//warn("word[%s]", s);

            if(strlen(s) == 0) {
				if(e == NULL) break;

				s = e+1;
                continue;
            }

			//warn("add word[%s]", s);
			strncpy(index_word[index_word_idx].word, strtoupper(s), MAX_WORD_LEN);
			index_word[index_word_idx].word[MAX_WORD_LEN-1] = '\0';
			index_word[index_word_idx].pos = *pos;
			index_word[index_word_idx].len = strlen(index_word[index_word_idx].word);
			index_word_idx++;
			
			if(e == NULL) break;
			s = e+1;

		    if(index_word_idx + 1 >= max) {
                if(strlen(s) > 0) {
					//warn("save word[%s], index_word_idx[%d]", s, index_word_idx);
					strncpy(handle->remain_token, s, MAX_WORD_LEN);
                }
			    return index_word_idx;
            }
		}
		
		(*pos)++;
	} // loop 

	handle->remain_token[0] = '\0';
	return index_word_idx;
}

static int destroy_unia_handler(index_word_extractor_t *extractor)
{
//	unia_handle_t *handle = NULL;

	if (extractor->id != MY_EXTRACTOR_ID) return DECLINE;

/*
	handle = extractor->handle;
	
    utf8_destroy(handle->unia);

	sb_free(handle);
	sb_free(extractor);

*/
	return SUCCESS;
}

static void register_hooks(void)
{
    sb_hook_new_index_word_extractor(create_unia_handler, NULL, NULL, HOOK_MIDDLE);
    sb_hook_index_word_extractor_set_text(unia_set_text, NULL, NULL, HOOK_MIDDLE);
    sb_hook_get_index_words(unia_analyze, NULL, NULL, HOOK_MIDDLE); 
    sb_hook_delete_index_word_extractor(destroy_unia_handler, NULL, NULL, HOOK_MIDDLE);
}

static config_t config[] = {
    {NULL}
};

module utf8_module = {
    STANDARD_MODULE_STUFF,
    config,                 /* config */
    NULL,                   /* registry */
    NULL,                   /* module initialize */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
