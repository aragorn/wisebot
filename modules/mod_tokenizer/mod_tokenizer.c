/* $Id$ */
#include <stdint.h>
#include <string.h>
#include "common_core.h"
#include "memory.h"
#include "mod_api/tokenizer.h"
#include "mod_tokenizer.h"

#define MAX_KOREAN_LEN		MAX_WORD_LEN
#define MAX_CHINESE_LEN		MAX_WORD_LEN
#define MAX_JAPANESE_LEN	MAX_WORD_LEN
#define MAX_ALPHABET_LEN	20
#define MAX_NUMBER_LEN		10
#define MAX_SPECIALCHAR_LEN	4

#define IS_WHITE(c)		((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))
#define IS_COMMA(c)		(c==',')
#define IS_KOREAN(c)	IS_KOREAN_FIRST_BYTE(c)
#define IS_KOREAN_FIRST_BYTE(c)	((c >= 0xb0) && (c <= 0xc8) )
#define IS_ALPHABET(c)	(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))
#define IS_CHINESE(c)	((c >= 0xca) && (c <= 0xfd))
#define IS_JAPANESE(c)	((c == 0xaa) || (c == 0xab))
#define IS_NUMBER(c)	((c >= '0') && (c <= '9'))
#define IS_SPECIAL_CHAR(c)	(c < 128 && special_char_table[c])
#define IS_END_OF_DOCUMENT(c)	(c == '\0')

static uint8_t special_char_table[128] =
{ 
/*0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 0
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 1
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    // 2
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,    // 3
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 4
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,    // 5
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 6
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0     // 7
};


static tokenizer_t* new_tokenizer(void)
{
	return (tokenizer_t*)sb_calloc(1, sizeof(tokenizer_t));
}

static void set_text(tokenizer_t *tokenizer, char *text)
{
	tokenizer->text = text;
	tokenizer->current = tokenizer->text;
}

static void delete_tokenizer(tokenizer_t *tokenizer)
{
	sb_free(tokenizer);
}

static void add_statustoken(token_t *token, int tokentype);
static int add_token(token_t *token, unsigned char* c, int *pass);

int get_tokens(tokenizer_t *tokenizer, token_t tokens[], int maxtokens)
{
	unsigned char c;
	int idx=0,tokenlen=0,pass=0;

	/* XXX: end of paragraph, end of line check goes here */
	while (1) {
		c = *(tokenizer->current);
		if (idx >= maxtokens) {
			break;
		}
		else if (IS_WHITE(c) || IS_COMMA(c)) {
			add_statustoken(&tokens[idx],TOKEN_END_OF_WORD);
			idx++;

			while( IS_WHITE(*tokenizer->current) || IS_COMMA(*tokenizer->current) ) tokenizer->current++;

		}
		else if (IS_END_OF_DOCUMENT(c)) {
			add_statustoken(&tokens[idx],TOKEN_END_OF_DOCUMENT);
			idx++;
			
			break;
		}
		else {
			tokenlen = add_token(&tokens[idx], tokenizer->current, &pass);
			tokenizer->current += pass;
			if (tokenlen != 0) {
				idx++;
			}
		}
	}

	return idx;
}

static void add_statustoken(token_t *token, int tokentype)
{
	token->type = tokentype;
	snprintf(token->string,MAX_WORD_LEN,"#STATUS_TOKEN:0x%x",tokentype);
	token->string[MAX_WORD_LEN-1]='\0';
	token->len = strlen(token->string);
}

