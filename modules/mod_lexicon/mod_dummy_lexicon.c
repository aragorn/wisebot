/* $Id$ */
#include "softbot.h"
#include "mod_api/lexicon.h"

static uint32_t wordid = 1;

static int get_new_wordid_dummy(word_db_t *word_db,  word_t *word, uint32_t did )
{
	word->word_attr.id = wordid++;
	if (wordid >= 1000) wordid = 1;
	return SUCCESS;
}

static int get_wordid_dummy(word_db_t *word_db,  word_t *word )
{
	word->word_attr.id = 1;
	return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_get_new_word(get_new_wordid_dummy, 	NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_word(get_wordid_dummy, 	NULL, NULL, HOOK_MIDDLE);
}

module dummy_lexicon_module=
{
	STANDARD_MODULE_STUFF,
    NULL,                 /* config */
    NULL,                   /* registry */
    NULL,             /* initialize function */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
