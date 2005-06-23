/* $Id$ */
#include "softbot.h"
#include "mod_bigram_truncation_search_support.h"
#include "mod_api/index_word_extractor.h"

#define MY_EXTRACTOR_ID		21

index_word_extractor_t *new_bigram_generator2(int id)
{
	index_word_extractor_t *extractor = NULL;
	bigram_wrapper_t *bigram_wrapper = NULL;

	if (id != MY_EXTRACTOR_ID)
		return (index_word_extractor_t*)MINUS_DECLINE;

	extractor = sb_calloc(1, sizeof(index_word_extractor_t));
	if (extractor == NULL) {
		crit("cannot allocate index word extractor object");
		return NULL;
	}

	bigram_wrapper = sb_calloc(1, sizeof(bigram_wrapper_t));
	if (bigram_wrapper == NULL) {
		crit("cannot allocate bigram_wrapper object");
		sb_free(extractor);
		return NULL;
	}
	extractor->handle = bigram_wrapper;
	extractor->id = id;

	bigram_wrapper->bigram = new_bigram();
	if (bigram_wrapper->bigram == NULL) {
		crit("cannot allocate bigram object in wrapper");
		sb_free(bigram_wrapper);
		sb_free(extractor);
		return NULL;
	}

	if (extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("extractor pointer is %d (same as the decline value for this api",
								MINUS_DECLINE);
	}
	
	return extractor;
}

static int bigram_set_text2(index_word_extractor_t* extractor, char* text)
{
	bigram_wrapper_t *wrapper=NULL;
	if (extractor->id != MY_EXTRACTOR_ID)
		return DECLINE;

	wrapper = extractor->handle;
	bigram_set_text(wrapper->bigram, text);

	wrapper->last_position = 0;
	wrapper->end_of_extraction = 0;

	wrapper->last_index_word.word[0] = '\0';
	wrapper->last_index_word.len = 0;
	wrapper->last_index_word.pos = 0;
	wrapper->last_index_word.field = 0;
	wrapper->last_index_word.attribute = 0;

	return SUCCESS;
}

static int delete_bigram_generator2(index_word_extractor_t* extractor)
{
	bigram_wrapper_t *wrapper=NULL;

	if (extractor->id != MY_EXTRACTOR_ID)
		return DECLINE;

	wrapper=extractor->handle;
	delete_bigram_generator(wrapper->bigram);

	sb_free(wrapper);
	sb_free(extractor);

	return SUCCESS;
}

#define IS_KOREAN(c)	((c >= 0xb0) && (c <= 0xc8) )   // XXX: customizing for shortening number of bigram index word


static void bigram_word_copy(char *dest, char *src, int max, int pos) // XXX: pos가 0이면 전방, pos가 2이면 후방이다.
{
	if(IS_KOREAN((unsigned char)src[0]))
	{
		dest[0]=src[pos];
		dest[1]=src[pos+1];
		dest[2]='\0';
	}
	else	
		strncpy(dest, src, max);

	dest[max-1]='\0';
}


static int bigram_generator2(index_word_extractor_t *extractor, 
						index_word_t index_word[], int32_t max_index_word)
{
	int ret, idx, i, index_word_len;
	bigram_wrapper_t *wrapper=NULL;
	char tmp_string[MAX_WORD_LEN];

	if (extractor->id != MY_EXTRACTOR_ID)
		return MINUS_DECLINE;

	if (max_index_word <= 1) {
		crit("increase max_index_word, and recompile");
		return FAIL;
	}

	index_word_len = max_index_word / 2;
	wrapper = extractor->handle;
	ret = bigram_generator(wrapper->bigram, index_word, index_word_len);
	if (ret == FAIL) {
		error("bigram_generator returned FAIL");
		return FAIL;
	}

	/* add truncation search support routine */
	wrapper = extractor->handle;
	if ( ret == 0 && wrapper->last_index_word.len == 0 )
		wrapper->end_of_extraction = 1;

	if (ret == 0 && wrapper->end_of_extraction) {
		return 0;
	}
	else if (ret == 0 && ! wrapper->end_of_extraction ) {

		if ( wrapper->last_index_word.len > 2 )
			bigram_word_copy(tmp_string, wrapper->last_index_word.word, MAX_WORD_LEN, 2);
		else bigram_word_copy(tmp_string, wrapper->last_index_word.word, MAX_WORD_LEN, 0);

		snprintf(index_word[0].word, MAX_WORD_LEN, "%s%s", tmp_string, "\\>");
		index_word[0].pos = wrapper->last_index_word.pos;
		index_word[0].len = strlen(index_word[0].word);
		index_word[0].attribute =  wrapper->last_index_word.attribute;
		index_word[0].field = wrapper->last_index_word.field;

		wrapper->end_of_extraction = 1;
		return 1;
	}

	idx=ret;
	
	if (ret >= 1 && index_word[0].pos != wrapper->last_position) {

		bigram_word_copy(tmp_string , wrapper->last_index_word.word, MAX_WORD_LEN, 2);

		snprintf(index_word[idx].word, MAX_WORD_LEN, "%s%s", tmp_string, "\\>");
		index_word[idx].pos = wrapper->last_index_word.pos;
		index_word[idx].len = strlen(index_word[idx].word);
		index_word[idx].attribute =  wrapper->last_index_word.attribute;
		index_word[idx].field = wrapper->last_index_word.field;
	}

	for (i=0; i<ret && idx < max_index_word; i++) {
		if (wrapper->last_position != index_word[i].pos) {
			
			bigram_word_copy(tmp_string , index_word[i].word, MAX_WORD_LEN, 0);

			snprintf(index_word[idx].word, MAX_WORD_LEN, "%s%s", "\\<", tmp_string);

			index_word[idx].pos = index_word[i].pos;
			index_word[idx].len = strlen(index_word[i].word);
			index_word[idx].field = index_word[i].field;
			index_word[idx].attribute = index_word[i].attribute;
			
			idx++;
			if (idx == max_index_word) {
				break;
			}
		}

		if ( i+1 < ret && index_word[i+1].pos != index_word[i].pos ) {

			if ( index_word[i].len > 2 )
				bigram_word_copy(tmp_string, index_word[i].word, MAX_WORD_LEN, 2);
			else bigram_word_copy(tmp_string, index_word[i].word, MAX_WORD_LEN, 0);

			snprintf(index_word[idx].word, MAX_WORD_LEN, "%s%s", tmp_string, "\\>");

			index_word[idx].pos = index_word[i].pos;
			index_word[idx].len = strlen(index_word[i].word);
			index_word[idx].field = index_word[i].field;
			index_word[idx].attribute = index_word[i].attribute;

			idx++;
			if (idx == max_index_word) {
				break;
			}
		}

		wrapper->last_position = index_word[i].pos;
		wrapper->last_index_word = index_word[i];
	}

	return idx;
}

static void register_hooks(void)
{
	sb_hook_new_index_word_extractor(new_bigram_generator2,NULL,NULL,HOOK_MIDDLE);
	sb_hook_index_word_extractor_set_text(bigram_set_text2,NULL,NULL,HOOK_MIDDLE);
	sb_hook_delete_index_word_extractor(delete_bigram_generator2,NULL,NULL,HOOK_MIDDLE);
	sb_hook_get_index_words(bigram_generator2,NULL,NULL,HOOK_MIDDLE);
}

module bigram_truncation_search_support_module = {
    STANDARD_MODULE_STUFF,
    NULL, 	                /* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
