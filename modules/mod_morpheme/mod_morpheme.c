/* $Id$ */
#include "common_core.h"
#include <stdio.h>
#include <stdlib.h>

#include "mod_api/morpheme.h"

#include "lib/old_softbot.h"
#include "lib/lb_std.h"
#include "lib/lb_lex.h"
#include "lib/ma.h"
#include "lib/mabi.h"
#include "lib/ma_code.h"

// XXX mod_indexer에서 export하는 함수들을 hook하기 위해
//     need to change..(indexer_api..??)
/*#include "mod_indexer/mod_indexer.h"*/

/* XXX: move followings to mod_api/morpheme.c ?
 *      include/morheme.h referes to mod_morpheme/lib/lb_lex.h
 *      so..
 */

static char mParagraphDelimeter[SHORT_STRING_SIZE]="\n";
static size_t mDelimeterSize = 1;
static char m_dict_path[MAX_PATH_LEN]=DB_PATH"/dic/ma.dic";

static int init(void){
	// FIXME: 
	// MAinit prints error message to stdout/stderr. 
	// change it to use logmodule
	MAinit(m_dict_path);	
/*	MABIinit(m_dict_path);*/

	return SUCCESS;
}

static int set_text(Morpheme *morpObj,char *strings,char id) {
	// XXX: temporarily last two parameter have no meaning right nos
	//      it's for comment block, .. etc. ask YoungHoon or KJB
	//
	// XXX: for bigram generator set_text.. temporarily
	if (id == 2) return DECLINE;
		
	LEXset(&(morpObj->lexHandle),&(morpObj->lex),(TBYTE*)strings,0,0); 
	return SUCCESS;
}

#if 0
static int get_bigram_wordlist(Morpheme *morp,WordList *wordlist,char id)
{
	CMA ma;
	CLexVar *lexHandle;
	int32_t i,j,ret,ix=0;
	int rv=SUCCESS;

	if (id != 2) { 
		return DECLINE; 
	}
	
	lexHandle = &(morp->lexHandle);

	ma.wMask = MA_KEY;
	
	if ( !LEXgetToks(lexHandle) ){
		ix=0;
		rv=FAIL;
		goto DONE_MORP;
	}
	debug("lex->lTokCnt (%d)",lexHandle->pLex->lTokCnt);
	
	for (i=0; i<lexHandle->pLex->lTokCnt; i++) {
		ret = MABIanal(&(lexHandle->pLex->aTok[i]),&ma,
				lexHandle->pLex->lTokCnt);
		if (ma.shCntSL == 0) {
			debug("lex->aTok[%d].szTok:[%s] is meaningless noise",
							i, lexHandle->pLex->aTok[i].szTok);
			continue;
		}

		for (j=0; j<ma.shCntSL; j++) {
			if (ix >= MAX_WORD_PER_PHRASE) {
				warn("more words in phrase than MAX_WORD_PER_PHRASE(%d)",
														MAX_WORD_PER_PHRASE);
				rv=SUCCESS;
				goto DONE_MORP;
			}

			strncpy(morp_get_word(&(wordlist->words[ix])),
							ma.aMASL[j].szWord,MAX_WORD_LEN);
			wordlist->words[ix]string[MAX_WORD_LEN-1]='\0';
			wordlist->words[ix].attribute = ma.aMASL[j].wPosTag;
			wordlist->words[ix].pos = lexHandle->pLex->lPos;
			ix++;

			debug("morp_get_word [%s]", ma.aMASL[j].szWord);
		}
	}
	rv=SUCCESS;

DONE_MORP:
	wordlist->num_of_words = ix;
	return rv;
}
#endif

