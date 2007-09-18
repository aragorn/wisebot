/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "memory.h"

#include "mod_koma_complex_noun_support.h"
#include "mod_api/index_word_extractor.h"

#include <string.h>

#define MY_EXTRACTOR_ID1		11
#define MY_EXTRACTOR_ID2		12

#define KOMA_INDEX_WORD_SIZE  (1024)
#define  TMP_INDEX_WORD_SIZE  (100)

#define SINGLE_WORD 				// koma_wrapper_analyzer2 , single word handle

static index_word_extractor_t *new_cn_analyzer(int id)
{
	index_word_extractor_t      *this = NULL;
	koma_complex_noun_support_t *handle = NULL;
	index_word_t  *koma_words = NULL;
	index_word_t  *tmp_words  = NULL;
	koma_handle_t *koma = NULL;

	if (id != MY_EXTRACTOR_ID1 &&
			id != MY_EXTRACTOR_ID2) 
		return (index_word_extractor_t*)MINUS_DECLINE; //XXX: just DECLINE??

	this = sb_calloc(1, sizeof(index_word_extractor_t));
	if (this == NULL) {
		crit("cannot allocate index word extractor object");
		goto FAILURE;
	}
	this->id = id;

	handle = sb_calloc(1, sizeof(koma_complex_noun_support_t));
	if (handle == NULL) {
		crit("cannot allocate koma_analyzer with complex noun support id[%d]", id);
		goto FAILURE;
	}
	this->handle = handle;
	handle->id = id;

	koma = new_koma();
	if (koma == NULL) {
		crit("cannot create mod_koma handler");
		goto FAILURE;
	}
	handle->koma = koma;

	koma_words = sb_calloc(KOMA_INDEX_WORD_SIZE, sizeof(index_word_t));
	if (koma_words == NULL) {
		crit("cannot allocate koma_words");
		goto FAILURE;
	}
	handle->koma_words = koma_words;
	handle->koma_words_size = 0;

	tmp_words = sb_calloc(TMP_INDEX_WORD_SIZE, sizeof(index_word_t));
	if (tmp_words == NULL) {
		crit("cannot allocate tmp_words");
		goto FAILURE;
	}
	handle->tmp_words = tmp_words;
	handle->tmp_words_size = 0;


	if (this == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("extractor pointer is %d (same as the decline value for this api)", MINUS_DECLINE);
	}

	return this;

FAILURE:
	if (tmp_words != NULL) sb_free(tmp_words);
	if (koma_words != NULL) sb_free(koma_words);
	if (handle != NULL) sb_free(handle);
	if (this   != NULL) sb_free(this);
	return NULL;

}

static int cn_set_text(index_word_extractor_t* extractor, const char* text)
{
	koma_complex_noun_support_t* handle=NULL;

	if (extractor->id != MY_EXTRACTOR_ID1 &&
			extractor->id != MY_EXTRACTOR_ID2) 
		return DECLINE;

	handle = extractor->handle;
	koma_set_text(handle->koma, text);
	handle->end_of_text = FALSE;
	memset(handle->koma_words, 0x00, sizeof(index_word_t)*KOMA_INDEX_WORD_SIZE);

	return SUCCESS;
}

static int delete_cn_analyzer(index_word_extractor_t* extractor)
{
	koma_complex_noun_support_t *wrapper=NULL;

	if (extractor->id != MY_EXTRACTOR_ID1 &&
			extractor->id != MY_EXTRACTOR_ID2) 
		return DECLINE;

	wrapper = extractor->handle;
	delete_koma(wrapper->koma);

	sb_free(wrapper);
	sb_free(extractor);

	return SUCCESS;
}

