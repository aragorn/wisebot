/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "memory.h"

#include "mod_api/index_word_extractor.h"
#include "mod_pkoma.h"

#include <string.h>
#include <errno.h>

#define MODE_TREAT_XPNN 1 // ���λ� ó��
#define MODE_TREAT_XSNN 1 // ���̻� ó��

#define MY_EXTRACTOR_ID 	30
#define MY_RAW_EXTRACTOR_ID 39

static int HAS_LOADED_PKOMA_ENGINE = 0;

static int DO_TREAT_JUPDUSA = 1;
#define MAX_NUM_JUPDUSA (32)
static int  JUPDUSA_COUNT = 0;
static char JUPDUSA_LIST[MAX_NUM_JUPDUSA][5];

static int DO_TREAT_JUPMISA = 1;
#define MAX_NUM_JUPMISA (64)
static int  JUPMISA_COUNT = 0;
static char JUPMISA_LIST[MAX_NUM_JUPMISA][5];

static int MAX_SENTENCE_LENGTH = MOD_PKOMA_SENTENCE_LEN;

static int load_pkoma_engine();
static pkoma_handle_t *pkoma_handle = NULL;

pkoma_handle_t* new_pkoma()
{
	if (! HAS_LOADED_PKOMA_ENGINE)   load_pkoma_engine();

	if (pkoma_handle != NULL) return pkoma_handle;
	
	pkoma_handle = sb_calloc(1, sizeof(pkoma_handle_t));
	if (pkoma_handle == NULL) {
		crit("failed to malloc pkoma_handle: %s", strerror(errno));
		return NULL;
	}

	pkoma_handle->option = REMOVE_ON;
	pkoma_handle->option |= HtoH_ON;
	pkoma_handle->option |= TAGGER_ON;
	pkoma_handle->option |= SPACE_ON;

	return pkoma_handle;
}


static index_word_extractor_t* new_pkoma_analyzer(int id)
{
	index_word_extractor_t *extractor = NULL;
	pkoma_handle_t *handle=NULL;

	if (id != MY_EXTRACTOR_ID && id != MY_RAW_EXTRACTOR_ID)
		return (index_word_extractor_t*)MINUS_DECLINE;

	extractor = sb_calloc(1, sizeof(index_word_extractor_t));
	if (extractor == NULL) {
		crit("cannot allocate index word extractor object");
		return NULL;
	}

	handle = new_pkoma();
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
static char* move_text(pkoma_handle_t *h)
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
		return NULL;
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

	//h->result_index = 0;
	//h->result_count = 0;

	debug("text[%p:(%s)~(%s)]", h->text, dbg_str_begin(b, h->text), dbg_str_end(e, h->text));
	debug("next_text[%p:(%s)~(%s)]",
			h->next_text, dbg_str_begin(b, h->next_text), dbg_str_end(e, h->next_text));
	if (h->next_length == 0) h->next_text = NULL;

	return h->text;
}

/* ���¼Һм��� �ؽ�Ʈ�� �����Ѵ�. */
void pkoma_set_text(pkoma_handle_t* handle, const char* text)
{

	// set handle
	handle->orig_text = text;
	handle->eojeol_position = 0;
	handle->byte_position = 0;
	handle->next_text = handle->orig_text;
	handle->next_length = strlen(handle->next_text);
	handle->text[0] = '\0';

	handle->is_completed = TRUE;
}

static int _pkoma_set_text(index_word_extractor_t* extractor, const char* text)
{
	pkoma_handle_t *handle = NULL;

	if (extractor->id != MY_EXTRACTOR_ID && extractor->id != MY_RAW_EXTRACTOR_ID)
		return DECLINE;

	handle = extractor->handle;

	pkoma_set_text(handle, text);

	return SUCCESS;
}


