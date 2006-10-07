/* $Id$ */
#include "common_core.h"
#include "memory.h"

#include "mod_koma_complex_noun_support.h"
#include "mod_api/index_word_extractor.h"

#include <string.h>

#define MY_EXTRACTOR_ID1		11
#define MY_EXTRACTOR_ID2		12

#define SINGLE_WORD 				// koma_wrapper_analyzer2 , single word handle

static index_word_extractor_t *new_koma_wrapper(int id)
{
	index_word_extractor_t *extractor = NULL;
	koma_complex_noun_support_t *koma_wrapper = NULL;

	if (id != MY_EXTRACTOR_ID1 &&
			id != MY_EXTRACTOR_ID2) 
		return (index_word_extractor_t*)MINUS_DECLINE; //XXX: just DECLINE??

	extractor = sb_calloc(1, sizeof(index_word_extractor_t));
	if (extractor == NULL) {
		crit("cannot allocate index word extractor object");
		return NULL;
	}
	
	koma_wrapper = sb_calloc(1, sizeof(koma_complex_noun_support_t));
	if (koma_wrapper == NULL) {
		crit("cannot allocate koma_wrapper(complex noun support 1) id[%d]", id);
		sb_free(extractor);
		return NULL;
	}
	extractor->handle = koma_wrapper;
	extractor->id = id;

	koma_wrapper->koma = new_koma();
	if (koma_wrapper->koma == NULL) {
		crit("cannot allocate koma object in wrapper");
		sb_free(koma_wrapper);
		sb_free(extractor);
	}

	if (extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("extractor pointer is %d (same as the decline value for this api",
								MINUS_DECLINE);
	}

	return extractor;
}

static int koma_wrapper_set_text(index_word_extractor_t* extractor, const char* text)
{
	koma_complex_noun_support_t* wrapper=NULL;

	if (extractor->id != MY_EXTRACTOR_ID1 &&
			extractor->id != MY_EXTRACTOR_ID2) 
		return DECLINE;

	wrapper = extractor->handle;
	koma_set_text(wrapper->koma, text);
	wrapper->end_of_text = FALSE;
	wrapper->last_position = 0;
	memset(&(wrapper->last_index_word), 0x00, sizeof(index_word_t));

	return SUCCESS;
}

static int delete_koma_wrapper(index_word_extractor_t* extractor)
{
	koma_complex_noun_support_t *wrapper=NULL;

	if (extractor->id != MY_EXTRACTOR_ID1 &&
			extractor->id != MY_EXTRACTOR_ID2) 
		return DECLINE;

	wrapper = extractor->handle;
	delete_koma(wrapper->koma);

	sb_free(wrapper);
	sb_free(extractor);

	return SUCCESS;
}

static void merge_word(index_word_t *result, index_word_t *src1, index_word_t *src2)
{
	if (src1->pos != src2->pos) {
		crit("positions must be the same. pos1:%d, pos2:%d",
				src1->pos, src2->pos);
		return;
	}

	strncpy(result->word, src1->word, MAX_WORD_LEN);
	result->word[MAX_WORD_LEN-1] = '\0';
	strncat(result->word, src2->word, MAX_WORD_LEN - strlen(result->word) - 1);
	result->word[MAX_WORD_LEN-1] = '\0';

	//XXX: how should I set the attribute of merged word? 
	// new tag : COMP -- dyaus 
	result->attribute = 1347243843;	
	result->field = 0;
	result->len = strlen(result->word);
	result->pos = src1->pos;
}

/* ABCD -> A, B, C, D, AB, BC, CD, ABCD */
static int koma_wrapper_analyze1(index_word_extractor_t *extractor,
						index_word_t index_word[], int32_t max_index_word)
{
	int limited_index_word_len=0, rv=0, i=0, idx=0, dist = 0;
	koma_complex_noun_support_t *wrapper=NULL;
	int32_t last_position = 0;
	index_word_t last_index_word;
	index_word_t last_index_full_word;

	if (extractor->id != MY_EXTRACTOR_ID1) 
		return MINUS_DECLINE;

	wrapper = extractor->handle;

	if ( wrapper->end_of_text == TRUE ) 
		return 0;

	/* 1. first, get each lexem A, B, C, D for complex noun, ABCD */
	limited_index_word_len = max_index_word/2; //XXX: description for it goes here..
	rv = koma_analyze(wrapper->koma, index_word, limited_index_word_len);
	if (rv == FAIL) {
		error("koma_analyze returned FAIL");
		return FAIL;
	}

	if (rv == 0) { /* end of text */
		if(wrapper->last_index_full_word_distance>=2) {
			index_word[0] = wrapper->last_index_full_word;
			wrapper->end_of_text = TRUE;
			return 1;
		} else 
			return 0;
	}

	last_position = wrapper->last_position;
	last_index_word = wrapper->last_index_word;
	last_index_full_word = wrapper->last_index_full_word;
	dist = wrapper->last_index_full_word_distance;

	/* complex noun dealing code here.. */
	idx = rv; //XXX: do not overwrite.. (reserve A, B, C, D..)

	/* 2. second, merge each two words, AB, BC, CD .. */
	for (i=0; i<rv; i++) {
		if ( !(TAG_IS_NOUN((char *)&(index_word[i].attribute))
				|| TAG_IS_KYUKJOSA((char *)&(index_word[i].attribute)) ) ) 
			continue;

		if (i==0 && last_position == index_word[i].pos) {
			merge_word( &(index_word[idx]), &last_index_word, &(index_word[i]) );
				idx++;
		}
		else if (i>0 && index_word[i-1].pos == index_word[i].pos) {
			merge_word( &(index_word[idx]), &(index_word[i-1]), &(index_word[i]) );
			idx++;
		}

		if (idx == max_index_word) {
			warn("not enough index_word[] size (%d)", max_index_word);

			wrapper->last_position = index_word[rv-1].pos;
			wrapper->last_index_word = index_word[rv-1];
			wrapper->last_index_full_word = index_word[rv-1];
			wrapper->last_index_full_word_distance = 0;

			return idx;
		}
	}

	last_index_full_word.pos = last_position;

	if(last_position != index_word[0].pos && last_index_full_word.len == 0) {
		last_index_full_word = index_word[0];
	}
	else if(last_position != index_word[0].pos && last_index_full_word.len > 0) {
		if (dist>=2)
			index_word[idx++] = last_index_full_word;

		last_index_full_word = index_word[0];
		dist = 1;
		if (idx >= max_index_word) {
			wrapper->last_position = index_word[rv-1].pos;
			wrapper->last_index_word = index_word[rv-1];
			wrapper->last_index_full_word = last_index_full_word;
			wrapper->last_index_full_word_distance = dist;

			return idx;
		}
	}

	for (i=1; i<rv; i++) {

		if(index_word[i-1].pos == index_word[i].pos) {
			last_index_full_word.pos = index_word[i].pos;
			merge_word( &last_index_full_word, &last_index_full_word , &(index_word[i]) );
			dist++;
		}
		else if(index_word[i-1].pos != index_word[i].pos) {
			if (dist>=2)
				index_word[idx++] = last_index_full_word;
			last_index_full_word = index_word[i];
			dist = 0;
		}

		if (idx >= max_index_word)
			break;
	}

	wrapper->last_position = index_word[rv-1].pos;
	wrapper->last_index_word = index_word[rv-1];
	wrapper->last_index_full_word = last_index_full_word;
	wrapper->last_index_full_word_distance = dist;

	return idx;
}

