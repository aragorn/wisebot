/* $Id$ */
#include "softbot.h"
#include "mod_api/tokenizer.h"
#include "mod_api/morpheme.h"
#include "mod_bigram.h"
#include "lib/ma_code.h"

/* XXX obsolete code? --aragorn, 2002/11/22 */

#define MAX_WORD_NUM		1024

static void add_index_word ( Word *index_word , token_t *token , int32_t pos);
static void merge_two_tokens( token_t *token1 , token_t *token2 , token_t *dest_token);

void* new_bigram_generator(int id)
{
	bigram_handle_t *handle=NULL;

	if (id != 2)
		return DECLINE;
		
	handle = sb_calloc(1,sizeof(bigram_handle_t));
	if (handle != NULL) {
		handle->tokenizer = sb_run_new_tokenizer();
	}
	if (handle == NULL || handle->tokenizer == NULL) {
		crit("handle[%p] or handle->tokenizer[%p] is null",
					handle, handle->tokenizer);
	}
	return handle;
}

/* FIXME this function causes collision with 
 * mod_index_word_extractor/mod_bigram.c . */
void bigram_set_text_old(void* h, char* text)
{
	bigram_handle_t *handle = h;
	sb_run_tokenizer_set_text(handle->tokenizer, text);
	handle->last_token.type = TOKEN_END_OF_DOCUMENT;
	handle->last_token.len = 0;
	handle->last_token_idx = MAX_TOKENS;
	handle->position = 0;
}

void delete_bigram_generator_old(void *h)
{
	bigram_handle_t *handle = h;
	sb_run_delete_tokenizer(handle->tokenizer);
	sb_free(handle);
}


#define IS_WHITE_TOKEN(t) ( (t)->type == TOKEN_END_OF_WORD  || (t)->type == TOKEN_END_OF_DOCUMENT \
						  || (t)->type == TOKEN_END_OF_PARAGRAPH || (t)->type == TOKEN_END_OF_SENTENCE )
	// max_index_word가, 한글 토큰의 최대 길이 보다 커야함. 

