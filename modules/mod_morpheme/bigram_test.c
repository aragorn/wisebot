/* $Id$ */

#include "softbot.h"
#include "mod_api/tokenizer.h"
#include"bigram.h"

typedef struct _bigram_handle_t
{
	token_t *last_token;     // ������ ��ū
	int32_t last_position;     // ������ ������
} bigram_handle_t;

int main()
{
    int i, num=-1;
	bigram_handle_t *handle;
	index_word_t index_word[1024];

	
	handle = new_bigram_generator();


	bigram_set_text_old(handle,
		"�� ABCD�ѱ����ǵ����12345 �츮����a ��������cd ee qwe aaa123 \n\n");

	printf("�� ABCD�ѱ����ǵ����12345 �츮����a ��������cd ee qwe aaa123 \n\n");
	do {
		num = bigram_generator_old(handle, index_word, 5);

		for(i=0; index_word[i].len != 0 ; i++) {
			printf("%ld %s\n", index_word[i].position , index_word[i].string);
		}

	} while(num);

	bigram_destroyer(handle);

	return 0;

}
	
	
