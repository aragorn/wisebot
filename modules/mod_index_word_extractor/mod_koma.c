/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "memory.h"

#include "mod_api/index_word_extractor.h"
#include "mod_koma.h"

#include <string.h>
#include <errno.h>

// XXX: ���ͳݿ����� �⺻������ ������ ������ ������� �ʴ´�.  -- dyaus
#define MODE_TREAT_XPNN 1 // ���λ� ó��
#define MODE_TREAT_XSNN 1 // ���̻� ó��

#define MY_EXTRACTOR_ID 	10
#define MY_RAW_EXTRACTOR_ID 100

static int HAS_LOADED_KOMA_ENGINE = 0;
static int HAS_LOADED_HANTAG_ENGINE = 0;

static int DO_TREAT_JUPDUSA = 1;
#define MAX_NUM_JUPDUSA (32)
static int  JUPDUSA_COUNT = 0;
static char JUPDUSA_LIST[MAX_NUM_JUPDUSA][5];

static int DO_TREAT_JUPMISA = 1;
#define MAX_NUM_JUPMISA (64)
static int  JUPMISA_COUNT = 0;
static char JUPMISA_LIST[MAX_NUM_JUPMISA][5];

static int TAGGING_METHOD = PATH_BASED_TAGGING;
static int MAX_SENTENCE_LENGTH = MOD_KOMA_SENTENCE_LEN;

static char MAIN_FST_FILE[SHORT_STRING_SIZE] = "share/koma/main.FST";
static char MAIN_DAT_FILE[SHORT_STRING_SIZE] = "share/koma/main.dat";
static char CONNECTION_FILE[SHORT_STRING_SIZE] = "share/koma/connection.txt";
static char TAG_FILE[SHORT_STRING_SIZE]      = "share/koma/tag.nam";
static char TAGOUT_FILE[SHORT_STRING_SIZE]   = "share/koma/tagout.nam";

static char PROB_FST_FILE[SHORT_STRING_SIZE] = "share/koma/prob.FST";
static char PROB_DAT_FILE[SHORT_STRING_SIZE] = "share/koma/prob.dat";

static int load_koma_engine();
static int load_hantag_engine();
static void *HanTag = NULL;
static koma_handle_t *KomaHandle = NULL;

/* HanTag object�� singleton ���� ����ϵ��� �Ѵ�. index_word_extractor��
 * �Բ� �����ǰ� ����Ǵ� ���, ������尡 ����� ũ��. --2006-10-09 ������
 */
