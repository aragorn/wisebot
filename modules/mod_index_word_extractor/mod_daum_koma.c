#include "softbot.h"
#include "mod_api/index_word_extractor.h"
#include "daum_koma/dha.h"
#include "common_util.h"

#define MY_EXTRACTOR_ID				(16)
#define DIC_PATH       				"dat/daum_koma"
#define CONF_PATH                    DIC_PATH"/CONFIG.HANL"
#define MAX_OUTPUT  4096

static char dha_config_path[512]=CONF_PATH;
static int cur_pos = 0;
static char *cur_text = NULL;

static index_word_extractor_t* create_daum_dha_handler(int id)
{
	index_word_extractor_t *extractor = NULL;
	void *handle;

	if (id != MY_EXTRACTOR_ID)
		return (index_word_extractor_t*)MINUS_DECLINE;

	extractor = sb_calloc(1, sizeof(index_word_extractor_t));
	if (extractor == NULL) {
		crit("cannot allocate index word extractor object");
		return NULL;
	}

	// initialize
	handle = dha_initialize(DIC_PATH, CONF_PATH);
	if (handle == NULL) {
		crit("cannot allocate koma_handle_t object");
		sb_free(extractor);
		return NULL;
	}

	extractor->handle = handle;
	extractor->id = id;

	return extractor;
}

static int daum_dha_set_text(index_word_extractor_t* extractor, char* text)
{
	if (extractor->id != MY_EXTRACTOR_ID)
		return MINUS_DECLINE;

	cur_text = text;
	cur_pos = 0;
	return SUCCESS;
}

#define is_whitespace(c)		((c) == ' ' || (c) == '\n' || (c) == '\t')
static int daum_dha_analyze(index_word_extractor_t *extractor, index_word_t indexwords[], int32_t max)
{
	char result[MAX_OUTPUT];
	char *ptr=NULL, *word=NULL, *rptr=NULL;
	int count = 0;

	if (extractor->id != MY_EXTRACTOR_ID)
		return MINUS_DECLINE;

	/*
	 * DHA의 입력은 어절(띄어쓰기) 단위입니다.
	 */

	// skipping whitespace
	while (is_whitespace(*cur_text) && *cur_text != '\0') cur_text++;
	ptr = cur_text;

	if (*ptr == '\0') return 0;

	// find next whitespace
	while (!is_whitespace(*ptr) && *ptr != '\0') ptr++;
	*ptr = '\0';

	// analyze
	dha_analyze(extractor->handle, NULL, cur_text, MAX_OUTPUT, result);

	/*
	 * DHA의 출력은 공백으로 구분된 색인어 문자열입니다.
	 */
	for (word=result, rptr=result, count=0; count<max; ) {

		while (*rptr!=' ' && *rptr!='\0') rptr++;
		if (*rptr == '\0') break;
		*rptr = '\0';

		strncpy(indexwords[count].word, strtoupper(word), MAX_WORD_LEN);
		indexwords[count].word[MAX_WORD_LEN-1] = '\0';
		indexwords[count].pos = cur_pos;
		indexwords[count].len = strlen(word);

		rptr++;
		word = rptr;
		count++;
	}

	cur_text = ++ptr;
	cur_pos++;
	return count;
}

static int destroy_daum_dha_handler(index_word_extractor_t *extractor)
{
	if (extractor->id != MY_EXTRACTOR_ID)
		return MINUS_DECLINE;

	dha_finalize(extractor->handle);
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