static int merge_noun(index_word_t *result, index_word_t *src1, index_word_t *src2)
{
	sb_assert(result != NULL);
	sb_assert(src1 != NULL);

	if (src2 == NULL)
	{
		strncpy(result->word, src1->word, MAX_WORD_LEN-1);
		result->word[MAX_WORD_LEN-1] = '\0';

		result->attribute = src1->attribute;
		result->field = src1->field;
		result->len = strlen(result->word);
		result->pos = src1->pos;

		return 1;
	}

	if (src1->pos != src2->pos) {
		crit("eojeol positions must be the same. pos1:%d, pos2:%d",
				src1->pos, src2->pos);
		return 0;
	}

	/* ��縸 �������� �ʰ�, ��� ǰ�翡 ���� ���ո��ó�� ��Ģ�� �����Ѵ�. */
	if ( TRUE ||
		( TAG_IS_NOUN((char *)&(src1->attribute))
		&& TAG_IS_NOUN((char *)&(src1->attribute)) ) )
	{
		strncpy(result->word, src1->word, MAX_WORD_LEN);
		result->word[MAX_WORD_LEN-1] = '\0';
		strncat(result->word, src2->word, MAX_WORD_LEN - strlen(result->word) - 1);
		result->word[MAX_WORD_LEN-1] = '\0';

		memcpy(&result->attribute, "NNCC", 4);
		result->field = src1->field;
		result->len = strlen(result->word);
		result->pos = src1->pos;
	
		return 1;
	} else {
		/* �� ���¼� ��� ��簡 �ƴϸ�, �������� �ʴ´�. */
		return 0;
	}
}

static int take_tmp_words12 (
 koma_complex_noun_support_t *h,
 index_word_t *index_word,
 int32_t max_index_word )
{
	int count = 0;

	debug("max_index_word[%d] tmp_words_size[%d] tmp_idx[%d]",
		max_index_word, h->tmp_words_size, h->tmp_idx);
	if (max_index_word == 0) return 0;
	if (h->tmp_words_size == 0) return 0;

	/* ���ո�縦 �����Ͽ� index_word[]�� �����δ�. */

	if (h->tmp_words_size == 1)
	{ /* ������ �ܾ 1���� ���, �״�� index_word[]�� �ִ´�. */
		int n = merge_noun(&index_word[count], &h->tmp_words[0], NULL);
		count += n;
	} else
	if (h->tmp_words_size == 2)
	{ /* ������ �ܾ 2���� ��� */
		int n = merge_noun(&index_word[count], &h->tmp_words[0], &h->tmp_words[1]);
		count += n;
	} else
	{ /* ������ �ܾ 3�� �̻��� ��� */
		int j;

		for (j = 0; j < h->tmp_words_size-2; j++)
		{
			index_word_t w;
			int n = merge_noun(&w, &h->tmp_words[j], &h->tmp_words[j+1]);
			if ( n )
			{
				n = merge_noun(&index_word[count], &w, &h->tmp_words[j+2]);
				count += n;
			}
		}
	} /* end of ������ �ܾ 3�� �̻��� ��� */

	h->tmp_idx = 0;
	h->tmp_words_size = 0;
	memset(h->tmp_words, 0x00, sizeof(index_word_t)*TMP_INDEX_WORD_SIZE);
	return count;
}


static int take_tmp_words (
 koma_complex_noun_support_t *h,
 index_word_t *index_word,
 int32_t max_index_word )
{
	int i, count = 0;
	int next = h->tmp_words_size;

	if (h->id == MY_EXTRACTOR_ID2)
		return take_tmp_words12(h, index_word, max_index_word);

	debug("max_index_word[%d] tmp_words_size[%d] tmp_idx[%d]",
		max_index_word, h->tmp_words_size, h->tmp_idx);
	if (max_index_word == 0) return 0;
	if (h->tmp_words_size == 0) return 0;

	if (h->tmp_idx == 0)
	{ /* ���ո�縦 �����Ͽ� tmp_words[]�� �����δ�. */

		if (h->tmp_words_size == 1)
		{ /* ������ �ܾ 1���� ���, �ƹ��͵� ���� �ʴ´�. */
			;
		} else
		if (h->tmp_words_size == 2)
		{ /* ������ �ܾ 2���� ��� */
			int n = merge_noun(&h->tmp_words[next], &h->tmp_words[0], &h->tmp_words[1]);
			next += n;
		} else
		{ /* ������ �ܾ 3�� �̻��� ��� */
			int j;

			for (j = 0; j < h->tmp_words_size-1; j++)
			{
				int n = merge_noun(&h->tmp_words[next], &h->tmp_words[j], &h->tmp_words[j+1]);
				next += n;
			}

			for (j = 0; j < h->tmp_words_size-2; j++)
			{
				index_word_t w;
				int n = merge_noun(&w, &h->tmp_words[j], &h->tmp_words[j+1]);
				if ( n )
				{
					n = merge_noun(&h->tmp_words[next], &w, &h->tmp_words[j+2]);
					next += n;
				}
			}
		} /* end of ������ �ܾ 3�� �̻��� ��� */
	} // if (h->tmp_idx == 0)
	h->tmp_words_size = next;

	for (i = h->tmp_idx; i < h->tmp_words_size; i++)
	{
		debug("index_word at %d <-- tmp_words[%d][%s/%s]",
		  count, i, h->tmp_words[i].word, (char*)&h->tmp_words[i].attribute);

		merge_noun(&index_word[count], &h->tmp_words[i], NULL);
		count ++;
		h->tmp_idx ++;

		if (count == max_index_word) break;
		/* break�� �ƴ϶� return�� �ϰ� �ȴٸ�? tmp_words�� �� ����� �ʾƾ�
		 * �ϴ� �� �ƴѰ�? tmp_idx�� ������ ������ ��������. */
	}
	h->tmp_idx = 0;
	h->tmp_words_size = 0;
	memset(h->tmp_words, 0x00, sizeof(index_word_t)*TMP_INDEX_WORD_SIZE);
	return count;
}

