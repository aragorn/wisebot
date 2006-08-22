/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "memory.h"
#include "util.h"

#include "mod_api/index_word_extractor.h"
#include "daum_koma/dha.h"

#include <string.h>

#define MY_EXTRACTOR_ID1             (26)
#define MY_EXTRACTOR_ID2             (27)
#define DIC_PATH                    "dat/daum_koma"
//#define CONF_PATH                    DIC_PATH"/CONFIG.HANL"
#define MAX_OUTPUT  4096

typedef	struct {
	char* text;
	int32_t position;
	// void* dha;  // XXX: 'exact' needs no context
} dha_handle_t;

//static char dha_config_path[STRING_SIZE] = CONF_PATH;
static index_word_extractor_t *extractor1 = NULL;
static index_word_extractor_t *extractor2 = NULL;

static index_word_extractor_t* create_daum_dha_handler(int id)
{
	dha_handle_t *handle = NULL;

	if (id != MY_EXTRACTOR_ID1 && id != MY_EXTRACTOR_ID2)
		return (index_word_extractor_t*)MINUS_DECLINE;

	if (id == MY_EXTRACTOR_ID1) {
		if(extractor1 == NULL) {
			extractor1 = sb_calloc(1, sizeof(index_word_extractor_t));
			if (extractor1 == NULL) {
				crit("cannot allocate index word extractor object");
				return NULL;
			}

			handle = sb_calloc(1, sizeof(dha_handle_t));
			if (handle == NULL) {
				crit("failed to malloc dha_handle");
				sb_free(extractor1);
				return NULL;
			}

			extractor1->handle = handle;
			extractor1->id = id;
		}
		return extractor1;
	}
	else {
		if(extractor2 == NULL) {
			extractor2 = sb_calloc(1, sizeof(index_word_extractor_t));
			if (extractor2 == NULL) {
				crit("cannot allocate index word extractor object");
				return NULL;
			}

			handle = sb_calloc(1, sizeof(dha_handle_t));
			if (handle == NULL) {
				crit("failed to malloc dha_handle");
				sb_free(extractor2);
				return NULL;
			}

			extractor2->handle = handle;
			extractor2->id = id;
		}
		return extractor2;
	}
}

static int daum_dha_set_text(index_word_extractor_t* extractor, char* text)
{
	dha_handle_t *handle = NULL;

	if (extractor->id != MY_EXTRACTOR_ID1 && extractor->id != MY_EXTRACTOR_ID2)
		return DECLINE;

	handle = extractor->handle;
	handle->text = sb_trim(text);
	
	return SUCCESS;
}

#define IS_WHITE(c)     ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))
static int daum_dha_analyze(index_word_extractor_t *extractor, index_word_t *index_word, int max)
{
	dha_handle_t *handle=NULL;
    char result[MAX_OUTPUT];
    char *s=NULL, *e=NULL;
	int index_word_idx = 0, i;

	if (extractor->id != MY_EXTRACTOR_ID1 && extractor->id != MY_EXTRACTOR_ID2)
		return MINUS_DECLINE;

	handle = extractor->handle;

	if (handle->text[0] == '\0') {
		return 0;
	}

	debug("here: %d", extractor->id);
	if (extractor->id == MY_EXTRACTOR_ID1) { // indexing time
		debug("input: %s", handle->text);
		dha_exact_analyze(NULL, NULL, handle->text, MAX_OUTPUT-1, result);
		debug("return: %s", result);

		/*
		 * DHA_EXACT의 출력은 공백으로 구분된 색인어 문자열입니다.
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
			strncpy(index_word[index_word_idx].word, strtoupper(s), MAX_WORD_LEN);
			index_word[index_word_idx].word[MAX_WORD_LEN-1] = '\0';
			index_word[index_word_idx].pos = 0;
			index_word[index_word_idx].len = strlen(index_word[index_word_idx].word);
			index_word_idx++;

			if(e == NULL) break;
			s = e+1;
		}
	}
	else { // search time
		s = handle->text;
		e = result;
		i = 0;
		while (*s != '\0' && i<MAX_OUTPUT-1) {
			if (IS_WHITE(*s) || *s == ')' || *s == '(') {
				s++;
				continue;
			}
			*e = *s;
			s++;
			e++;
			i++;
		}
		*e = '\0';

		strncpy(index_word[index_word_idx].word, strtoupper(result), MAX_WORD_LEN);
		index_word[index_word_idx].word[MAX_WORD_LEN-1] = '\0';
		debug("[%s]", index_word[index_word_idx].word);
		index_word[index_word_idx].pos = 0;
		index_word[index_word_idx].len = strlen(result);
		index_word_idx++;
	}

	handle->text[0] = '\0';
	return index_word_idx;
}

static int destroy_daum_dha_handler(index_word_extractor_t *extractor)
{
	if (extractor->id != MY_EXTRACTOR_ID1 && extractor->id != MY_EXTRACTOR_ID2)
		return DECLINE;
	return SUCCESS;
}

static void register_hooks(void)
{
    sb_hook_new_index_word_extractor(create_daum_dha_handler, NULL, NULL, HOOK_MIDDLE);
    sb_hook_index_word_extractor_set_text(daum_dha_set_text, NULL, NULL, HOOK_MIDDLE);
    sb_hook_get_index_words(daum_dha_analyze, NULL, NULL, HOOK_MIDDLE); 
    sb_hook_delete_index_word_extractor(destroy_daum_dha_handler, NULL, NULL, HOOK_MIDDLE);
}
/*
static void get_dha_config_file (configValue v)
{
    strncpy(dha_config_path, v.argument[0], 512);
    dha_config_path[511] = '\0';
}
*/

static config_t config[] = {
    //CONFIG_GET("DhaConfigFile",get_dha_config_file,1, ""),
    {NULL}
};

module dha_exact_module = {
    STANDARD_MODULE_STUFF,
    config,                 /* config */
    NULL,                   /* registry */
    NULL,                   /* module initialize */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
