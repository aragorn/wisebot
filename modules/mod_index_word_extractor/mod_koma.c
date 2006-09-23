/* $Id$ */
#include "common_core.h"
#include "memory.h"

#include "mod_api/index_word_extractor.h"
#include "mod_koma.h"

#include <string.h>
#include <errno.h>

// XXX: 인터넷에서는 기본적으로 단음절 변형을 사용하지 않는다.  -- dyaus
#define MODE_TREAT_JUPDUSA 1 // 접두사 처리
#define MODE_TREAT_JUPMISA 1 // 접미사 처리

#define MY_EXTRACTOR_ID 	10
#define MY_RAW_EXTRACTOR_ID 100

static int HAS_LOADED_KOMA_ENGINE = 0;
static int HAS_LOADED_HANTAG_ENGINE = 0;

static char MAIN_FST_FILE[SHORT_STRING_SIZE] = "share/koma/main.FST";
static char MAIN_DAT_FILE[SHORT_STRING_SIZE] = "share/koma/main.dat";
static char CONNECTION_FILE[SHORT_STRING_SIZE] = "share/koma/connection.txt";
static char TAG_FILE[SHORT_STRING_SIZE]      = "share/koma/tag.nam";
static char TAGOUT_FILE[SHORT_STRING_SIZE]   = "share/koma/tagout.nam";

static char PROB_FST_FILE[SHORT_STRING_SIZE] = "share/koma/prob.FST";
static char PROB_DAT_FILE[SHORT_STRING_SIZE] = "share/koma/prob.dat";

static int load_koma_engine();
static int load_hantag_engine();

koma_handle_t* new_koma()
{
	koma_handle_t *koma = NULL;

	if (! HAS_LOADED_KOMA_ENGINE)   load_koma_engine();
	if (! HAS_LOADED_HANTAG_ENGINE) load_hantag_engine();

	koma = sb_calloc(1, sizeof(koma_handle_t));
	if (koma == NULL) {
		crit("failed to malloc koma_handle");
		return NULL;
	}

	koma->HanTag = CreateHanTag();
	if (koma->HanTag == NULL) {
		crit("cannot make HanTag instance");
		sb_free(koma);
		return NULL;
	}

	return koma;
}


static index_word_extractor_t* new_koma_analyzer(int id)
{
	index_word_extractor_t *extractor = NULL;
	koma_handle_t *handle=NULL;

	if (id != MY_EXTRACTOR_ID && id != MY_RAW_EXTRACTOR_ID)
		return (index_word_extractor_t*)MINUS_DECLINE;

	extractor = sb_calloc(1, sizeof(index_word_extractor_t));
	if (extractor == NULL) {
		crit("cannot allocate index word extractor object");
		return NULL;
	}

	handle = new_koma();
	if (handle == NULL) {
		crit("cannot allocate koma_handle_t object");
		sb_free(extractor);
		return NULL;
	}

	extractor->handle = handle;
	extractor->id = id;

	if (extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("extractor pointer is -99 (same as the MINUS_DECLINE)");
	}

	return extractor;
}

// XXX koma 에서 한번에 처리할 수 있는 문자열 길이로 분할한다.
static void move_text(koma_handle_t *handle)
{
	int k, i, cnt=0;
	char *text;

	text = handle->next_text;

	if (handle->next_length > MOD_KOMA_SENTENCE_LEN) { 
		k = MOD_KOMA_SENTENCE_LEN;

		while ( (! IS_WHITE_CHAR(text[k]) ) && k > 0) { 
			k--; 
		}

		if (k == 0) {
			// 한글이 잘리지 않게... chaeyk
			cnt = 0;
			for( i = MOD_KOMA_SENTENCE_LEN; i >= 0; i-- ) {
				if ( text[i] & 0x80 ) cnt++;
				else break;
			}

			if ( cnt % 2 ) k = MOD_KOMA_SENTENCE_LEN;
			else k = MOD_KOMA_SENTENCE_LEN - 1;

			handle->next_text = &(text[k+1]);
			if ( text[k] & 0x80 && text[k-1] & 0x80 ) text[k-1] = '\0';
			text[k] = '\0';
			handle->text = text;
		} else {
			handle->next_text = &(text[k+1]);
			text[k] = '\0';
			handle->text = text;
		}
	
		handle->current_length = k;
		handle->next_length = strlen(handle->next_text);

		if (handle->next_length <= 0) {
			handle->next_text = NULL;
			handle->next_length = 0;
		}

	} else {
		handle->text = text;
		handle->next_text = NULL;
		handle->next_length = 0;
	}
}