static int get_wordlist(Morpheme *morp,WordList *wordlist,char id)
{
	CMA ma;
	CLexVar *lexHandle;
	int32_t i,j,ret,ix=0;
	int rv=SUCCESS;
	char buf[MAX_WORD_LEN];
	int remainsize=MAX_WORD_LEN;
	WordList _wordlist;
	_wordlist.num_of_words = 0;

	if (id != 1) { 
		return DECLINE; 
	}
	
	lexHandle = &(morp->lexHandle);

	ma.wMask = MA_KEY;
	
	if ( !LEXgetToks(lexHandle) ){
		ix=0;
		rv=FAIL;
		goto DONE_MORP;
	}

	debug("lex->lTokCnt (%d)",lexHandle->pLex->lTokCnt);
	
	for (i=0; i<lexHandle->pLex->lTokCnt; i++) {
		ret = MAanal(&(lexHandle->pLex->aTok[i]),&ma,lexHandle->pLex->lTokCnt);
		if (ma.shCntSL == 0) {
			debug("lex->aTok[%d].szTok:[%s] is meaningless noise",
							i, lexHandle->pLex->aTok[i].szTok);
			continue;
		}

		for (j=0; j<ma.shCntSL; j++) {
			if (ix >= MAX_WORD_PER_PHRASE) {
				warn("more words in phrase than MAX_WORD_PER_PHRASE(%d)",
														MAX_WORD_PER_PHRASE);
				rv=SUCCESS;
				goto DONE_MORP;
			}

			strncpy(morp_get_word(&(_wordlist.words[ix])),
							ma.aMASL[j].szWord, MAX_WORD_LEN);
			_wordlist.words[ix].word[MAX_WORD_LEN-1]='\0';
			_wordlist.words[ix].attribute = ma.aMASL[j].wPosTag;
			_wordlist.words[ix].position = lexHandle->pLex->lPos;
			ix++;

			debug("morp_get_word [%s]", ma.aMASL[j].szWord);
		}
	}
	rv=SUCCESS;

DONE_MORP:
	_wordlist.num_of_words = ix;
	
	//XXX: ABCD-> A AB ABC ABCD, B BC BCD, C CD, D
#define MAX_CON_WORD		4
	//XXX: maximum 4 word can be concatenated

	wordlist->num_of_words=0;
    if (_wordlist.num_of_words > MAX_CON_WORD) {
        DEBUG("concaternating all words ");
        buf[0] = '\0';
        remainsize = MAX_WORD_LEN;
        for (j=0; j<_wordlist.num_of_words && remainsize > 0; j++) {
            strncat(buf,_wordlist.words[j].word, remainsize);
            remainsize -= strlen(_wordlist.words[j].word);
        }
        strncpy(wordlist->words[wordlist->num_of_words].word, buf, MAX_WORD_LEN);
        wordlist->words[wordlist->num_of_words].word[MAX_WORD_LEN-1] = '\0';
        wordlist->words[wordlist->num_of_words].attribute =
                _wordlist.words[0].attribute;
        wordlist->words[wordlist->num_of_words].position =
                _wordlist.words[0].position;
        wordlist->num_of_words++;
    }

	for(i=0 ; i < _wordlist.num_of_words ; i++){
		remainsize = MAX_WORD_LEN;
		buf[0]='\0';
		for(j=i ; j<_wordlist.num_of_words && j<i+MAX_CON_WORD; j++) {
			strncat(buf, _wordlist.words[j].word, remainsize);
			remainsize -= strlen(_wordlist.words[j].word);

			if (remainsize <=0)
				break;

			strncpy(wordlist->words[wordlist->num_of_words].word, buf,MAX_WORD_LEN);
			wordlist->words[wordlist->num_of_words].word[MAX_WORD_LEN-1] = '\0';
			wordlist->words[wordlist->num_of_words].attribute = 
				_wordlist.words[j].attribute;
			wordlist->words[wordlist->num_of_words].position = 
				_wordlist.words[j].position;

			wordlist->num_of_words++;
			if (wordlist->num_of_words == MAX_WORD_PER_PHRASE)
				goto MORP_RETURN;
		}
	}
/*
#ifdef DEBUG_SOFTBOTD
	for (i=0; i<wordlist->num_of_words; i++) {
		INFO("word[%d]:|%s|",i,wordlist->words[i].word);
	}
#endif	
*/
MORP_RETURN:
	return rv;
}

