/* $Id$ */
#ifndef TOKENIZER_H
#define TOKENIZER_H 1

#include "softbot.h"

typedef struct _tokenizer_t tokenizer_t;

typedef struct {
	int32_t type;
	char    string[MAX_WORD_LEN];
	int32_t len;
	int32_t byte_position;
} token_t;

#define TOKEN_KOREAN		0x01
#define TOKEN_CHINESE		0x02
#define TOKEN_JAPANESE		0x03
#define TOKEN_ALPHABET		0x04
#define TOKEN_NUMBER		0x05
#define TOKEN_SPECIAL_CHAR	0x06

#define TOKEN_END_OF_WORD		0x10
#define TOKEN_END_OF_SENTENCE	0x20
#define TOKEN_END_OF_PARAGRAPH	0x30
#define TOKEN_END_OF_DOCUMENT	0x40

SB_DECLARE_HOOK(tokenizer_t*,new_tokenizer,(void))
SB_DECLARE_HOOK(void, tokenizer_set_text, (tokenizer_t* tokenizer, char* text))
SB_DECLARE_HOOK(int, get_tokens, (tokenizer_t* tokenizer, token_t t[], int max))
SB_DECLARE_HOOK(void, delete_tokenizer, (tokenizer_t* tokenizer))
#endif