// XXX 분석할 문자열의 위치를 지정한다.
void koma_set_text(koma_handle_t* handle, char* text)
{
	handle->next_length = strlen(text);

	// set handle
	handle->orig_text = text;
	handle->position = 1;
	handle->current_index = 0;
	handle->current_bytes_position = 0;
	handle->next_text = text;
	handle->text = text;

	move_text(handle);
	handle->koma_done = FALSE;
}

static int _koma_set_text(index_word_extractor_t* extractor, char* text)
{
	koma_handle_t *handle = NULL;

	if (extractor->id != MY_EXTRACTOR_ID && extractor->id != MY_RAW_EXTRACTOR_ID)
		return DECLINE;

	handle = extractor->handle;

	koma_set_text(handle, text);

	return SUCCESS;
}

// XXX 분석될 결과에서 형태소와 품사를 분리한다.
static char* koma_get_token_and_pumsa( char *str, char *tok, char *tag)
{
	int i,j;

	tag[0] = tok[0] = '\0';

	for(i=0; *str != '/'; str++, i++ ) {

		if (*str=='\0') {
			tok[i] = '\0';
			return NULL;
		}

		tok[i] = *str;
	}

	tok[i] = '\0';

	str++;

	for(j=0; j<KOMA_TAG_LEN && *str != '+'; str++, j++ ) {

		if (*str=='\0') 
			return NULL;

		tag[j] = *str;
	}
	
	str++;

	// XXX '/' 경우에 null str 이 생성되게 된다.
	if (!(strncmp(tag, "/SS/", 4))) {
		*tok = '/';
		*(tok+1) = '/';
		strncpy(tag, "SS/", 3);
	}

	if (*str=='\0')
		return NULL;

	return str;

}

