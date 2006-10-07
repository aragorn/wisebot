/* $Id$ */
#include "common_core.h"
#include "memory.h"

#include "mod_api/tokenizer.h"
#include "mod_bigram.h"
#include "mod_api/index_word_extractor.h"

/* XXX: should be removed */
#include "lib/ma_code.h"

#define MY_EXTRACTOR_ID 	20
#define MAX_WORD_NUM		1024

static void add_index_word(index_word_t *index_word , token_t *token , int32_t pos);
static void merge_two_tokens(token_t *token1 , token_t *token2 , token_t *dest_token);

static int minimum_token_length = 3; /* ���ξ�� ����ϴ� �ּұ����� ��ū */

// XXX: should be extern...
bigram_t* new_bigram()
{
	bigram_t *handle=NULL;
	handle = sb_calloc(1,sizeof(bigram_t));
	if (handle == NULL) {
		crit("cannot allocate bigram handle");
		return NULL;
	}

	handle->tokenizer = sb_run_new_tokenizer();
	if (handle->tokenizer == NULL) {
		crit("cannot allocate tokenizer");
		sb_free(handle);
		return NULL;
	}

	return handle;
}

static index_word_extractor_t* new_bigram_generator(int id)
{
	index_word_extractor_t *extractor = NULL;
	bigram_t *handle = NULL;

	if (id != MY_EXTRACTOR_ID)
		return (index_word_extractor_t*)MINUS_DECLINE;

	extractor = sb_calloc(1, sizeof(index_word_extractor_t));
	if (extractor == NULL) {
		crit("cannot allocate index word extractor object");
		return NULL;
	}
	
	handle = new_bigram(); 
	if (handle == NULL) {
		crit("cannot allocate bigram handle");
		sb_free(extractor);
		return NULL;
	}
	extractor->handle = handle;
	extractor->id = id;

	if (extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("extractor pointer is %d (same as the decline value for this api",
								MINUS_DECLINE);
	}

	return extractor;
}

void bigram_set_text(bigram_t *handle, const char* text)
{
	sb_run_tokenizer_set_text(handle->tokenizer, text);
	handle->last_token.type = TOKEN_END_OF_DOCUMENT;
	handle->last_token.len = 0;
	handle->last_token_idx = MAX_TOKENS;
	handle->position = 1;
}

static int _bigram_set_text(index_word_extractor_t* extractor, const char* text)
{
	bigram_t *handle = NULL;

	if (extractor->id != MY_EXTRACTOR_ID) 
		return DECLINE;
	
	handle = extractor->handle;

	bigram_set_text(handle, text);

	return SUCCESS;
}

void delete_bigram_generator(bigram_t *handle)
{
	sb_run_delete_tokenizer(handle->tokenizer);
	sb_free(handle);
}

static int _delete_bigram_generator(index_word_extractor_t *extractor)
{
	bigram_t *handle = NULL;
	if (extractor->id != MY_EXTRACTOR_ID)
		return DECLINE;

	handle = extractor->handle;
	delete_bigram_generator(handle);
	sb_free(extractor);

	return SUCCESS;
}

#define IS_WHITE_TOKEN(t) ( (t)->type == TOKEN_END_OF_WORD  || (t)->type == TOKEN_END_OF_DOCUMENT \
					  || (t)->type == TOKEN_END_OF_PARAGRAPH || (t)->type == TOKEN_END_OF_SENTENCE )
// XXX: max_index_word��, �ѱ� ��ū�� �ִ� ���� ���� Ŀ����. 

