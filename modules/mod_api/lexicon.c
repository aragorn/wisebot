/* $Id$ */
#include "softbot.h"
#include "mod_api/lexicon.h"

HOOK_STRUCT(
	HOOK_LINK(open_word_db)
	HOOK_LINK(sync_word_db)
	HOOK_LINK(close_word_db)

	HOOK_LINK(get_new_wordid)
	HOOK_LINK(get_word)
	HOOK_LINK(get_word_by_wordid)
	HOOK_LINK(get_num_of_word)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,open_word_db,\
			( word_db_t** word_db, int opt ), (word_db, opt),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,sync_word_db,\
			( word_db_t* word_db ),(word_db),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,close_word_db,\
			( word_db_t* word_db ),(word_db),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_new_wordid,\
			( word_db_t* word_db, word_t *word ),( word_db, word ),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_word,\
			( word_db_t* word_db, word_t *word ),( word_db, word ),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_word_by_wordid,\
			( word_db_t* word_db, word_t *word ),( word_db, word ),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_num_of_word,\
			( word_db_t* word_db, uint32_t  *num ),( word_db, num ),DECLINE)

