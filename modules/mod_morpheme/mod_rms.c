/* $Id$ */
#include "softbot.h"
#include "mod_api/rms.h"
#include "mod_api/index_word_extractor.h"

#define MAX_RMAS_INDEX_WORD_LEN	1024
#define RMS_INITIAL_BUFFER_NUM (MAX_RMAS_INDEX_WORD_LEN*20)

int rma_merge_index_word_array ( sb4_merge_buffer_t *mbuf , void *catdata , int num_of_catdata ) // need to free after this function
{
	if (!mbuf->data)
	{
			
		mbuf->allocated_size = RMS_INITIAL_BUFFER_NUM * sizeof(index_word_t);
		mbuf->data = sb_calloc( 1 , mbuf->allocated_size);
		if (mbuf->data == 0x00)
		{
			error("allocation fail : rma_merge_index_word_array");
			return FAIL;
		}
		mbuf->data_size = 0;
	}

	while ( num_of_catdata * sizeof(index_word_t) >= mbuf->allocated_size - mbuf->data_size )
	{
		mbuf->allocated_size *=2; // doubling

		mbuf->data = sb_realloc(mbuf->data , mbuf->allocated_size);

		if (mbuf->data == 0x00)
		{
			error("allocation fail : rma_merge_index_word_array");
			return FAIL;
		}
		
	}

	memcpy(mbuf->data + mbuf->data_size , catdata , num_of_catdata * sizeof(index_word_t));

	mbuf->data_size += num_of_catdata * sizeof(index_word_t);

	return SUCCESS;
	
}

int rma_morphological_analyzer ( int field_id , char* input , void **output , int *num_of_output , int morpheme_id)
{

	index_word_extractor_t *extractor;
	int n, nret;
	sb4_merge_buffer_t *mbuf=0x00; 
	index_word_t *index_word_array;

	extractor = sb_run_new_index_word_extractor(morpheme_id);
	if ( extractor == NULL || extractor == (index_word_extractor_t*)MINUS_DECLINE ) {
		error( "cannot get extractor: %d, %x", morpheme_id, extractor );
		return FAIL;
	}

	sb_run_index_word_extractor_set_text(extractor, input);

	index_word_array = (index_word_t*)sb_calloc(MAX_RMAS_INDEX_WORD_LEN, sizeof(index_word_t));

	while( ( n = sb_run_get_index_words(extractor, index_word_array ,MAX_RMAS_INDEX_WORD_LEN) ) > 0 )
	{
		nret = rma_merge_index_word_array(mbuf , index_word_array , n);

		if (nret == FAIL)
			return FAIL;

	}

	*output = mbuf->data;

	sb_free(index_word_array);

	sb_run_delete_index_word_extractor(extractor);

	return SUCCESS;

}

static void register_hooks(void)
{
	sb_hook_rma_merge_index_word_array(rma_merge_index_word_array,NULL,NULL,HOOK_MIDDLE);
	sb_hook_rma_morphological_analyzer(rma_morphological_analyzer, NULL, NULL, HOOK_MIDDLE);
}

module morpheme_module = {
    STANDARD_MODULE_STUFF,
    NULL,                 /* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