int bigram_generator(bigram_t *handle, index_word_t index_word[], int32_t max_index_word) 
{ 
	token_t *token_array = NULL;
	token_t *current_token, *prev_token , *next_token, tmp_token1, tmp_token2; // �ӽ� token 
	token_t end_of_token;
	int32_t *pos = NULL;
	int32_t index_word_idx = 0 , tmp_index_word_idx = 0; // index_word ����ü�� index.. 
	int32_t *num_of_tokens = NULL;
	int is_index_word = 0;  // Ȧ�� �ܾ�� ä���Ұ��ΰ� �ƴѰ��� ǥ���ϴ� �ӽ� flag 
	int i=0, *current_idx = NULL;

	pos = &(handle->position); // ������ �۾��ߴ� position�� �̾�޴´�. 

	end_of_token.type = TOKEN_END_OF_WORD;
	end_of_token.len = 0;

	if (handle->last_token_idx < MAX_TOKENS 
			&& handle->last_tokens[handle->last_token_idx].type != TOKEN_END_OF_DOCUMENT) {
		current_idx = &(handle->last_token_idx);
		num_of_tokens = &(handle->num_of_tokens);
	}
	else {
		handle->num_of_tokens = sb_run_get_tokens(handle->tokenizer, handle->last_tokens, MAX_TOKENS);
		handle->last_token_idx = 0;
		current_idx = &(handle->last_token_idx);
		num_of_tokens = &(handle->num_of_tokens);
	}

	token_array = handle->last_tokens;

	// �� , ����, �� ��ū �ʱ� ���� 
	prev_token = &(handle->last_token);
	current_token = &(token_array[*current_idx]);
	next_token = &(token_array[(*current_idx)+1]);

	// ���� ������ ����

	while(*current_idx < *num_of_tokens && current_token->type != TOKEN_END_OF_DOCUMENT) { 

		is_index_word = 0;
		// Rule of Index Word  

		switch(current_token->type) 
		{ 
			case TOKEN_ALPHABET : // ������ Token, ���� Token, Ư������ Token�� ���  

				for(i=0; i<current_token->len; i++) {
					current_token->string[i] = 
							(char)toupper(current_token->string[i]);
				}

				// falls through
			case TOKEN_NUMBER:         
			case TOKEN_SPECIAL_CHAR : 

				// ���� �ܾ���� ���� 
				if(!IS_WHITE_TOKEN(prev_token))
				{
					merge_two_tokens( prev_token , current_token , &tmp_token1);
					add_index_word( &(index_word[index_word_idx]), &tmp_token1, *pos);  
				    if(++index_word_idx >= max_index_word)
						goto END_OF_EXTRACTING_INDEX_WORDS; 
				}

				if (current_token->len >= minimum_token_length)
				/* ���̰� minimum_token_length byte �̻��� ��� bigram �����   
				 * ���հ��� ������ Ȭ Token�� ���ξ�� �߰��Ѵ�. */
					is_index_word = 1;       
				else if (current_token->len >= 2)
				/* ���̰� 2byte �̻��̸鼭 �ش� Token�� ���� �� ���ܿ� ��ġ�� ��� 
				 * �ش� Token�� ���ξ�� �߰��Ѵ�.  */
				{                                 
					if (IS_WHITE_TOKEN(prev_token) || IS_WHITE_TOKEN(next_token) ) 
						is_index_word = 1; 
				} 

				if (current_token->len < minimum_token_length)
				/* Ȧ�� ����, �� �յڰ� ��� ���� ���� ������ Ȭ Token�� ���̰� 
				 * minimum_token_length byte �̸��� ��� ���ξ�� ������� �ʴ´�. */
				{  
					if ( IS_WHITE_TOKEN(prev_token) && IS_WHITE_TOKEN(next_token) ) 
						is_index_word = 0; 
				} 

				/* bigram ���� Ư���ϰ� ����ϴ� \< \> �� Ȧ�� �������� �ʴ´�. */
				if ( current_token->len == 2
						&& ( strcmp(current_token->string, "\\<") == 0
							|| strcmp(current_token->string, "\\>") == 0 ) ) {
					is_index_word = 0;
				}

				if( is_index_word )
				{ 
					add_index_word( &index_word[index_word_idx] , current_token , *pos);
				    if(++index_word_idx >= max_index_word)
						goto END_OF_EXTRACTING_INDEX_WORDS; 
					
				}     

				break; 

			case TOKEN_CHINESE: 

				CDconvChn2Kor(current_token->string , current_token->string);
				// falls through
			case TOKEN_JAPANESE:
				// falls through

			case TOKEN_KOREAN: 

				tmp_index_word_idx = index_word_idx;

				if(!IS_WHITE_TOKEN(prev_token)) {
					tmp_token1.len = 2;                         // �ӽ� token setting 
					tmp_token1.string[0] = current_token->string[0];   // ���� 
					tmp_token1.string[1] = current_token->string[1];                                     
					tmp_token1.string[2] = '\0'; 

					merge_two_tokens( prev_token , &tmp_token1 , &tmp_token2 );
					add_index_word( &(index_word[index_word_idx]) , &tmp_token2 , *pos );

				    if(++index_word_idx >= max_index_word) {
						// reserve last unmerged Korean character
						handle->last_token = tmp_token1;
						handle->last_token.type = TOKEN_KOREAN;

						if ( current_token->len > 2 ) {
							strncpy(&(current_token->string[0]) , &(current_token->string[2]),
											MAX_WORD_LEN );
							current_token->len = strlen(current_token->string);
							current_token->string[MAX_WORD_LEN-1]='\0';
						}
						else {
							(*current_idx)++;
						}
/*						INFO("last_token:[%s] current_token:[%s]",handle->last_token.string, token_array[*current_idx].string);*/

						return index_word_idx;
					}
				}

				/* 
				 * Ȧ�� ����, �� �յڰ� ��� ���� ���� ������ Ȭ Token�� ���̰� 
				 * minimum_token_length byte �̸��� ��� ���ξ�� ������� �ʴ´�.  
				 */
				if (current_token->len < minimum_token_length)
					break;

				for(i=0 ; i < current_token->len-2 ; i+=2) 
				{ 
					tmp_token1.len = 4;                         // �ӽ� token setting 
					tmp_token1.string[0] = current_token->string[i];   // ���� 
					tmp_token1.string[1] = current_token->string[i+1];                                     
					tmp_token1.string[2] = current_token->string[i+2];                                     
					tmp_token1.string[3] = current_token->string[i+3];                                     
					tmp_token1.string[4] = '\0'; 

					add_index_word( &(index_word[index_word_idx]) , &tmp_token1, *pos); 
					if(++index_word_idx >= max_index_word) {

						handle->last_token.len = 2;
						handle->last_token.string[0] = current_token->string[i+2];                                     
						handle->last_token.string[1] = current_token->string[i+3];                                     
						handle->last_token.string[2] = '\0'; 
						handle->last_token.type = TOKEN_KOREAN;

						if ( current_token->len>4) {
							strncpy(&(current_token->string[0]) , &(current_token->string[i+4]) , MAX_WORD_LEN );
/*							INFO("current_token:%s %s", &(current_token->string[0]) ,  &(current_token->string[i+4]));*/
							current_token->len = strlen(current_token->string);
							current_token->string[MAX_WORD_LEN-1]='\0';
						}
						else {
							(*current_idx)++;
						}

/*						INFO("last_token:[%s] current_token:[%s]",handle->last_token.string, token_array[*current_idx].string);*/
						return index_word_idx;
							
					}
				} 

				// �ѱ� ��ū���� �ѱ��ڸ� ���� ��. Ȭ������ ��쵵 �����.

				tmp_token1.len = 2;                         // �ӽ� token setting 
				tmp_token1.string[0] = current_token->string[i];   // ���� 
				tmp_token1.string[1] = current_token->string[i+1];                                     
				tmp_token1.string[2] = '\0'; 

				strncpy(current_token->string, tmp_token1.string, MAX_WORD_LEN);
				current_token->string[MAX_WORD_LEN-1] = '\0';
				current_token->len = tmp_token1.len;
				
				if ( i == 0 )
					add_index_word( &(index_word[index_word_idx++]) , &tmp_token1, *pos); 

				break; 

			case TOKEN_END_OF_WORD:      // ���� ��ū 
			case TOKEN_END_OF_SENTENCE: 
			case TOKEN_END_OF_PARAGRAPH: 

				(*pos)++;     // ���� ��ū�� ��Ÿ���� ������ ���� 

				break; 

		} // switch  


		prev_token = token_array + (*current_idx); // = current_token;
		current_token = token_array + (*current_idx) + 1; // = next_token;
		if(*current_idx + 2 == *num_of_tokens) {
			next_token = &end_of_token;
		}
		else {
			next_token = token_array + (*current_idx) + 2;
		}

		(*current_idx)++;


	} // for 

END_OF_EXTRACTING_INDEX_WORDS:

	handle->last_token = handle->last_tokens[handle->last_token_idx-1];

	return index_word_idx;
}