static int get_wordlist_merge_all_words(Morpheme *morp,WordList *wordlist,char id)
{
	CMA ma;
	CLexVar *lexHandle;
	int32_t i,j,ret,ix=0;
	int rv=SUCCESS;
	char buf[MAX_WORD_LEN];
	int remainsize=MAX_WORD_LEN;
	uint32_t position = 0,attribute=0;

	if (id != 3) { 
		return DECLINE; 
	}
	
	lexHandle = &(morp->lexHandle);

	ma.wMask = MA_KEY;
	
	if ( !LEXgetToks(lexHandle) ){
		ix=0;
		rv=FAIL;
		goto DONE_MORP;
	}

	debug("lex->lTokCnt (%d)",lexHandle->pLex->lTokCnt);
	
	for (i=0; i<lexHandle->pLex->lTokCnt; i++) {
		ret = MAanal(&(lexHandle->pLex->aTok[i]),&ma,lexHandle->pLex->lTokCnt);
		if (ma.shCntSL == 0) {
			debug("lex->aTok[%d].szTok:[%s] is meaningless noise",
							i, lexHandle->pLex->aTok[i].szTok);
			continue;
		}

		for (j=0; j<ma.shCntSL; j++) {
			if (ix >= MAX_WORD_PER_PHRASE) {
				warn("more words in phrase than MAX_WORD_PER_PHRASE(%d)",
														MAX_WORD_PER_PHRASE);
				rv=SUCCESS;
				goto DONE_MORP;
			}

			strncpy(morp_get_word(&(wordlist->words[ix])),
							ma.aMASL[j].szWord,MAX_WORD_LEN);
			wordlist->words[ix].word[MAX_WORD_LEN-1]='\0';
			wordlist->words[ix].attribute = ma.aMASL[j].wPosTag;
			wordlist->words[ix].position = lexHandle->pLex->lPos;
			ix++;

			debug("morp_get_word [%s]", ma.aMASL[j].szWord);
		}
	}
	rv=SUCCESS;

DONE_MORP:
	wordlist->num_of_words = ix;
	//XXX: dirty hack to morp analyzer
	if (wordlist->num_of_words >= 2 ) {
		position = wordlist->words[0].position;
		attribute = wordlist->words[0].attribute;
		remainsize = MAX_WORD_LEN;
		buf[0]='\0';
		for (i=0; i<wordlist->num_of_words; i++) {
			strncat(buf, wordlist->words[i].word, remainsize);
			remainsize -= strlen(wordlist->words[i].word);

			if (remainsize <= 0) 
				break;
		}
		strncpy(wordlist->words[0].word, buf, MAX_WORD_LEN);
		wordlist->words[0].word[MAX_WORD_LEN-1] = '\0';
		wordlist->words[0].position = position;
		wordlist->words[0].attribute = attribute;
		wordlist->num_of_words=1;
	}
	return rv;
}

static int get_nexttoken(Morpheme *morp,WordList *wordlist,char id)
{
	CLexVar *lexHandle;
	int i, left;

	if (id != 0) { 
		return DECLINE; 
	}

	lexHandle = &(morp->lexHandle);

	if ( !LEXgetToks(lexHandle) ){
		wordlist->num_of_words = 0;
		return FAIL;
	}

    left = MAX_WORD_LEN;
    wordlist->words[0].word[0] = '\0';
    for (i=0; i<lexHandle->pLex->lTokCnt; i++) {
        strncat(wordlist->words[0].word, lexHandle->pLex->aTok[i].szTok, left>0?left:0);
        left -= strlen(lexHandle->pLex->aTok[i].szTok);
    }

    wordlist->words[0].position = lexHandle->pLex->lPos;
    wordlist->num_of_words = 1;
		
	return SUCCESS;
}