static int add_token(token_t *token, unsigned char *c, int *pass)
{
	int i=0;
	*pass = 0;
	token->len = 0;

	if (IS_KOREAN(*c)) {
		token->type = TOKEN_KOREAN;
		while(IS_KOREAN(*c)) {
			if (*(c+1) == '\0') { //XXX: half korean
				*pass+=1;
				break;
			}

			/* ¡¾©¡AI¡Æ¢® ©øN¨úi¡Æ¢®¢¥A ¡Æ©¡¢¯i¢¯¢®¢¥A ¡¾¡¿¡¾iAo token¢¯¢® copyCI¡Æi
			 * ¢¥UA¨ö token¢¯¢®¢¥U ©ø¨£¢¬OAo ¨¬I¨¬¨¢A¡í copyCO ¨ùo AO¡ÆO..
			 */
			if (i+1 >= MAX_WORD_LEN || i+2 >= MAX_WORD_LEN)
				break;

//			if (i+1 < MAX_KOREAN_LEN && i+2 < MAX_WORD_LEN) {
				token->string[i] = *c;
				token->string[i+1] = *(c+1);
				token->string[i+2] = '\0';
				token->len += 2;
//			}
			*pass+=2;
			i+=2;
			c+=2;
		}
	}
	else if (IS_CHINESE(*c)) {
		token->type = TOKEN_CHINESE;
		while(IS_CHINESE(*c)) {
			if (*(c+1) == '\0') { //XXX: half chinese
				*pass+=1;
				break;
			}

			if (i+1 >= MAX_WORD_LEN || i+2 >= MAX_WORD_LEN)
				break;

//			if (i+1 < MAX_CHINESE_LEN && i+2 < MAX_WORD_LEN) {
				token->string[i] = *c;
				token->string[i+1] = *(c+1);
				token->string[i+2] = '\0';
				token->len += 2;
//			}
			*pass+=2;
			i+=2;
			c+=2;
		}
	}
	else if (IS_JAPANESE(*c)) {
		token->type = TOKEN_JAPANESE;
		while(IS_JAPANESE(*c)) {
			if (*(c+1) == '\0') { //XXX: half japanese
				*pass+=1;
				break;
			}

			if (i+1 >= MAX_WORD_LEN || i+2 >= MAX_WORD_LEN)
				break;

//			if (i+1 < MAX_JAPANESE_LEN && i+2 < MAX_WORD_LEN) {
				token->string[i] = *c;
				token->string[i+1] = *(c+1);
				token->string[i+2] = '\0';
				token->len += 2;
//			}
			*pass+=2;
			i+=2;
			c+=2;
		}
	}
	else if (IS_ALPHABET(*c)) {
		token->type = TOKEN_ALPHABET;
		while(IS_ALPHABET(*c)) {
			if (i >= MAX_ALPHABET_LEN || i+1 >= MAX_WORD_LEN)
				break;

			token->string[i] = *c;
			token->len += 1;

			*pass+=1;
			i+=1;
			c+=1;
		}
		token->string[i] = '\0';
	}
	else if (IS_NUMBER(*c)) {
		token->type = TOKEN_NUMBER;
		while(IS_NUMBER(*c)) {
			if (i >= MAX_NUMBER_LEN || i+1 >= MAX_WORD_LEN)
				break;

//			if (i < MAX_NUMBER_LEN && i+1 < MAX_WORD_LEN) {
				token->string[i] = *c;
				token->string[i+1] = '\0';
				token->len += 1;
//			}
			*pass+=1;
			i+=1;
			c+=1;
		}
	}
	else if (IS_SPECIAL_CHAR(*c)) {
		token->type = TOKEN_SPECIAL_CHAR;
		while(IS_SPECIAL_CHAR(*c)) {
			if (i >= MAX_SPECIALCHAR_LEN || i+1 >= MAX_WORD_LEN)
				break;

			token->string[i] = *c;
			token->len += 1;

			*pass+=1;
			i+=1;
			c+=1;
		}
		token->string[i] = '\0';
	}
	else { // XXX: unknown euc-kr code
		if (*(c+1) != '\0' && ((*c) & 0x80) ) {
			*pass+=2;
			c+=2;
		}
		else {
			*pass+=1;
			c+=1;
		}
	}

	return i;
}

static void register_hooks(void)
{
	sb_hook_new_tokenizer(new_tokenizer, NULL, NULL, HOOK_FIRST);
	sb_hook_tokenizer_set_text(set_text, NULL, NULL, HOOK_FIRST);
	sb_hook_get_tokens(get_tokens, NULL, NULL, HOOK_FIRST);
	sb_hook_delete_tokenizer(delete_tokenizer, NULL, NULL, HOOK_FIRST);
}

module tokenizer_module = {
    STANDARD_MODULE_STUFF,
    NULL,                	/* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};