int pkoma_analyze(pkoma_handle_t *h, index_word_t *out, int max)
{
	int	idx_of_index_word = 0;
	int previous_idx_of_index_word;
	path_ptr path;

AGAIN:
	/* ���� DoKomaAndHanTag()�� ����� ��� return�� ��� */
	if ( h->is_completed == TRUE && move_text(h) != NULL) {
		char b[SHORT_STRING_SIZE+1], e[SHORT_STRING_SIZE+1];
		//move_text(h);
		debug("h->text[%p] [%s]~[%s]",
		      h->text, dbg_str_begin(b, h->text), dbg_str_end(e, h->text));
		               /* API_Tagger(�Է� �ؽ�Ʈ, ���, �ɼ�) */
		h->eojeol_index = 0;
		h->eojeol_count = API_Tagger(h->text, &(h->tag_result), h->option);
		h->current_path = h->tag_result;
		h->is_completed = FALSE; /* Result �迭�� ���� ��� index_word_t* out ���� ��������, 
								    h->is_completed = TRUE �� �ȴ�. */
	}

	/* ���¼� �м� ����� �� ���������� ������ ����. */
	for (path = h->current_path;
	     path != NULL;
		 path = path->next, h->eojeol_index++) {

		int morpheme_count;
		int jupdusa_exist;
		char jupdusa[SHORT_STRING_SIZE];
		char jupdusa_tag[SHORT_STRING_SIZE];
		morph_ptr current_morpheme;

		debug("eojeol[%s]", path->word);
		previous_idx_of_index_word = idx_of_index_word;
		/* idx_of_index_word�� max�� �ʰ��� ���, ���������� �ǵ����� �Ѵ�. */

		/* prev_cont�� ���� 1�� ���, ���� ������ �̹� ������ �Է½� ����
		 * �����̾����� �ǹ��Ѵ�. KoMAApi.h �� path_node ����ü ����. */
		if (path->prev_cont == 0) h->eojeol_position++;

		for (current_morpheme = path->select->morph_s,
		         morpheme_count = 0, jupdusa_exist = FALSE;
		     current_morpheme != NULL;
			 current_morpheme = current_morpheme->next,
			     morpheme_count++) {

			char *morpheme = current_morpheme->morph;
			char *tag      = current_morpheme->tag;
			char *ci       = current_morpheme->ci;
			//int morpheme_len = strlen(morpheme);

			debug("count[%d] m[%s] ci[%s] tag[%s]", morpheme_count, morpheme, ci, tag);

            if ( h->is_raw_koma_text ) {
                strnhcpy(out[idx_of_index_word].word, morpheme, MAX_WORD_LEN-1);
                out[idx_of_index_word].len = strlen(out[idx_of_index_word].word);
                out[idx_of_index_word].pos = h->eojeol_position;
                memcpy(&(out[idx_of_index_word].attribute), tag,
                                    sizeof(out[idx_of_index_word].attribute));
                
				idx_of_index_word++;
            } else if ( DO_TREAT_JUPDUSA
			            && morpheme_count == 0
						&& TAG_IS_JUPDUSA(tag, morpheme_len)
						&& exists_in(JUPDUSA_LIST, JUPDUSA_COUNT, morpheme) ) {
			    /* ���λ� ��ó�� */
				strncpy(jupdusa, morpheme, SHORT_STRING_SIZE-1);
				strncpy(jupdusa_tag, tag, SHORT_STRING_SIZE-1);
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
					out[idx_of_index_word].pos = h->eojeol_position;
					memcpy(&(out[idx_of_index_word].attribute),
						   tag, sizeof(out[idx_of_index_word].attribute));
					idx_of_index_word++;
					debug("���λ� �Ϸ�: %s/%s + %s/%s => %s/%s",
						jupdusa, jupdusa_tag, morpheme, tag, merged_morpheme, tag);
				} else {
				/* �̹� ���¼��� ǰ�簡 �������� ���� ǰ���� ���, ���λ� ������ ������. */
					jupdusa[0] = '\0';
					jupdusa_exist = FALSE;
					debug("���λ� ����: %s/%s + %s/%s => skip",
						jupdusa, jupdusa_tag, morpheme, tag);
				} /* if (!( _TAG_TO_BE_IGNORED_(tag)) ) */
			} /* ���λ� ��ó�� �Ϸ� */
            else if ( DO_TREAT_JUPMISA
			          && idx_of_index_word > 0
					  && TAG_IS_JUPMISA(tag, morpheme_len)
					  && exists_in(JUPMISA_LIST, JUPMISA_COUNT, morpheme)
                      && out[idx_of_index_word-1].pos == h->eojeol_position ) {
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
				out[idx_of_index_word].pos = h->eojeol_position;
				memcpy(&(out[idx_of_index_word].attribute), tag,
									sizeof(out[idx_of_index_word].attribute));

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
				/* �̹� ������ �� ó������ ���Ͽ���. �������� �ٽ� ó���Ѵ�. */
				debug("idx_of_index_word[%d] >= max[%d]", idx_of_index_word, max);
				debug("eojeol_index[%d]/eojeol_count[%d]", h->eojeol_index, h->eojeol_count);

				if ( previous_idx_of_index_word == 0 ) {
					crit("Too small max_index_words[%d]. It cannot hold several morphemes from first one eojeol.", max);
					idx_of_index_word = max;
					h->eojeol_index++;
					h->current_path = path->next;
				}
				else {
					h->current_path = path;
					idx_of_index_word = previous_idx_of_index_word;
				}
				goto FINISH;
			} else {
				/* ���������� index_word �迭 ������ �Ϸ���. */
				;
			}
	
		} /* for (pairs = morph_tag_pairs, morpheme_count = 0, jupdusa_exist = FALSE; */

	} /* for (idx = h->current_index; idx < h->result_count; idx++) */

	h->current_path = path;