int bigram_generator_old(void *h, Word index_word[], int32_t max_index_word) 
{ 
	bigram_handle_t *handle = h;
	token_t *token_array = NULL;
	token_t *current_token, *prev_token , *next_token, tmp_token , tmp_token2; // 임시 token 
	token_t end_of_token;
	int32_t *pos = &(handle->position) ; // 이전에 작업했던 position을 이어받는다. 
	int32_t index_word_idx = 0 , tmp_index_word_idx = 0; // index_word 구조체의 index.. 
	int32_t *num_of_tokens = NULL;
	int is_index_word = 0;  // 홀로 단어로 채택할것인가 아닌가를 표시하는 임시 flag 
	int i=0, *current_idx = NULL;

	sb_assert(max_index_word > MAX_WORD_LEN / 2);

	end_of_token.type = TOKEN_END_OF_WORD;
	end_of_token.len = 0;


	if (handle->last_token_idx < MAX_TOKENS 
			&& handle->last_tokens[handle->last_token_idx].type != TOKEN_END_OF_DOCUMENT) {
		current_idx = &(handle->last_token_idx);
		num_of_tokens = &(handle->num_of_tokens);
	}
	else {
		handle->num_of_tokens = sb_run_get_tokens(handle->tokenizer), handle->last_tokens, MAX_TOKENS);
		handle->last_token_idx = 0;
		current_idx = &(handle->last_token_idx);
		num_of_tokens = &(handle->num_of_tokens);
	}

	token_array = handle->last_tokens;

	// 앞 , 지금, 뒤 토큰 초기 세팅 
	prev_token = &(handle->last_token);
	current_token = &(token_array[*current_idx]);
	next_token = &(token_array[*current_idx+1]);

	// 이전 포지션 세팅

	while(*current_idx < *num_of_tokens && current_token->type != TOKEN_END_OF_DOCUMENT) { 

		is_index_word = 0;
		// Rule of Index Word  

		switch(current_token->type) 
		{ 
			case TOKEN_ALPHABET : // 영문자 Token, 숫자 Token, 특수문자 Token의 경우  

				for(i=0; i<current_token->len; i++) {
					current_token->string[i] = 
							(char)toupper(current_token->string[i]);
				}

				// falls through
			case TOKEN_NUMBER:         
			case TOKEN_SPECIAL_CHAR : 

				if (current_token->len >= 3) // 길이가 3byte 이상인 경우 bigram 방식의   
					is_index_word = 1;              //조합과는 별도로 홑 Token을 색인어로 추가한다. 
				else if (current_token->len >= 2) //길이가 2byte 이상이면서 해당 Token이 어절 
				{                               // 양 끝단에 위치한 경우 해당 Token을 색인어로 추가한다.  
					if (IS_WHITE_TOKEN(prev_token) || IS_WHITE_TOKEN(next_token) ) 
						is_index_word = 1; 
				} 

				if (current_token->len < 3)  // 홀로 쓰인, 즉 앞뒤가 모두 어절 종결 상태인  
					 // 홑 Token은 길이가 3byte 미만인 경우 색인어로 사용하지 않는다.  
				{  
					if ( IS_WHITE_TOKEN(prev_token) && IS_WHITE_TOKEN(next_token) ) 
						is_index_word = 0; 
				} 


				if(is_index_word)  
				{ 
					add_index_word( &index_word[index_word_idx] , current_token , *pos);
				    if(++index_word_idx >= max_index_word)
						goto END_OF_EXTRACTING_INDEX_WORDS; 
					
				}     

				// 이전 단어와의 조합 
				if(!IS_WHITE_TOKEN(prev_token))
				{
					merge_two_tokens( prev_token , current_token , &tmp_token);
					add_index_word( &(index_word[index_word_idx]), &tmp_token, *pos);  
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

				if(!IS_WHITE_TOKEN(prev_token))  
				{
					tmp_token.len = 2;                         // 임시 token setting 
					tmp_token.string[0] = current_token->string[0];   // 복사 
					tmp_token.string[1] = current_token->string[1];                                     
					tmp_token.string[2] = '\0'; 

					merge_two_tokens( prev_token , &tmp_token , &tmp_token2 );
					add_index_word( &(index_word[index_word_idx]) , &tmp_token2 , *pos );
				    if(++index_word_idx >= max_index_word) {
						(*current_idx)--; // roll back for (only) hangul
						index_word_idx = tmp_index_word_idx;
						goto END_OF_EXTRACTING_INDEX_WORDS; 
					}
				}

				if (current_token->len < 3)  // 홀로 쓰인, 즉 앞뒤가 모두 어절 종결 상태인  
					 // 홑 Token은 길이가 3byte 미만인 경우 색인어로 사용하지 않는다.  
			    	break;


				for(i=0 ; i < current_token->len-2 ; i+=2) 
				{ 
					tmp_token.len = 4;                         // 임시 token setting 
					tmp_token.string[0] = current_token->string[i];   // 복사 
					tmp_token.string[1] = current_token->string[i+1];                                     
					tmp_token.string[2] = current_token->string[i+2];                                     
					tmp_token.string[3] = current_token->string[i+3];                                     
					tmp_token.string[4] = '\0'; 

					add_index_word( &(index_word[index_word_idx]) , &tmp_token, *pos); 
				    if(++index_word_idx >= max_index_word) {
						(*current_idx)--; // roll back for (only) hangul
						index_word_idx = tmp_index_word_idx;
						goto END_OF_EXTRACTING_INDEX_WORDS; 
					}
				} 

				tmp_token.len = 2;                         // 임시 token setting 
				tmp_token.string[0] = current_token->string[i];   // 복사 
				tmp_token.string[1] = current_token->string[i+1];                                     
				tmp_token.string[2] = 0; 

				strncpy(current_token->string, tmp_token.string, MAX_WORD_LEN);
				current_token->string[MAX_WORD_LEN-1] = '\0';
				current_token->len = tmp_token.len;
				
				break; 

			case TOKEN_END_OF_WORD:      // 종결 토큰 
			case TOKEN_END_OF_SENTENCE: 
			case TOKEN_END_OF_PARAGRAPH: 

				(*pos)++;     // 종결 토큰이 나타나면 포지션 증가 

				break; 

		} // switch  


		prev_token = token_array + *current_idx; // = current_token;
		current_token = token_array + *current_idx + 1; // = next_token;
		if(*current_idx + 2 == *num_of_tokens) {
			next_token = &end_of_token;
		}
		else {
			next_token = token_array + *current_idx + 2;
		}

		(*current_idx)++;


	} // for 

END_OF_EXTRACTING_INDEX_WORDS:

	handle->last_token = handle->last_tokens[handle->last_token_idx-1];

	return index_word_idx;
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

static void add_index_word ( Word *index_word , token_t *token , int32_t pos)
{
	index_word->position = pos;
	index_word->len = token->len;
	strncpy(index_word->word, token->string, MAX_WORD_LEN);

	return;
}

int bigram_set_text_wrapper(Morpheme *morp, char *text, char id)
{
	if (id != 2) return DECLINE;

	bigram_set_text_old(&(morp->bigram_handle),text);
	return SUCCESS;
}

int bigram_generator_wrapper(Morpheme *morp, WordList *wordlist,char id)
{
	int rv=0;
	if (id != 2) return DECLINE;

	rv = bigram_generator_old(&(morp->bigram_handle), wordlist->words, MAX_WORD_PER_PHRASE);
	wordlist->num_of_words = rv;
	if (rv == 0) return FAIL;

	return rv;
}

static void register_hooks(void)
{
/*	sb_hook_morp_set_text(bigram_set_text_wrapper,NULL,NULL,HOOK_MIDDLE);
	sb_hook_morp_get_wordlist(bigram_generator_wrapper, NULL, NULL, HOOK_MIDDLE);*/
}

module bigram_generator_module = {
    STANDARD_MODULE_STUFF,
    NULL, 	                /* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