static int get_nexttoken_chn2kor_converted(Morpheme *morp,WordList *wordlist,char id)
{
    CLexVar *lexHandle;
    char szTmp[MA_WORD_LEN + 4];
    int i, left;

    if (id != 4) {
        return DECLINE;
    }

    lexHandle = &(morp->lexHandle);

    if ( !LEXgetToks(lexHandle) ){
        wordlist->num_of_words = 0;
        return FAIL;
    }

    left = MAX_WORD_LEN;
    wordlist->words[0].word[0] = '\0';
    wordlist->words[1].word[0] = '\0';
    for (i=0; i<lexHandle->pLex->lTokCnt; i++) {
        strncat(wordlist->words[0].word, lexHandle->pLex->aTok[i].szTok, left>0?left:0);

        if (lexHandle->pLex->aTok[i].byTokID == TOK_CHN) {
            //XXX: boundary check in CDconvChn2Kor of second parameter
            if (!CDconvChn2Kor(lexHandle->pLex->aTok[i].szTok, szTmp))
                break;
            strncat(wordlist->words[1].word, szTmp, left>0?left:0);
            left -= strlen(szTmp);
        }
        else {
            strncat(wordlist->words[1].word, lexHandle->pLex->aTok[i].szTok, left>0?left:0);
            left -= strlen(lexHandle->pLex->aTok[i].szTok);
        }
    }

    wordlist->words[0].position = lexHandle->pLex->lPos;
    wordlist->words[1].position = lexHandle->pLex->lPos;
    wordlist->num_of_words = 2;

    return SUCCESS;
}

static int get_wordpos(Morpheme *morp, int16_t *pos, int16_t *bytepos)
{
	CLexVar *lexHandle;
	
	lexHandle = &(morp->lexHandle);

	if ( !LEXgetToks(lexHandle) ){
		return FAIL;
	}

	*pos = lexHandle->pLex->lPos;
	*bytepos = lexHandle->pLex->lBytePos;
	return SUCCESS;
}

void set_paragraphtext(Paragraph *para, char *text)
{
	para->next = text;
}

char* get_allparagraph(Paragraph *para, char flag)
{
	char *nextparagraph=para->next;

	if (flag != 0) { return (char *)-1; } /* DECLINING */

	para->next=NULL;
	
	return nextparagraph;
}

char* get_paragraph(Paragraph *para, char flag)
{
	char *tmp=NULL;
	char *nextparagraph=para->next;

	if (flag != 1) { return (char *)-1; } /* DECLINING */

	if (para->next == NULL) {
		return NULL;
	}

	debug("before strstr: %s",para->next);
	tmp = strstr(para->next,mParagraphDelimeter);
	if (tmp == NULL) {
		para->next=NULL;
	}
	else {
		*tmp = '\0';
		tmp += mDelimeterSize;
		for (; LEXM_IS_WHITE(*tmp); tmp++);
		para->next = tmp;
		debug("strstred pointer:%s",tmp);
	}
	
	return nextparagraph;
}

char* get_paragraphlen(Paragraph *para, int *len)
{
	char *tmp=NULL;
	char *nextparagraph=para->next;

	if (para->next == NULL) {
		return NULL;
	}

	debug("before strstr: %s",para->next);
	tmp = strstr(para->next,mParagraphDelimeter);
	if (tmp == NULL) {
		*len = (int)(tmp - para->next);
		para->next=NULL;
	}
	else {
		*len = (int)(tmp - para->next);
		tmp += mDelimeterSize;
		for (; LEXM_IS_WHITE(*tmp); tmp++);
		para->next = tmp;
		debug("strstred pointer:%s",tmp);
	}
	
	return nextparagraph;
}

// XXX: obsolete
#if 0
#define returnIfOverflow(index,length) { if(index >= length) \
											return OVERFLOW; }
#define returnIfNotInitialized() { if (m_isMorphemeInitialized == 0)\
										{return FAIL;} }
