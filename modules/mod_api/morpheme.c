/* $Id$ */
// XXX: obsolete. see index_word_extractor.[ch]
#include "softbot.h"
#include "mod_api/morpheme.h"

HOOK_STRUCT(
	HOOK_LINK(morp_set_text)
	HOOK_LINK(morp_get_wordlist)
	HOOK_LINK(morp_get_wordpos)

//XXX:  move this to mod_index_doc after discussing with nominam ??
//              -- jiwon, 2002/09/14

	HOOK_LINK(morp_set_paragraphtext)
	HOOK_LINK(morp_get_paragraph)
	HOOK_LINK(morp_get_paragraphlen)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,morp_set_text, \
				(Morpheme *morpObj,char *strings,char id),\
				(morpObj,strings,id),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,morp_get_wordlist,\
				(Morpheme *morpObj,WordList *wordlist,char id),\
				(morpObj,wordlist,id),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,morp_get_wordpos,\
				(Morpheme *morpObj,int16_t *pos, int16_t *bytepos),\
				(morpObj,pos,bytepos),DECLINE)
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(morp_set_paragraphtext,\
				(Paragraph *para, char *text),\
				(para,text))
SB_IMPLEMENT_HOOK_RUN_FIRST(char*,morp_get_paragraph,\
				(Paragraph *para,char flag), \
				(para,flag),(char*)-1)
SB_IMPLEMENT_HOOK_RUN_FIRST(char*,morp_get_paragraphlen,\
				(Paragraph *para, int *len),\
				(para, len),(char*)-1)