static int put_tmp_words(
 koma_complex_noun_support_t *h,
 index_word_t *index_word)
{
	int next = h->tmp_words_size;

	/* ���� ���� ���¼� ���ڰ� �ʹ� ����. */
	if (next > 30) return next;

	debug("tmp_words at %d <-- index_word[%s/%s]",
	      next, index_word->word, (char*)&index_word->attribute);
	strnhcpy(h->tmp_words[next].word, index_word->word, MAX_WORD_LEN-1);
	h->tmp_words[next].attribute = index_word->attribute;
	h->tmp_words[next].pos = index_word->pos;

	next++;
	h->tmp_words_size = next;
	return next;
}

/* ABCD -> A, B, C, D, AB, BC, CD, ABCD */
static int cn_analyze(index_word_extractor_t *extractor,
						index_word_t index_word[], int32_t max_index_word)
{
	int count = 0, i=0, n;
	koma_complex_noun_support_t *h=NULL;

	if (extractor->id != MY_EXTRACTOR_ID1
		&& extractor->id != MY_EXTRACTOR_ID2 ) return MINUS_DECLINE;

	if (max_index_word < 10 + 9 + 8)
	{
		warn("max size[%d] of index_word[] is too small.", max_index_word);
	}

	h = extractor->handle;

	/* tmp_words[]�� �ܾ���� ���� �ִ��� Ȯ���ϰ�, �� �ܾ���� �����
	 * ��������. */
	if ( h->tmp_words_size > 0 )
	{
		n = take_tmp_words(h, index_word, max_index_word);
		/* XXX ������ �� ���� ���� ��������, max_index_word�� �ʹ� ���� ���
		 * �� ������ �ܾ ��� ����� �������� ���� �� �ִ�.
		 * �� ���, max_index_word ������ ����� ��������, �ʰ����� ��������
		 * �Ѵ�. */
		count += n;
		if ( count == max_index_word ) return count;
	}

	/* end_of_text�� ���, set_text()���� �ٽ� �ؾ� �Ѵ�. */
	if ( h->end_of_text == TRUE ) return 0;

	/* koma_words_is_empty���, ���¼Һм� ����� �� �����´�. */
	if ( h->koma_words_size == 0 )
	{
		int rv = koma_analyze(h->koma, h->koma_words, KOMA_INDEX_WORD_SIZE);
		h->koma_words_size = rv;
		if (rv == FAIL)
		{
			error("koma_analyze returned FAIL");
			h->end_of_text = TRUE;
			h->koma_idx = 0;
			h->koma_words_size = 0;
			return FAIL;
		} else if (rv == 0) /* end of text */
		{
			h->end_of_text = TRUE;
			h->koma_idx = 0;
			h->koma_words_size = 0;
			return 0;
		} else {
			h->koma_idx = 0;
			h->koma_words_size = rv;
		}
	}

	debug("count[%d],h->koma_idx[%d],h->koma_words_size[%d],h->tmp_idx[%d],h->tmp_words_size[%d]",
	      count, h->koma_idx, h->koma_words_size, h->tmp_idx, h->tmp_words_size);
	for (i = h->koma_idx; i < h->koma_words_size; i++, h->koma_idx++)
	{	/* ���¼Һм� ��� array�� ���� loop�� ����. */

		if (h->tmp_words[0].pos != h->koma_words[i].pos)
		{ /* ���ο� �����̴�. tmp_words[]�� �ܾ���� ����� �����
		   * ��������. */
			n = take_tmp_words(h, index_word+count, max_index_word-count);
			count += n;
			/* max_index_word�� �ʰ��Ͽ� return�ϴ� ���, h->koma_idx��
			 * �������� �ʾƾ� �Ѵ�. */
			if (count == max_index_word) return count;
		}
		
		debug("put_tmp_words[%d][%s/%s]", i, h->koma_words[i].word,
		                             (char*)&h->koma_words[i].attribute);
		put_tmp_words(h, &(h->koma_words[i]));
	}

	n = take_tmp_words(h, index_word+count, max_index_word-count);
	count += n;
	h->koma_words_size = 0;

	return count;
}