FINISH:

	debug("eojeol_index[%d]/eojeol_count[%d]", h->eojeol_index, h->eojeol_count);

	if ( h->current_path != NULL ) {
		/* ���� tag_result ����� ��� �ǵ������� ���Ͽ���. ������ ȣ�⿡ ���� �迭��
		 * ���� �ǵ����־�� �Ѵ�. ���� move_text() ���� �ʵ���, is_completed�� FALSE��
		 * �����Ѵ�. */
		h->is_completed = FALSE;
 	} else {
		h->is_completed = TRUE;
		FreePathPtr(&(h->tag_result));
	}

	if (idx_of_index_word == 0 && h->next_text != NULL) goto AGAIN;

	return idx_of_index_word;
}

static int _pkoma_analyze(index_word_extractor_t *extractor, index_word_t *indexwords, int max)
{
    if(extractor->id != MY_EXTRACTOR_ID && extractor->id != MY_RAW_EXTRACTOR_ID) return MINUS_DECLINE;

    if(extractor->id == MY_RAW_EXTRACTOR_ID) {
        ((pkoma_handle_t*)extractor->handle)->is_raw_koma_text = 1;
    }

    return pkoma_analyze(extractor->handle, indexwords, max);
}

void delete_pkoma(pkoma_handle_t *handle)
{
	/* NOTE: koma_handle_t �� ���� singleton���� ����Ѵ�. ����
	 *       index_word_extractor�� �Բ� koma handle�� �������� �ʾƵ� �ȴ�.
	 */
	memset(handle, 0x00, sizeof(handle));
}
	
static int delete_pkoma_analyzer(index_word_extractor_t *extractor)
{
	if (extractor->id != MY_EXTRACTOR_ID && extractor->id != MY_RAW_EXTRACTOR_ID) return DECLINE;

	delete_pkoma(extractor->handle);
	sb_free(extractor);

	return SUCCESS;
}

static int load_pkoma_engine()
{
	char dictionary_path[STRING_SIZE];

	if (HAS_LOADED_PKOMA_ENGINE) return SUCCESS;

	snprintf(dictionary_path, STRING_SIZE, "%s/%s", gSoftBotRoot, "share/pkoma/");

	Load_Dictionary(dictionary_path);
	Load_Bigram(dictionary_path);
	Load_Rule(dictionary_path);

	HAS_LOADED_PKOMA_ENGINE = 1;
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

/** config stuff **/
static config_t config[] = {
    CONFIG_GET("TreatJupdusa", setTreatJupdusa, 1, "YES or NO. Default value is YES."),
    CONFIG_GET("Jupdusa", setJupdusa, VAR_ARG, "List of Jupdusa."),
    CONFIG_GET("TreatJupmisa", setTreatJupmisa, 1, "YES or NO. Default value is YES."),
    CONFIG_GET("Jupmisa", setJupmisa, VAR_ARG, "List of Jupmisa."),
	{ NULL }
};

static void register_hooks(void)
{
	sb_hook_new_index_word_extractor(new_pkoma_analyzer, 
												NULL, NULL, HOOK_MIDDLE);
	sb_hook_index_word_extractor_set_text(_pkoma_set_text, 
												NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_index_words(_pkoma_analyze, NULL, NULL, HOOK_MIDDLE);
	sb_hook_delete_index_word_extractor(delete_pkoma_analyzer, 
												NULL, NULL, HOOK_MIDDLE);
}

module pkoma_module = {
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

	extractor = new_pkoma_analyzer(morph_id);
	warn("morph_id[%d] set_text(e, [%s])", morph_id, t);
	_pkoma_set_text(extractor, t);
	index_word_array = (index_word_t*)sb_calloc(10, sizeof(index_word_t));

	while ( ( n = _pkoma_analyze(extractor, index_word_array, 10) ) > 0 )
	{
		int i;

		for ( i = 0; i < n; i++ )
		{
			char tag[PKOMA_TAG_LEN+1] = "";
			memcpy(tag, (char *)&index_word_array[i].attribute, PKOMA_TAG_LEN);
			tag[PKOMA_TAG_LEN] = '\0';
			info("[%s] [%s] [%d]", index_word_array[i].word, tag, index_word_array[i].pos);
		}
	}

	info("test is done.");
}

int test_mod_pkoma(void)
{
	char *t1 = "�ѱ۹��ڿ����߶�������׽�Ʈ�մϴ�.\r\n"
			"�ι�° �����Դϴ�. ���ξ�� �ʰ�. ������/�׽�Ʈ. �ѱ�ABC�Է�.\r\n"
			"�ȳ��ϼ���. �ݰ����ϴ�.\r\n"
			"������ ����.  ���� �ٹٲ� ������ ����.\r\n";
	char *t2 = "�ι�° �׽�Ʈ.\r\n"
			"�ι�° ������ ����.  ���� �ٹٲ� ������ ����.";
	char *t3 = "����° �׽�Ʈ.\r\n"
			"����° ������ ����.  ���� �ٹٲ� ������ ����";
	char *t4 = "�׹�° �׽�Ʈ.\r\n"
			"�׹�° ������ ����.  ���� �ٹٲ� ������ ����y";
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