int koma_analyze(koma_handle_t *handle, index_word_t *out, int max)
{
    int i;
	int token_len , *cur_pos;
	int	idx_of_index_word=0;
	int previous_idx_of_index_word;
	int	num_of_cut=0, jupdusa_exist;

	char temp_string[LONG_STRING_SIZE];
	char token_string[LONG_STRING_SIZE];
#ifdef MODE_TREAT_JUPDUSA
	char jupdusa_string[LONG_STRING_SIZE];
#endif
	char *ptemp_string;
	char tag[KOMA_TAG_LEN+1];
	koma_handle_t *h=NULL;

	h = handle;

	cur_pos = &(h->position);
	if (h->text == NULL) {
		return 0;
	}

	if ( h->koma_done == FALSE) { 
		info( "h->text: %s", h->text );
		DoKomaAndHanTag(h->HanTag,
						PATH_BASED_TAGGING, h->text, h->Wrd, h->bPos, h->Result);
		h->koma_done = TRUE;
		h->result_count = 0;
		for(i=0;i<MAX_NUM_OF_MORPHEMES && h->Wrd[i];i++) 
			h->result_count++;

	}

	for (i = h->current_index; i< h->result_count; i++) {

		// XXX: koma 결과에 버그 있음. 에러 안나고 넘어가게만 조치. 나중에라도 수정되야함. 

		if (h->Result[i][0] == 0x00)
			continue; // skip

		strncpy(temp_string, h->Result[i][0], LONG_STRING_SIZE);
		temp_string[LONG_STRING_SIZE-1]='\0';

		debug("index[%d] temp_string[%s]", i, temp_string);
		ptemp_string = temp_string;

		previous_idx_of_index_word = idx_of_index_word; // roll back

		for (num_of_cut=0, jupdusa_exist=FALSE;
			 ptemp_string != NULL;
			 num_of_cut++)
		{
			memset(tag, 0x00, KOMA_TAG_LEN+1); /* initialize 품사 tag */
			ptemp_string = 
					koma_get_token_and_pumsa(ptemp_string, token_string, tag);

			token_len = strlen(token_string);
            debug("num_of_cut[%d], token_string[%s] tag[%s]", num_of_cut, token_string, tag);

            if(h->is_raw_koma_text) {
                strncpy(out[idx_of_index_word].word, token_string, MAX_WORD_LEN);
                out[idx_of_index_word].word[MAX_WORD_LEN-1] = '\0';
                out[idx_of_index_word].len = strlen(out[idx_of_index_word].word);
                out[idx_of_index_word].pos = *cur_pos;
                memcpy(&(out[idx_of_index_word].attribute), tag,
                                    sizeof(out[idx_of_index_word].attribute));
                
                out[idx_of_index_word].bytepos = h->current_bytes_position + h->bPos[i];
				idx_of_index_word++;
            }
#ifdef MODE_TREAT_JUPDUSA
			// 접두사 전처리 
			else if ( num_of_cut == 0 && TAG_IS_JUPDUSA(tag, token_len) ) {
				// XXX: length of jupdusa is always 2. we can assert!!!
				strncpy(jupdusa_string, token_string, token_len);
				jupdusa_string[token_len] = '\0';
				jupdusa_exist = TRUE;
			} else
			// 접두사 후처리
			if ( num_of_cut == 1 && jupdusa_exist == TRUE ) {
				int tmp_len=0;

				// XXX: 연결할 단어의 품사를 확인해서,
				// 기존의 색인어인 경우에는 앞어절을 연결한 색인어를 만든다.
				if (!(TAG_TO_BE_IGNORED(tag)) ) {
					tmp_len = strlen(jupdusa_string);
					strncat(jupdusa_string, token_string, LONG_STRING_SIZE-tmp_len);
					jupdusa_string[LONG_STRING_SIZE-1]='\0';
					token_len = strlen(jupdusa_string);

					jupdusa_exist = FALSE;

					strncpy(out[idx_of_index_word].word,
							jupdusa_string, MAX_WORD_LEN);
					out[idx_of_index_word].word[MAX_WORD_LEN-1] = '\0';
					out[idx_of_index_word].len =
							strlen(out[idx_of_index_word].word);
					out[idx_of_index_word].pos = *cur_pos;
					// XXX: sizeof(int) is must be 4 byte
					memcpy(&(out[idx_of_index_word].attribute),
						   tag, sizeof(out[idx_of_index_word].attribute));
					out[idx_of_index_word].bytepos =  h->current_bytes_position + h->bPos[i];
					idx_of_index_word++;
				} else 
				// 문장부호 & 조사 처리 
				// 단음절 색인어를 만들고, 조사나 문장부호는 버린다.
				if ( TAG_IS_JOSA(tag) || TAG_IS_MUNJANGBUHO(tag) ){
					strncpy(out[idx_of_index_word].word,
							jupdusa_string, MAX_WORD_LEN);
					out[idx_of_index_word].word[MAX_WORD_LEN-1] = '\0';
					out[idx_of_index_word].len =
							strlen(out[idx_of_index_word].word);

					out[idx_of_index_word].pos = *cur_pos;

					jupdusa_exist = FALSE;

					// 품사 정보 처리 : FIXME
					memcpy(&(out[idx_of_index_word].attribute),
						   "NNCG", sizeof(out[idx_of_index_word].attribute));
					// byteposition 처리 : FIXME
					out[idx_of_index_word].bytepos 
							=  h->current_bytes_position + h->bPos[i] - 1;
					idx_of_index_word++;
				}
				else {
				// 분리된 단음절과 다음 token을 버린다.
					out[idx_of_index_word].word[0] = '\0';
					out[idx_of_index_word].len = 0;
					jupdusa_string[0]='\0';
					jupdusa_exist = FALSE;
				}
			} else
#endif
#ifdef MODE_TREAT_JUPMISA
			// 접미사 처리

			if ( idx_of_index_word > 0 && TAG_IS_JUPMISA(tag, token_len)
                        && out[idx_of_index_word-1].pos == *cur_pos) {

				int len = MAX_WORD_LEN - out[idx_of_index_word-1].len;
				if (len < 0) {
					crit("length of word[%s] is larger than MAX_WORD_LEN[%d]",
							out[idx_of_index_word-1].word, MAX_WORD_LEN);
					len = 0;
				}
				strncat(out[idx_of_index_word-1].word , token_string , len);
				out[idx_of_index_word-1].word[MAX_WORD_LEN-1]='\0';
				out[idx_of_index_word-1].len =
									strlen(out[idx_of_index_word-1].word);
			} else
#endif
			// 그외 품사 처리 
			// 색인단어에서 제외할 품사들
			// TAG_TO_BE_IGNORED_MORE or TAG_TO_BE_IGNORED
			if ( ! TAG_TO_BE_IGNORED(tag) ) {
				/* 색인단어에서 제외할 품사들이 아니면 ... */
				strncpy(out[idx_of_index_word].word,
						token_string, MAX_WORD_LEN);
				out[idx_of_index_word].word[MAX_WORD_LEN-1] = '\0';
				out[idx_of_index_word].len =
										strlen(out[idx_of_index_word].word);
				out[idx_of_index_word].pos = *cur_pos;
				memcpy(&(out[idx_of_index_word].attribute), tag,
									sizeof(out[idx_of_index_word].attribute));

				out[idx_of_index_word].bytepos = h->current_bytes_position + h->bPos[i];
				if (idx_of_index_word>max)
					CRIT("idx_of_index_word : wrong value %d [%s]", max, token_string);

				idx_of_index_word++;
			} else {
            	debug("[%s]/[%s] is ignored.", token_string, tag);
			}

			if ( idx_of_index_word >= max ) {
				if (previous_idx_of_index_word == 0) {
					crit("too small max_index_word_size[%d]!", max);
					idx_of_index_word = max;
					h->current_index = i+1;
				}
				else {
					h->current_index = i;
					idx_of_index_word = previous_idx_of_index_word;
				}

				goto FINISH;
			} 
	
		} // for num_of_cut

		// 접두사 전처리후 다음이 없으면 색인어로 처리한다.
		// 닭, 비 등이 이것에 의해 색인어가 된다.
		if ( jupdusa_exist == TRUE ) {
			strncpy(out[idx_of_index_word].word,
					jupdusa_string, MAX_WORD_LEN);
			out[idx_of_index_word].word[MAX_WORD_LEN-1] = '\0';
			out[idx_of_index_word].len =
					strlen(out[idx_of_index_word].word);

			out[idx_of_index_word].pos = *cur_pos;

			jupdusa_exist = FALSE;

			// 품사 정보 처리 : FIXME
			memcpy(&(out[idx_of_index_word].attribute),
				   "NNCG", sizeof(out[idx_of_index_word].attribute));
			// byteposition 처리 : FIXME
			out[idx_of_index_word].bytepos 
					=  h->current_bytes_position + h->bPos[i] - 1;
			idx_of_index_word++;
		}

		(*cur_pos)++;	

	} // for koma_result

	h->current_index = i;

	h->current_bytes_position = h->next_text - h->orig_text;
//	h->current_bytes_position += h->current_length;

FINISH:

	if ( i >= h->result_count ) {
		move_text(h);
		h->koma_done = FALSE;
	 	h->current_index = 0;
 	}

	return idx_of_index_word;

}