#if 0

		if (last_of_this_eojeol != koma_index_words[i].pos)
		{
			/* ���ο� �����̴�. ���� ���� �ܾ�� ���ո�縦 ����� ����. */
			int words = last_of_this_eojeol - first_of_this_eojeol + 1;

			if (last_of_this_eojeol == 0)
			{ /* �ƹ��͵� ���� �ʰ� �Ѿ��. */
				;
			} else
			if (max_index_word - count < words * 3)
			{ /* index_word[]�� ���� ���� á��. ���⼭ �׸��д�. */
				; /* XXX */
			} else
			if (words == 1)
			{ /* ������ �ܾ 1���� ���, �ܾ �׳� �ѱ��. */
				merge_word(&index_word[count++], &koma_index_words[last_of_this_eojeol], NULL);
			} else
			if (words == 2)
			{ /* ������ �ܾ 2���� ��� */
				merge_word(&index_word[count++], &koma_index_words[first_of_this_eojeol], NULL);
				merge_word(&index_word[count++], &koma_index_words[last_of_this_eojeol], NULL);
				merge_word(&index_word[count++], &koma_index_words[first_of_this_eojeol],
				                                 &koma_index_words[last_of_this_eojeol]);
			} else
			{ /* ������ �ܾ 3�� �̻��� ��� */
				int j;
				for (j = first_of_this_eojeol; j <= last_of_this_eojeol; j++)
					merge_word(&index_word[count++], &koma_index_words[j], NULL);

				for (j = first_of_this_eojeol; j < last_of_this_eojeol; j++)
					merge_word(&index_word[count++], &koma_index_words[j], &koma_index_words[j+1]);

				for (j = first_of_this_eojeol; j < last_of_this_eojeol - 1; j++)
				{
					index_word_t w;
					merge_word(&w, &koma_index_words[j], &koma_index_words[j+1]);
					merge_word(&index_word[count++], &w, &koma_index_words[j+2]);
				}
			}
		}

	}

#endif

/* ABCD -> AB, BC, CD */
#if 0
static int cn_analyze2(index_word_extractor_t *extractor,
						index_word_t index_word[], int32_t max_index_word)
{
	int	 	rv=0, i=0, idx=0;
	int 	limited_index_word_len=0;
	int32_t last_position = 0;
	int8_t	is_first_cut;
	int8_t  flag = TRUE;
	index_word_t tmp_index_word;
	index_word_t last_index_word;
	koma_complex_noun_support_t *wrapper=NULL;
	index_word_t *tmp_buffer_index_word=NULL;

	if (extractor->id != MY_EXTRACTOR_ID2) 
		return MINUS_DECLINE;


	wrapper = extractor->handle;

	limited_index_word_len = max_index_word; //XXX: description for it goes here..
	rv = koma_analyze(wrapper->koma, index_word, limited_index_word_len);
	if (rv == FAIL) {
		error("koma_analyze returned FAIL");
		return FAIL;
	}

	if (rv == 0)  /* end of text */
		return 0;

	is_first_cut = TRUE;

	memset(&last_index_word, 0x00 , sizeof(index_word_t));

	tmp_buffer_index_word = (index_word_t*)sb_calloc(max_index_word , sizeof(index_word_t));

	if ( tmp_buffer_index_word == NULL ) {
		info("cannot alloction memory : tmp_index_word array size :[%d]", max_index_word);
		return FAIL;
	}

	/* complex noun dealing code here.. */
	idx = 0; //XXX: overwrite.. (removes A, B, C, D..)
	for (i=0; i<rv; i++) {


		if ( !(TAG_IS_NOUN((char *)&(index_word[i].attribute))
				|| TAG_IS_KYUKJOSA((char *)&(index_word[i].attribute)) ) ) {
				if (flag == TRUE && last_index_word.len > 0) {
					tmp_buffer_index_word[idx] = last_index_word;
					idx++;
					if (idx == max_index_word)
						break;
				} 
				tmp_buffer_index_word[idx] = index_word[i];
				is_first_cut = FALSE;
				last_index_word = index_word[i];
				flag = FALSE;
				idx++;
				if (idx == max_index_word)
					break;

		}
		else if (last_position == index_word[i].pos ) {


			tmp_index_word = index_word[i];
			merge_noun( &(tmp_buffer_index_word[idx]), &last_index_word, &(index_word[i]));
			last_index_word = tmp_index_word;
			flag = FALSE;
			idx++;

			is_first_cut = FALSE;

		}
		else {

			tmp_index_word = index_word[i];

			if ( i>0 && is_first_cut == TRUE ) {
				tmp_buffer_index_word[idx] = last_index_word;
				idx++;
			}

			last_index_word = tmp_index_word;
			flag = TRUE;

			is_first_cut = TRUE;
		}

		last_position = index_word[i].pos;

		if (idx == max_index_word)
			break;
	}

	if ( i>0 && idx < max_index_word && is_first_cut == TRUE ) {
		tmp_buffer_index_word[idx] = last_index_word;
		idx++;
	}

	for(i=0; i<idx; i++) { // copy
		index_word[i] = tmp_buffer_index_word[i];
	}

	// sb_free(tmp_buffer_index_word); tmp_buffer_index_word = NULL;

	return idx;

}

