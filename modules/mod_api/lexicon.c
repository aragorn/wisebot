/* $Id$ */
#include "softbot.h"
#include "mod_api/lexicon.h"

HOOK_STRUCT(
	HOOK_LINK(get_global_word_db)

	HOOK_LINK(open_word_db)
	HOOK_LINK(synchronize_wordids)
	HOOK_LINK(sync_word_db)
	HOOK_LINK(close_word_db)

	HOOK_LINK(get_new_word)
	HOOK_LINK(get_word)
	HOOK_LINK(get_word_by_wordid)
	HOOK_LINK(get_num_of_word)
	
	HOOK_LINK(get_right_truncation_words)
	HOOK_LINK(get_left_truncation_words)
	
	HOOK_LINK(clear_wordids)
	
	HOOK_LINK(print_hash_bucket)
	HOOK_LINK(print_hash_status)
	HOOK_LINK(print_truncation_bucket)
)

// server implement APIs

SB_IMPLEMENT_HOOK_RUN_FIRST(word_db_t*,get_global_word_db,\
			(void), (), DECLINE)

SB_IMPLEMENT_HOOK_RUN_ALL(int,open_word_db,\
			( word_db_t *word_db, char *path, int flag ), (word_db, path, flag),SUCCESS,DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int,synchronize_wordids,\
			( word_db_t *word_db ),(word_db),SUCCESS,DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int,sync_word_db,\
			( word_db_t *word_db ),(word_db),SUCCESS,DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int,close_word_db,\
			( word_db_t *word_db ),(word_db),SUCCESS,DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_new_word,\
			( word_db_t *word_db, word_t *word, uint32_t did ),( word_db, word, did),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_word,\
			( word_db_t *word_db, word_t *word ),( word_db,  word ),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_word_by_wordid,\
			( word_db_t *word_db, word_t *word ),( word_db, word ),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_num_of_word,\
			( word_db_t *word_db, uint32_t  *num ),( word_db, num ),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_right_truncation_words,\
			( word_db_t *word_db, char *word, word_t ret_word[], int max_word_num ),\
			( word_db, word, ret_word, max_word_num ),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,get_left_truncation_words,\
			( word_db_t *word_db, char *word, word_t ret_word[], int max_word_num ),\
			( word_db, word, ret_word, max_word_num ),DECLINE)


SB_IMPLEMENT_HOOK_RUN_ALL(int,clear_wordids,\
			( word_db_t *word_db ),( word_db ),SUCCESS,DECLINE)

SB_IMPLEMENT_HOOK_RUN_ALL(int,print_hash_bucket,\
			( word_db_t *word_db, int bucket_idx ),( word_db, bucket_idx ),SUCCESS,DECLINE)

SB_IMPLEMENT_HOOK_RUN_ALL(int,print_hash_status,\
			( word_db_t *word_db ),( word_db ),SUCCESS,DECLINE)

SB_IMPLEMENT_HOOK_RUN_ALL(int,print_truncation_bucket,\
			( word_db_t *word_db, int bucket_idx ),( word_db, bucket_idx ),SUCCESS,DECLINE)