koma_handle_t* new_koma()
{
	if (! HAS_LOADED_KOMA_ENGINE)   load_koma_engine();
	if (! HAS_LOADED_HANTAG_ENGINE) load_hantag_engine();

	if (KomaHandle != NULL) return KomaHandle;
	
	KomaHandle = sb_calloc(1, sizeof(koma_handle_t));
	if (KomaHandle == NULL) {
		crit("failed to malloc koma_handle: %s", strerror(errno));
		return NULL;
	}

	if (HanTag == NULL) HanTag = CreateHanTag();
	if (HanTag == NULL) {
		crit("cannot make HanTag instance: %s", strerror(errno));
		return NULL;
	}
	KomaHandle->HanTag = HanTag;
	debug("koma[%p]->HanTag[%p] is created.", KomaHandle, KomaHandle->HanTag);

	return KomaHandle;
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

static int exists_in(char list[][5], int size, const char* str)
{
	int i;
	for (i = 0; i < size; i++)
	{
		if (strncmp(list[i], str, 5) == 0) return 1;
	}
	return 0;
}

static const char *find_newline(const char *start, const char* end)
{
	while (start < end)
	{
		if (*start == '\0') return start;
		if (*start == '\n') return start+1;
		++start;
	}

	return NULL;
}

static const char *find_sentence_end(const char *start, const char* end)
{
	while (start < end)
	{
		++start;
		if (IS_WHITE_CHAR(*start) && *(start -1) == '.') return start+1;
		if (IS_WHITE_CHAR(*start) && *(start -1) == '?') return start+1;
		if (IS_WHITE_CHAR(*start) && *(start -1) == '!') return start+1;

		if (*start == '\0') return start;
	}

	return NULL;
}

static const char *find_whitespace(const char *start, const char *end)
{
	while (start < end)
	{
		if (IS_WHITE_CHAR(*end)) return end;
		--end;
	}

	return NULL;
}

static const char *find_last_hangul(const char *start, const char *end)
{
	int count = 0;
	while (start < end - count)
	{
		if ( *(end - count) & 0x80 ) count++;
		else break;
	}

	if ( count % 2 ) return end+1;
	else return end;
}

#define VERY_SHORT (32)
static char *dbg_str_begin(char *buf, const char *s)
{
	char *c;
	strnhcpy(buf, s, VERY_SHORT);
	for (c = buf; (c = strchr(c, '\n')); ) *c = ' ';
	for (c = buf; (c = strchr(c, '\r')); ) *c = ' ';
	
	return buf;
}

static char *dbg_str_end(char *buf, const char *s)
{
	char *c;
	int  len = strlen(s);
	if (len < VERY_SHORT) {
		strnhcpy(buf, s, VERY_SHORT);
	} else {
		char check_hangul[VERY_SHORT+1];
		strnhcpy(check_hangul, s+len-VERY_SHORT, 5);
		strnhcpy(buf, s+len-VERY_SHORT+strlen(check_hangul), VERY_SHORT);
		/*
		CRIT("s[%p]+len[%d]-VERY_SHORT[%d]+strlen(check_hangul)[%d:%s]+1",
				buf, len, VERY_SHORT, strlen(check_hangul), check_hangul);
		*/
	}
	for (c = buf; (c = strchr(c, '\n')); ) *c = ' ';
	for (c = buf; (c = strchr(c, '\r')); ) *c = ' ';

	return buf;
}

/* koma ���� �ѹ��� ó���� �� �ִ� ���ڿ� ���̷� �����Ѵ�. */
static void move_text(koma_handle_t *h)
{
	char *c;
	const char *start = h->next_text;
	const char *end   = start + MAX_SENTENCE_LENGTH;
	const char *test = NULL;
	char b[SHORT_STRING_SIZE+1], e[SHORT_STRING_SIZE+1];

	if (h->next_text == NULL || strlen(h->next_text) == 0)
	{
		debug("no more text");
		h->text[0] = '\0';
		return;
	}

	if      ((test = find_newline(start, end)) != NULL)      { end = test; debug("newline"); }
	else if ((test = find_sentence_end(start, end)) != NULL) { end = test; debug("sentence_end"); }
	else if ((test = find_whitespace(start, end)) != NULL)   { end = test; debug("whitespace"); }
	else if ((test = find_last_hangul(start, end)) != NULL)  { end = test; debug("last_hangul"); }

	strnhcpy(h->text, start, end-start);
	h->text_length = strlen(h->text);
	c = h->text + h->text_length;
	if (*c == '\r') *c = ' ';
	h->byte_position += h->text_length;

	h->next_text = start + h->text_length;
	h->next_length = strlen(h->next_text);

	h->result_index = 0;
	h->result_count = 0;

	debug("text[%p:(%s)~(%s)]", h->text, dbg_str_begin(b, h->text), dbg_str_end(e, h->text));
	debug("next_text[%p:(%s)~(%s)]",
			h->next_text, dbg_str_begin(b, h->next_text), dbg_str_end(e, h->next_text));
	if (h->next_length == 0) h->next_text = NULL;
}

/* ���¼Һм��� �ؽ�Ʈ�� �����Ѵ�. */
void koma_set_text(koma_handle_t* handle, const char* text)
{

	// set handle
	handle->orig_text = text;
	handle->position = 1;
	handle->byte_position = 0;
	handle->next_text = handle->orig_text;
	handle->next_length = strlen(handle->next_text);
	handle->text[0] = '\0';

	handle->koma_done = TRUE;
}

static int _koma_set_text(index_word_extractor_t* extractor, const char* text)
{
	koma_handle_t *handle = NULL;

	if (extractor->id != MY_EXTRACTOR_ID && extractor->id != MY_RAW_EXTRACTOR_ID)
		return DECLINE;

	handle = extractor->handle;

	koma_set_text(handle, text);

	return SUCCESS;
}

// "���¼�/ǰ��"�� ���ڿ����� ���¼ҿ� ǰ�縦 �и��Ѵ�.
static char* split_morph_tag_pair(char *str, char *morph, char *tag)
{
	char *slash, *plus;

	morph[0] = tag[0] = '\0';

	slash = strchr(str, '/');
	if ( slash == NULL ) return NULL; /* '/'�� ã�� �� ����. */
	for( ; str < slash; str++, morph++ ) { *morph = *str; }
	*morph = '\0';
	str++;


	plus = strchr(str, '+');
	if ( plus )
	{
		for( ; str < plus; str++, tag++ ) { *tag = *str; }
		*tag = '\0';
		str++;
		return str;
	} else {
		strncpy(tag, str, KOMA_TAG_LEN);
		tag[KOMA_TAG_LEN] = '\0';
		return NULL;
	}
}

int koma_analyze(koma_handle_t *h, index_word_t *out, int max)
{
    int idx;
	int	idx_of_index_word = 0;
	int previous_idx_of_index_word;

AGAIN:
/*
	if (h->text == NULL || h->text[0] == '\0') {
		return 0;
	}
*/
	/* ���� DoKomaAndHanTag()�� ����� ��� return�� ��� */
	if ( h->koma_done == TRUE ) {
		char b[SHORT_STRING_SIZE+1], e[SHORT_STRING_SIZE+1];
		move_text(h);
		debug("h->text[%p] [%s]~[%s]", h->text, dbg_str_begin(b, h->text), dbg_str_end(e, h->text));
		/*              �ڵ�       �±���        �ؽ�Ʈ, �������, ����Ʈ��ġ, ������ */
		h->result_count = DoKomaAndHanTag(h->HanTag, TAGGING_METHOD, h->text, h->Wrd, h->bPos, h->Result);
		h->koma_done = FALSE; /* Result �迭�� ���� ��� index_word_t* out ���� ��������, 
								 h->koma_done = TRUE �� �ȴ�. */
	}

	/* ���¼� �м� ����� �� ���������� ������ ����. */
	for (idx = h->result_index; idx < h->result_count; idx++, h->position++) {
		int morpheme_count;
		char morph_tag_pairs[LONG_STRING_SIZE+1]; /* �� ���ڿ��� STRING_SIZE�� ��� �� �ִ�. */
		char *pairs;
		char jupdusa[STRING_SIZE]; int jupdusa_exist;

		// XXX: koma ����� ���� ����. ���� �ȳ��� �Ѿ�Ը� ��ġ. ���߿��� �����Ǿ���. 
		if (h->Result[idx][0] == 0x00) continue; // skip

		/* morph_tag_pairs �� ������ ���� ���� ���ڿ��̴�.
		 * "��/DU+��/NNBU+°/XSNN+����/NNCG+��/I+���ϴ�/EFF" */
		strnhcpy(morph_tag_pairs, h->Result[idx][0], LONG_STRING_SIZE);

		debug("idx[%d] morph_tag_pair[%s]", idx, morph_tag_pairs);

		previous_idx_of_index_word = idx_of_index_word; // roll back

		/* �±� ���Ŀ��� Result[idx][] �迭�� 0��° ���� �ϳ� ä������. �±��� ���� �ʴ� ���
		 * ���� �м� �ĺ��� ��Ÿ�� �� ������, �±��� �� ��� �ϳ����� ������ �ش�.
		 * ���� Result[idx][1], Result[idx][2] ���� ���� �������� �ʾƵ� �ȴ�.
		 */
		for (pairs = morph_tag_pairs, morpheme_count = 0, jupdusa_exist = FALSE;
			 pairs != NULL;
			 morpheme_count++)
		{
			int morpheme_len = 0;
			char morpheme[STRING_SIZE] = "";
			char tag[KOMA_TAG_LEN+1] = "";
			pairs = split_morph_tag_pair(pairs, morpheme, tag);

			morpheme_len = strlen(morpheme);
            debug("count[%d], m[%s] tag[%s]", morpheme_count, morpheme, tag);

            if ( h->is_raw_koma_text ) {
                strnhcpy(out[idx_of_index_word].word, morpheme, MAX_WORD_LEN-1);
                out[idx_of_index_word].len = strlen(out[idx_of_index_word].word);
                out[idx_of_index_word].pos = h->position;
                memcpy(&(out[idx_of_index_word].attribute), tag,
                                    sizeof(out[idx_of_index_word].attribute));
                
                out[idx_of_index_word].bytepos = h->byte_position + h->bPos[idx];
				idx_of_index_word++;
            } else if ( DO_TREAT_JUPDUSA
			            && morpheme_count == 0
						&& TAG_IS_JUPDUSA(tag, morpheme_len)
						&& exists_in(JUPDUSA_LIST, JUPDUSA_COUNT, morpheme) ) {
			    /* ���λ� ��ó�� */
				strcpy(jupdusa, morpheme);
				jupdusa_exist = TRUE;
				debug("���λ� ���: %s/%s", morpheme, tag);
			} else if ( DO_TREAT_JUPDUSA
			            && morpheme_count == 1
					   	&& jupdusa_exist == TRUE ) {
			    /* ���λ� ��ó�� */

				/* ������ ���¼��� ǰ�縦 Ȯ���Ͽ� ������ ǰ���� ��쿡�� �վ����� ������ ���ξ �����. */
				if (!(TAG_TO_BE_IGNORED(tag))) {
					char merged_morpheme[STRING_SIZE] = "";
					strncat(merged_morpheme, jupdusa, STRING_SIZE);
					strncat(merged_morpheme, morpheme, STRING_SIZE);

					jupdusa_exist = FALSE;

					strnhcpy(out[idx_of_index_word].word, merged_morpheme, MAX_WORD_LEN-1);
					out[idx_of_index_word].len = strlen(out[idx_of_index_word].word);
					out[idx_of_index_word].pos = h->position;
					memcpy(&(out[idx_of_index_word].attribute),
						   tag, sizeof(out[idx_of_index_word].attribute));
					out[idx_of_index_word].bytepos =  h->byte_position + h->bPos[idx];
					idx_of_index_word++;
					debug("���λ� �Ϸ�: %s/%s + %s/%s => %s/%s",
						jupdusa, "????", morpheme, tag, merged_morpheme, tag);
				} else {
				/* �̹� ���¼��� ǰ�簡 �������� ���� ǰ���� ���, ���λ� ������ ������. */
					jupdusa[0] = '\0';
					jupdusa_exist = FALSE;
					debug("���λ� ����: %s/%s + %s/%s => skip",
						jupdusa, "????", morpheme, tag);
				} /* if (!( _TAG_TO_BE_IGNORED_(tag)) ) */
			} /* ���λ� ��ó�� �Ϸ� */
            else if ( DO_TREAT_JUPMISA
			          && idx_of_index_word > 0
					  && TAG_IS_JUPMISA(tag, morpheme_len)
					  && exists_in(JUPMISA_LIST, JUPMISA_COUNT, morpheme)
                      && out[idx_of_index_word-1].pos == h->position ) {
			  /* ���̻� ó�� */

				int len = MAX_WORD_LEN - out[idx_of_index_word-1].len;
				if (len < 0) {
			 	 crit("length of word[%s] is longer than MAX_WORD_LEN[%d]",
							out[idx_of_index_word-1].word, MAX_WORD_LEN);
			 	 len = 0;
				}
				debug("���̻�: %s/%s + %s/%s => %s%s/%s",
						out[idx_of_index_word-1].word,
						(char*)&out[idx_of_index_word-1].attribute,
						morpheme, tag, 
						out[idx_of_index_word-1].word, morpheme,
						(char*)&out[idx_of_index_word-1].attribute);

				strncat(out[idx_of_index_word-1].word, morpheme , len);
				out[idx_of_index_word-1].word[MAX_WORD_LEN-1]='\0';
				out[idx_of_index_word-1].len = strlen(out[idx_of_index_word-1].word);
			} /* ���̻� ó�� �Ϸ� */
			else
			/* �׿� ǰ�� ó�� */
			// ���δܾ�� ������ ǰ���
			// TAG_TO_BE_IGNORED_MORE or TAG_TO_BE_IGNORED
			if ( ! TAG_TO_BE_IGNORED(tag) ) {
				/* ���δܾ�� ������ ǰ����� �ƴϸ� ... */
				strnhcpy(out[idx_of_index_word].word, morpheme, MAX_WORD_LEN-1);
				out[idx_of_index_word].len = strlen(out[idx_of_index_word].word);
				out[idx_of_index_word].pos = h->position;
				memcpy(&(out[idx_of_index_word].attribute), tag,
									sizeof(out[idx_of_index_word].attribute));

				out[idx_of_index_word].bytepos = h->byte_position + h->bPos[idx];
				if (idx_of_index_word > max)
					CRIT("idx_of_index_word[%d] is bigger than parameter max[%d]: morpheme[%s]",
							idx_of_index_word, max, morpheme);
				idx_of_index_word++;
            	debug("���ξ�: %s/%s", morpheme, tag);
			} else {
            	debug("������: %s/%s", morpheme, tag);
			}
			/* end: if ( h->is_raw_koma_text ) */

			if ( idx_of_index_word >= max ) {
				debug("idx_of_index_word[%d] >= max[%d]", idx_of_index_word, max);
				debug("idx[%d]/result_index[%d]/result_count[%d]", idx, h->result_index, h->result_count);
				if ( previous_idx_of_index_word == 0 ) {
					crit("Too small max_index_words[%d]. It cannot hold several morphemes from first one eojeol.", max);
					idx_of_index_word = max;
					h->result_index = idx + 1;
				}
				else {
					h->result_index = idx;
					idx_of_index_word = previous_idx_of_index_word;
				}

				goto FINISH;
			} else {
				/* ���������� index_word �迭 ������ �Ϸ���. */
				;
			}
	
		} /* for (pairs = morph_tag_pairs, morpheme_count = 0, jupdusa_exist = FALSE; */

	} /* for (idx = h->current_index; idx < h->result_count; idx++) */

	h->result_index = idx;

FINISH:

	debug("idx[%d]/result_index[%d]/result_count[%d]", idx, h->result_index, h->result_count);
	if ( h->result_index < h->result_count ) {
		/* ���� Result[] �迭�� ���� ��� �ǵ������� ���Ͽ���. ������ ȣ�⿡ ���� �迭��
		 * ���� �ǵ����־�� �Ѵ�. ���� move_text() ���� �ʵ���, koma_done�� FALSE��
		 * �����Ѵ�. */
		h->koma_done = FALSE;
 	} else {
		h->koma_done = TRUE;
	}

	if (idx_of_index_word == 0 && h->next_text != NULL) goto AGAIN;

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
	/* NOTE: koma_handle_t �� ���� singleton���� ����Ѵ�. ����
	 *       index_word_extractor�� �Բ� koma handle�� �������� �ʾƵ� �ȴ�.
	 */
	/* FreeHanTag(handle->HanTag); */
	/* sb_free(handle); */
	memset(handle, 0x00, sizeof(handle));
	handle->HanTag = HanTag;
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

static void setTreatJupdusa(configValue v) {
	if ( strncasecmp(v.argument[0], "NO", SHORT_STRING_SIZE) == 0 )
		DO_TREAT_JUPDUSA = 0;
}

static void setJupdusa(configValue v) {
	int n;

	for (n = 0; n < v.argNum; n++)
	{
		strncpy(JUPDUSA_LIST[JUPDUSA_COUNT], v.argument[n], 5);
		JUPDUSA_LIST[JUPDUSA_COUNT][4] = '\0';
		JUPDUSA_COUNT++;
	}
}

static void setTreatJupmisa(configValue v) {
	if ( strncasecmp(v.argument[0], "NO", SHORT_STRING_SIZE) == 0 )
		DO_TREAT_JUPMISA = 0;
}

static void setJupmisa(configValue v) {
	int n;

	for (n = 0; n < v.argNum; n++)
	{
		strncpy(JUPMISA_LIST[JUPMISA_COUNT], v.argument[n], 5);
		JUPMISA_LIST[JUPMISA_COUNT][4] = '\0';
		JUPMISA_COUNT++;
	}
}

static void setTaggingMethod(configValue v) {
	if ( strncasecmp(v.argument[0], "PATH_BASED", SHORT_STRING_SIZE) == 0 )
		TAGGING_METHOD = PATH_BASED_TAGGING;
	else if ( strncasecmp(v.argument[0], "STATE_BASED", SHORT_STRING_SIZE) == 0 )
		TAGGING_METHOD = STATE_BASED_TAGGING;
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
    CONFIG_GET("TreatJupdusa", setTreatJupdusa, 1, "YES or NO. Default value is YES."),
    CONFIG_GET("Jupdusa", setJupdusa, VAR_ARG, "List of Jupdusa."),
    CONFIG_GET("TreatJupmisa", setTreatJupmisa, 1, "YES or NO. Default value is YES."),
    CONFIG_GET("Jupmisa", setJupmisa, VAR_ARG, "List of Jupmisa."),
    CONFIG_GET("TaggingMethod", setTaggingMethod, 1, "PATH_BASED or STATE_BASED. Default is PATH_BASED."),
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

static void test_sentence(char *t, int morph_id)
{
	int n;

	index_word_extractor_t *extractor = NULL;
	index_word_t *index_word_array = NULL;

	extractor = new_koma_analyzer(morph_id);
	warn("morph_id[%d] set_text(e, [%s])", morph_id, t);
	_koma_set_text(extractor, t);
	index_word_array = (index_word_t*)sb_calloc(10, sizeof(index_word_t));

	while ( ( n = _koma_analyze(extractor, index_word_array, 50) ) > 0 )
	{
		int i;

		for ( i = 0; i < n; i++ )
		{
			char tag[KOMA_TAG_LEN+1] = "";
			memcpy(tag, (char *)&index_word_array[i].attribute, KOMA_TAG_LEN);
			tag[KOMA_TAG_LEN] = '\0';
			info("[%s] [%s] [%d]", index_word_array[i].word, tag, index_word_array[i].pos);
		}
	}

	info("test is done.");
}

int test_mod_koma(void)
{
	char *t1 = "�ѱ۹��ڿ����߶�������׽�Ʈ�մϴ�.\r\n"
			"�ι�° �����Դϴ�. ���ξ�� �ʰ�. ������/�׽�Ʈ. �ѱ�ABC�Է�.\r\n"
			"�ȳ��ϼ���. �ݰ����ϴ�.\r\n"
			"������ ����.  ���� �ٹٲ� ������ ����.\r\n";
	char *t2 = "�ι�° �׽�Ʈ.\r\n"
			"�ι�° ������ ����.  ���� �ٹٲ� ������ ����.";
	char *t3 = "Ư�ʹ� �����Ģ(2002. 12. 31. �Ǽ�����η� ��344ȣ�� �����Ǳ� "
	           "���� ��, ���� ����Ư�� �����Ģ���̶� �Ѵ�)�� ���� ������ ���� ";
	char *t4 = "�޸�,��ħǥ.������������ "
	           "���ʰ�ȣ(�����ʰ�ȣ)���ȣ[���ȣ]������ "
			   "�߰�ȣ{�߰�ȣ}������/��ǥ*AND&OR|��#�����@����ǥ!";
	char *t5 = "�ټ�° �׽�Ʈ.\r\n"
			"�ټ�° ������ ����.  ���� �ٹٲ� ������ ��y��";

	MAX_SENTENCE_LENGTH = 100;
	
	test_sentence(t1, MY_RAW_EXTRACTOR_ID);
	test_sentence(t2, MY_RAW_EXTRACTOR_ID);
	test_sentence(t3, MY_RAW_EXTRACTOR_ID);
	test_sentence(t4, MY_RAW_EXTRACTOR_ID);
	test_sentence(t5, MY_RAW_EXTRACTOR_ID);
	test_sentence(t1, MY_EXTRACTOR_ID);
	test_sentence(t2, MY_EXTRACTOR_ID);
	test_sentence(t3, MY_EXTRACTOR_ID);
	test_sentence(t4, MY_EXTRACTOR_ID);
	test_sentence(t5, MY_EXTRACTOR_ID);
	
	return 0;
}