static int _koma_analyze(index_word_extractor_t *extractor, index_word_t *indexwords, int max)
{
    // XXX: why not just DECLINE?.. 2002/11/15
    if(extractor->id != MY_EXTRACTOR_ID && extractor->id != MY_RAW_EXTRACTOR_ID) return MINUS_DECLINE;

    if(extractor->id == MY_RAW_EXTRACTOR_ID) {
        ((koma_handle_t*)extractor->handle)->is_raw_koma_text = 1;
    }

    return koma_analyze(extractor->handle, indexwords, max);
}

void delete_koma(koma_handle_t *handle)
{
	FreeHanTag(handle->HanTag);
	sb_free(handle);
}
	
static int delete_koma_analyzer(index_word_extractor_t *extractor)
{
	if (extractor->id != MY_EXTRACTOR_ID && extractor->id != MY_RAW_EXTRACTOR_ID) return DECLINE;

	delete_koma(extractor->handle);

	sb_free(extractor);

	return SUCCESS;
}

static int load_koma_engine()
{
	char fstfile[STRING_SIZE];
	char dictfile[STRING_SIZE];
	char connection[STRING_SIZE];
	char tag_file[STRING_SIZE];
	char tagout_file[STRING_SIZE];

	if (HAS_LOADED_KOMA_ENGINE) return SUCCESS;

	snprintf(fstfile,     STRING_SIZE, "%s/%s", gSoftBotRoot, MAIN_FST_FILE);
	snprintf(dictfile,    STRING_SIZE, "%s/%s", gSoftBotRoot, MAIN_DAT_FILE);
	snprintf(connection,  STRING_SIZE, "%s/%s", gSoftBotRoot, CONNECTION_FILE);
	snprintf(tag_file,    STRING_SIZE, "%s/%s", gSoftBotRoot, TAG_FILE);
	snprintf(tagout_file, STRING_SIZE, "%s/%s", gSoftBotRoot, TAGOUT_FILE);

	if (LoadKomaEngine(fstfile, dictfile, connection, tag_file, tagout_file) == 0) {
		crit("cannot load KOMA engine: %s", strerror(errno));
		return FAIL;
	}

	HAS_LOADED_KOMA_ENGINE = 1;
	return SUCCESS;
}