/* ABCD -> AB, BC, CD */
static int koma_wrapper_analyze2(index_word_extractor_t *extractor,
						index_word_t index_word[], int32_t max_index_word)
{
	int	 	rv=0, i=0, idx=0;
	int 	limited_index_word_len=0;
	int32_t last_position = 0;
	int8_t	is_first_cut;
	int8_t  flag = TRUE;
	index_word_t tmp_index_word;
	index_word_t last_index_word;
	koma_complex_noun_support_t *wrapper=NULL;
	index_word_t *tmp_buffer_index_word;

	if (extractor->id != MY_EXTRACTOR_ID2) 
		return MINUS_DECLINE;


	wrapper = extractor->handle;

	limited_index_word_len = max_index_word; //XXX: description for it goes here..
	rv = koma_analyze(wrapper->koma, index_word, limited_index_word_len);
	if (rv == FAIL) {
		error("koma_analyze returned FAIL");
		return FAIL;
	}

	if (rv == 0)  /* end of text */
		return 0;

	is_first_cut = TRUE;

	memset(&last_index_word, 0x00 , sizeof(index_word_t));

	tmp_buffer_index_word = (index_word_t*)sb_calloc(max_index_word , sizeof(index_word_t));

	if ( tmp_buffer_index_word == NULL ) {
		info("cannot alloction memory : tmp_index_word array size :[%d]", max_index_word);
		return FAIL;
	}

	/* complex noun dealing code here.. */
	idx = 0; //XXX: overwrite.. (removes A, B, C, D..)
	for (i=0; i<rv; i++) {


		if ( !(TAG_IS_NOUN((char *)&(index_word[i].attribute))
				|| TAG_IS_KYUKJOSA((char *)&(index_word[i].attribute)) ) ) {
				if (flag == TRUE && last_index_word.len > 0) {
					tmp_buffer_index_word[idx] = last_index_word;
					idx++;
					if (idx == max_index_word)
						break;
				} 
				tmp_buffer_index_word[idx] = index_word[i];
				is_first_cut = FALSE;
				last_index_word = index_word[i];
				flag = FALSE;
				idx++;
				if (idx == max_index_word)
					break;

		}
		else if (last_position == index_word[i].pos ) {


			tmp_index_word = index_word[i];
			merge_word( &(tmp_buffer_index_word[idx]), &last_index_word, &(index_word[i]));
			last_index_word = tmp_index_word;
			flag = FALSE;
			idx++;

			is_first_cut = FALSE;

		}
		else {

			tmp_index_word = index_word[i];

			if ( i>0 && is_first_cut == TRUE ) {
				tmp_buffer_index_word[idx] = last_index_word;
				idx++;
			}

			last_index_word = tmp_index_word;
			flag = TRUE;

			is_first_cut = TRUE;
		}

		last_position = index_word[i].pos;

		if (idx == max_index_word)
			break;
	}

	if ( i>0 && idx < max_index_word && is_first_cut == TRUE ) {
		tmp_buffer_index_word[idx] = last_index_word;
		idx++;
	}

	for(i=0; i<idx; i++) { // copy
		index_word[i] = tmp_buffer_index_word[i];
	}

	sb_free(tmp_buffer_index_word);

	return idx;

}

static void register_hooks(void)
{
	sb_hook_new_index_word_extractor(new_koma_wrapper,NULL,NULL,HOOK_MIDDLE);
	sb_hook_index_word_extractor_set_text(koma_wrapper_set_text,NULL,NULL,HOOK_MIDDLE);
	sb_hook_delete_index_word_extractor(delete_koma_wrapper,NULL,NULL,HOOK_MIDDLE);

	sb_hook_get_index_words(koma_wrapper_analyze1,NULL,NULL,HOOK_MIDDLE);
	sb_hook_get_index_words(koma_wrapper_analyze2,NULL,NULL,HOOK_MIDDLE);
}

module koma_complex_noun_support_module = {
    STANDARD_MODULE_STUFF,
    NULL, 	                /* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
