/* $Id$ */
#ifndef MORPHEME_H
#define MORPHEME_H 1

#include "softbot.h"
#include "tokenizer.h"
#include "mod_morpheme/lib/lb_lex.h"	/* to use CLex type */
#include "mod_tokenizer/mod_tokenizer.h"

typedef struct{
	char *next;
} Paragraph;

//XXX: temporary...
#define MAX_TOKENS 32
typedef struct _bigram_handle_t
{
	token_t last_token;     // 지난번 토큰

	tokenizer_t *tokenizer;
	tokenizer_t tokenizerobj; // obsolete. only used in mod_morphem/ files

	token_t last_tokens[MAX_TOKENS];
	int32_t last_token_idx;
	int32_t num_of_tokens;

	int32_t position;
} bigram_handle_t;

typedef struct {
	char word[MAX_WORD_LEN];
	int  len;
	uint32_t attribute;
	uint8_t field;
	uint32_t position;
} Word;

typedef struct {
	CLexVar lexHandle;
	CLex lex;
	bigram_handle_t bigram_handle;
} Morpheme;

typedef struct {
	Word words[MAX_WORD_PER_PHRASE];
	uint32_t num_of_words;
} WordList;

#define morp_get_word(wordObj)	((wordObj)->word)
#define morp_get_wordattr(wordObj)	((wordObj)->attribute)
#define morp_set_wordattr(wordObj,attr) (((wordObj)->attribute) = (attr))

SB_DECLARE_HOOK(int,morp_set_text,(Morpheme *morpObj,char *strings,char id))
SB_DECLARE_HOOK(int,morp_get_wordlist,\
		(Morpheme *morpObj,WordList *wordList,char id))
SB_DECLARE_HOOK(int,morp_get_wordpos,\
		(Morpheme *morpObj,int16_t *pos, int16_t *bytepos))
SB_DECLARE_HOOK(void,morp_set_paragraphtext,(Paragraph *para, char *text))
SB_DECLARE_HOOK(char*,morp_get_paragraph,(Paragraph *para, char flag))
SB_DECLARE_HOOK(char*,morp_get_paragraphlen,(Paragraph *para, int *len))



#endif