static int load_hantag_engine()
{
	char prob_fst[STRING_SIZE];
	char prob_dat[STRING_SIZE];

	if (HAS_LOADED_HANTAG_ENGINE) return SUCCESS;

	snprintf(prob_fst, STRING_SIZE, "%s/%s", gSoftBotRoot, PROB_FST_FILE);
	snprintf(prob_dat, STRING_SIZE, "%s/%s", gSoftBotRoot, PROB_DAT_FILE);

    if (LoadHanTagEngine(prob_fst, prob_dat) == 0) {
		crit("cannot load HanTag engine: %s", strerror(errno));
		return FAIL;
	}

	HAS_LOADED_HANTAG_ENGINE = 1;
	return SUCCESS;
}

/** config stuff **/
static config_t config[] = {
/*
    CONFIG_GET("MainFstFile", setMainFstFile, 1, "main.FST file"),
    CONFIG_GET("MainDatFile", setMainDatFile, 1, "main.dat file"),
    CONFIG_GET("ConnectionFile", setConnectionFile, 1, "connection.txt file"),
    CONFIG_GET("TagFile", setTagFile, 1, "tag.nam file"),
    CONFIG_GET("TagOutFile", setTagOutFile, 1, "tagout.nam file"),
    CONFIG_GET("ProbFstFile", setProbFstFile, 1, "prob.FST file"),
    CONFIG_GET("ProbDatFile", setProbDatFile, 1, "prob.dat file"),
*/
	{ NULL }
};

static void register_hooks(void)
{
	sb_hook_new_index_word_extractor(new_koma_analyzer, 
												NULL, NULL, HOOK_MIDDLE);
	sb_hook_index_word_extractor_set_text(_koma_set_text, 
												NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_index_words(_koma_analyze, NULL, NULL, HOOK_MIDDLE);
	sb_hook_delete_index_word_extractor(delete_koma_analyzer, 
												NULL, NULL, HOOK_MIDDLE);
}

module koma_module = {
    STANDARD_MODULE_STUFF,
    config, 	            /* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};