static int _bigram_generator(index_word_extractor_t *extractor, index_word_t index_word[], int32_t max_index_word) 
{
	bigram_t *bigram=NULL;
	if (extractor->id != MY_EXTRACTOR_ID)
		return MINUS_DECLINE;

	bigram = extractor->handle;
	return bigram_generator(bigram, index_word, max_index_word);
}


static void merge_two_tokens( token_t *token1 , token_t *token2 , token_t *dest_token)
{

	dest_token->len = token1->len + token2->len;   

	if (dest_token->len > MAX_WORD_LEN-1) 
		dest_token->len = MAX_WORD_LEN-1; 

	strncpy(dest_token->string , token1->string , MAX_WORD_LEN); 

	strncat(dest_token->string , token2->string , MAX_WORD_LEN - token1->len); 

	dest_token->string[dest_token->len] = 0;

}

static void add_index_word ( index_word_t *index_word , token_t *token , int32_t pos)
{
	index_word->pos = pos;
	index_word->len = token->len;
	strncpy(index_word->word, token->string, MAX_WORD_LEN);

	return;
}

static void set_minimum_token_length(configValue v)
{
	minimum_token_length = atoi(v.argument[0]);
}

static config_t config[] = {
	CONFIG_GET("MinimumTokenLength", set_minimum_token_length,1,\
			"Minimum token length, default 3"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_new_index_word_extractor(new_bigram_generator,NULL,NULL,HOOK_MIDDLE);
	sb_hook_index_word_extractor_set_text(_bigram_set_text,NULL,NULL,HOOK_MIDDLE);
	sb_hook_get_index_words(_bigram_generator,NULL,NULL,HOOK_MIDDLE);
	sb_hook_delete_index_word_extractor(_delete_bigram_generator,NULL,NULL,HOOK_MIDDLE);
}

module bigram_module = {
    STANDARD_MODULE_STUFF,
    config,	                /* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