int MA_get(char aWord[], Word words[], int maxItem){
	CLex lex;
	CMA ma;
	Int32 i,j,ret,ix=0;

	// XXX last two parameter have no meaning
	LEXset(&lex,(TBYTE*)aWord,0,0); 
	ma.wMask = MA_KEY;

	while (1) {
		if ( !LEXgetToks(&lex) )
			break;

		for (i=0; i<lex.lTokCnt; i++) {
			ret = MAanal(&(lex.aTok[i]),&ma,lex.lTokCnt);
			if (ma.shCntSL != 0){
				for (j=0; j<ma.shCntSL; j++) {
					if (ix >= maxItem) 
						return OVERFLOW;

					strncpy(words[ix].word,ma.aMASL[j].szWord,MAX_WORD_LEN);
					words[ix].word[MAX_WORD_LEN-1]='\0';
					words[ix].attribute = ma.aMASL[j].wPosTag;
					ix++;
				}
			}
		}
	}

	return ix;	// never reaches		
}
#undef returnIfOverflow
#undef returnIfNotInitialized
#endif

static void register_hooks(void)
{
	sb_hook_morp_set_text(set_text,NULL,NULL,HOOK_MIDDLE);

	sb_hook_morp_get_wordlist(get_wordlist,NULL,NULL,HOOK_MIDDLE);
	sb_hook_morp_get_wordlist(get_wordlist_merge_all_words,
												NULL,NULL,HOOK_MIDDLE);
	sb_hook_morp_get_wordlist(get_nexttoken,NULL,NULL,HOOK_MIDDLE);
	sb_hook_morp_get_wordlist(get_nexttoken_chn2kor_converted,NULL,NULL,HOOK_MIDDLE);

	sb_hook_morp_get_wordpos(get_wordpos,NULL,NULL,HOOK_MIDDLE);
	sb_hook_morp_set_paragraphtext(set_paragraphtext,NULL,NULL,HOOK_MIDDLE);

	sb_hook_morp_get_paragraph(get_paragraph,NULL,NULL,HOOK_MIDDLE);
	sb_hook_morp_get_paragraph(get_allparagraph, NULL, NULL, HOOK_MIDDLE);

	sb_hook_morp_get_paragraphlen(get_paragraphlen,NULL,NULL,HOOK_MIDDLE);
}

static void set_dict_path(configValue v)
{
	strncpy(m_dict_path,v.argument[0],MAX_PATH_LEN);
	m_dict_path[MAX_PATH_LEN-1] = '\0';
	debug("dictionary path: %s",m_dict_path);
}

static void set_paragraph_delimiter(configValue v)
{
	//XXX:
	if (strncmp(v.argument[0],"\\n",SHORT_STRING_SIZE) == 0) {
		strncpy(mParagraphDelimeter,"\n",SHORT_STRING_SIZE);
		mParagraphDelimeter[SHORT_STRING_SIZE-1]='\0';
		mDelimeterSize = 1;
	}
	else if (strncmp(v.argument[0],"\\n\\n",SHORT_STRING_SIZE) == 0) {
		strncpy(mParagraphDelimeter,"\n\n",SHORT_STRING_SIZE);
		mParagraphDelimeter[SHORT_STRING_SIZE-1]='\0';
		mDelimeterSize = 2;
	}
	else {
		strncpy(mParagraphDelimeter,v.argument[0],SHORT_STRING_SIZE);
		mParagraphDelimeter[SHORT_STRING_SIZE-1]='\0';
		mDelimeterSize = strlen(mParagraphDelimeter);
	}
	debug("paragraph delimeter is [%s]",mParagraphDelimeter);
}

static config_t config[] = {
	CONFIG_GET("DICTIONARY_PATH",set_dict_path,1,\
				"dic file path."),
	CONFIG_GET("ParagraphDelimiter",set_paragraph_delimiter,1,\
			"Delimeter of paragraph. default is \\n\\n"),
	{NULL}
};

module morpheme_module = {
    STANDARD_MODULE_STUFF,
    config,                 /* config */
    NULL,                   /* registry */
	init,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