/* wrapper for cn_analyze1, cn_analyze2 */
static int cn_analyze(index_word_extractor_t *extractor,
						index_word_t index_word[], int32_t max_index_word)
{
	if (extractor->id == MY_EXTRACTOR_ID1) 
		return cn_analyze1(extractor, index_word, max_index_word);
	else
	if (extractor->id == MY_EXTRACTOR_ID2) 
		return cn_analyze2(extractor, index_word, max_index_word);
	else
		return MINUS_DECLINE;
}
#endif

static void register_hooks(void)
{
	sb_hook_new_index_word_extractor(new_cn_analyzer,NULL,NULL,HOOK_MIDDLE);
	sb_hook_index_word_extractor_set_text(cn_set_text,NULL,NULL,HOOK_MIDDLE);
	sb_hook_delete_index_word_extractor(delete_cn_analyzer,NULL,NULL,HOOK_MIDDLE);

	sb_hook_get_index_words(cn_analyze,NULL,NULL,HOOK_MIDDLE);
}

module koma_complex_noun_support_module = {
    STANDARD_MODULE_STUFF,
    NULL, 	                /* config */
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

	extractor = new_cn_analyzer(morph_id);
	warn("morph_id[%d] set_text(e, [%s])", morph_id, t);
	cn_set_text(extractor, t);
	index_word_array = (index_word_t*)sb_calloc(50, sizeof(index_word_t));

	while ( ( n = cn_analyze(extractor, index_word_array, 50) ) > 0 )
	{
		int i;

		for ( i = 0; i < n; i++ )
		{
			char tag[KOMA_TAG_LEN+1] = "";
			memcpy(tag, (char *)&index_word_array[i].attribute, KOMA_TAG_LEN);
			tag[KOMA_TAG_LEN] = '\0';
			info("[n=%d] %s/%s:%d", n, index_word_array[i].word, tag, index_word_array[i].pos);
		}
	}

	info("test is done.");
}

int test_mod_koma_complex_noun(void)
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

	test_sentence(t1, MY_EXTRACTOR_ID1);
	test_sentence(t2, MY_EXTRACTOR_ID1);
	test_sentence(t3, MY_EXTRACTOR_ID1);
	test_sentence(t4, MY_EXTRACTOR_ID1);
	test_sentence(t5, MY_EXTRACTOR_ID1);
	test_sentence(t1, MY_EXTRACTOR_ID2);
	test_sentence(t2, MY_EXTRACTOR_ID2);
	test_sentence(t3, MY_EXTRACTOR_ID2);
	test_sentence(t4, MY_EXTRACTOR_ID2);
	test_sentence(t5, MY_EXTRACTOR_ID2);
	
	return 0;
}

const void *hack_test_mod_koma_complex_noun = (const void *)test_mod_koma_complex_noun;

