/* $Id$ */
#include "common_core.h"
#include "memory.h"

#include "mod_api/rmas.h"
#include "mod_api/index_word_extractor.h"

#include <errno.h>
#include <string.h>

#define MAX_RMAS_INDEX_WORD_LEN	1024
#define RMS_INITIAL_BUFFER_NUM (MAX_RMAS_INDEX_WORD_LEN*10)

static int max_number_of_index_words = 400000;

int rmas_merge_index_word_array ( sb4_merge_buffer_t *mbuf , void *catdata , int catdata_size ) // need to free after this function
{
	int times = 1;
	int32_t needed_size=0;

	if (mbuf->data == NULL) {
		mbuf->allocated_size = RMS_INITIAL_BUFFER_NUM * sizeof(index_word_t);
		mbuf->data = sb_malloc(mbuf->allocated_size);
		if (mbuf->data == NULL) {
			error("allocation failed: %s", strerror(errno));
			return FAIL;
		}
		mbuf->data_size = 0;
	}

	needed_size = catdata_size + mbuf->data_size;

	if ( needed_size > mbuf->allocated_size ) {
		times = needed_size / mbuf->allocated_size + 1;

		mbuf->allocated_size *= times; // doubling

// FIXME if debugged clear me
//		INFO("mbuf->allocated_size[%d], times[%d], neededsize[%d]",
//							mbuf->allocated_size, times, needed_size);
//		CRIT("reallicating: %d", mbuf->allocated_size);
		mbuf->data = sb_realloc(mbuf->data , mbuf->allocated_size);

		if (mbuf->data == NULL) {
			error("allocation failed: %s", strerror(errno));
			return FAIL;
		}
	}

	memcpy(mbuf->data + mbuf->data_size , catdata , catdata_size);

	mbuf->data_size += catdata_size;
	debug("mbuf->datasize[%d]", mbuf->data_size);

	return SUCCESS;
}

int rmas_morphological_analyzer(int field_id , const char* input , void **output , int *output_size , int morpheme_id)
{
	index_word_extractor_t *extractor=NULL;
	index_word_t *index_word_array=NULL;
	sb4_merge_buffer_t buf;  //XXX: buf itself in stack, but variables in buf is in heap
	int n, nret, i;

	buf.data = NULL;
	buf.data_size = 0;
	buf.allocated_size = 0;
	*output_size = 0;

	extractor = sb_run_new_index_word_extractor(morpheme_id);
	if (extractor == NULL || extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		error("cannot create index_word_extractor, id:%d", morpheme_id);
		return FAIL;
	}

	sb_run_index_word_extractor_set_text(extractor, input);

	index_word_array = 
		(index_word_t*)sb_calloc(MAX_RMAS_INDEX_WORD_LEN, sizeof(index_word_t));
	if (index_word_array == NULL) {
		error("cannot allocate memory..");
		sb_run_delete_index_word_extractor(extractor);
		return FAIL;
	}

	INFO("before sb_run_get_index_words() loop...");
	while( ( n = sb_run_get_index_words(extractor, 
					index_word_array, MAX_RMAS_INDEX_WORD_LEN) ) > 0 )
	{
		INFO("sb_run_get_index_words() returned %d", n);
	
		// XXX : should be deleted ..
		for(i=0; i < n ; i++)
			index_word_array[i].field = field_id;

		nret = rmas_merge_index_word_array(&buf, 
										   index_word_array,
										   n * sizeof(index_word_t));

		INFO("rmas_merge_index_word_array() returned %d", nret);

		if (nret == FAIL) {
			error("merge index word array fail");
			sb_free(index_word_array);
			sb_run_delete_index_word_extractor(extractor);
			return FAIL;
		}

		if ( buf.data_size >
				max_number_of_index_words * sizeof(index_word_t) ) {
			DEBUG("buf.data_size[%d] > max_words[%d] * sizeof(index_word_t)[%d]",
				buf.data_size, max_number_of_index_words,
				(int)sizeof(index_word_t));

		/* we won't send more than max_number_of_index_words for each field */
			break;
		}	
	}
	INFO("done sb_run_get_index_words() loop, output_size = %d", buf.data_size);


	*output = buf.data;
	*output_size = buf.data_size;

	
	sb_free(index_word_array);
	sb_run_delete_index_word_extractor(extractor);

	return SUCCESS;

}

static void get_max_index_word(configValue v)
{
	max_number_of_index_words = atoi(v.argument[0]);
}

static config_t config[] = {
    CONFIG_GET("MaxIndexWord",get_max_index_word,1, \
               "max number of index word list for each field"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_rmas_merge_index_word_array(rmas_merge_index_word_array,NULL,NULL,HOOK_MIDDLE);
	sb_hook_rmas_morphological_analyzer(rmas_morphological_analyzer, NULL, NULL, HOOK_MIDDLE);
}

module rmas_module = {
    STANDARD_MODULE_STUFF,
    config,                 /* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
