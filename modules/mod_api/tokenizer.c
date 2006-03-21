/* $Id$ */
#include "common_core.h"
#include "tokenizer.h"

HOOK_STRUCT(
	HOOK_LINK(new_tokenizer)
	HOOK_LINK(tokenizer_set_text)
	HOOK_LINK(get_tokens)
	HOOK_LINK(delete_tokenizer)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(tokenizer_t*, new_tokenizer, (void),(),NULL)
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(tokenizer_set_text, (tokenizer_t* tokenizer, char* text),\
										(tokenizer, text))
SB_IMPLEMENT_HOOK_RUN_FIRST(int, get_tokens,(tokenizer_t* tokenizer, token_t t[], int max),\
										(tokenizer, t, max), DECLINE)
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(delete_tokenizer, (tokenizer_t* tokenizer),(tokenizer))
