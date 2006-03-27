/* $Id$ */ 
#include <math.h>
#include <fcntl.h> /* O_RDONLY */
#include <string.h>
#include <errno.h>
#include "common_core.h"
#include "memory.h"
#include "util.h"
#include "common_util.h"
#include "mod_api/qp.h"
#include "mod_api/qpp.h"
#include "mod_api/indexdb.h"
#include "mod_api/vrfi.h"
#include "mod_api/lexicon.h"
#include "mod_api/cdm.h"
#include "mod_api/docapi.h"
#include "mod_api/indexer.h"
#include "mod_api/protocol4.h" /* sb4 related error codes */

#include "mod_qp.h"
#include "mod_morpheme/lib/lb_lex.h" /* 2228,2236: LEXM_IS_WHITE,LEXM_IS_EOS */

#ifdef DEBUG_SOFTBOTD
#	define debug_show_dochitlist(dochits, start, nelm, stream) \
			show_dochitlist(dochits, start, nelm, stream)
#else
#	define debug_show_dochitlist(dochits, start, nelm, stream)
#endif

#ifdef DEBUG_SOFTBOTD
static void show_dochitlist(doc_hit_t dochits[], uint32_t start, uint32_t nelm, 
					FILE* stream)
{
	uint32_t i=0, j=0;
	for (i=start; i<start+nelm; i++) {

		fprintf(stream, "[%u] id:%u, nhits:%d\n",
				i, dochits[i].id, dochits[i].nhits);
		for (j=0; j<dochits[i].nhits; j++) {
			int32_t pos=sb_run_get_position(&(dochits[i].hits[j]));
			int32_t field=sb_run_get_field(&(dochits[i].hits[j]));
			fprintf(stream, "  dochit[%d].hits[%d] field:%u, pos:%u\n",
							i, j, field, pos);
		}
	}
}
#endif


///////////////////////////////////////////////////////////
// field의 갯수 
static int field_count = 0;
static field_info_t field_info[MAX_EXT_FIELD];
///////////////////////////////////////////////////////////

enum DbType {
	TYPE_VRFI,
	TYPE_INDEXDB
};

static enum DbType        mDbType = TYPE_VRFI;

static VariableRecordFile *mVRFI = NULL;
static index_db_t         *mIfs  = NULL;

static char mIdxFilePath[MAX_PATH_LEN]= "dat/indexer/index"; // used by vrfi
static int mIdxDbSet = -1;

#define MAX_DOCATTR_SORTING_CONF		32
static char *sortingorder[MAX_DOCATTR_SORTING_CONF] = { NULL };

static index_list_t complete_index_list_pool[MAX_INDEX_LIST_POOL];
static index_list_t incomplete_index_list_pool[MAX_INCOMPLETE_INDEX_LIST_POOL];

static int fill_title_and_comment(doc_hit_t* doc_hits, char* comment);

typedef struct {
	uint32_t id;
	uint32_t ndochits; /* < MAX_DOCHITS_PER_DOCUMENT */
	uint32_t field;
	uint32_t *relevancy[MAX_DOCHITS_PER_DOCUMENT];
	doc_hit_t *dochits[MAX_DOCHITS_PER_DOCUMENT];

	int dochit_idx;
	int hit_idx;
} index_document_t;

/* stack related stuff */
index_list_t *complete_free_list_root = NULL;
index_list_t *incomplete_free_list_root = NULL;

int get_num_of_complete_free_list()
{
	index_list_t *this = NULL;
	int count=0;

	count = 0;
	this = complete_free_list_root;
	while(this != NULL) {
		this = this->next;
		count++;
	}

	return count;
}

int get_num_of_incomplete_free_list()
{
	index_list_t *this = NULL;
	int count=0;

	count = 0;
	this = incomplete_free_list_root;
	while(this != NULL) {
		this = this->next;
		count++;
	}

	return count;
}

#define show_num_of_free_nodes() _show_num_of_free_nodes(__FUNCTION__)
void _show_num_of_free_nodes(const char *function)
{
	int num=0;

	num = get_num_of_complete_free_list();
	INFO("%s(): num of complete free nodes is %d", function, num);

	num = get_num_of_incomplete_free_list();
	INFO("%s(): num of incomplete free nodes is %d", function, num);
}

// This memory management code is kind of horrible, and probably ought to be rewritten if the upper limit
// is on a per-thread basis, rather than static, as seems to have been initially assumed.
// The wasted lead node is also odd.
// Here I have just patched it to allocate new nodes in the heap dynamically if it would otherwise run out.
// This reduces locality, so it's not a preferred option, but better than dying.
// There might also be some leakage here - CHECKME [brian]

static index_list_t *alloc_incomplete_index_list(index_list_t *prev, index_list_t *next)
{
	index_list_t *this = sb_malloc(sizeof(index_list_t));
	this->prev = prev;
	this->next = next;
	return this;
}

static index_list_t *alloc_complete_index_list(index_list_t *prev, index_list_t *next)
{
	index_list_t *this = sb_malloc(sizeof(index_list_t));
	this->prev = prev;
	this->next = next;

	this->doc_hits = (doc_hit_t*)sb_calloc(MAX_DOC_HITS_SIZE, sizeof(doc_hit_t));
	if (this->doc_hits == NULL) {
		error("fail calling calloc: %s", strerror(errno));
		return NULL;
	}
	this->relevancy = (uint32_t*)sb_calloc(MAX_DOC_HITS_SIZE, sizeof(uint32_t));
	if (this->relevancy == NULL) {
		error("fail calling calloc: %s", strerror(errno));
		return NULL;
	}

	return this;
}

index_list_t *get_incomplete_list(void)
{
	index_list_t *this = NULL;

	if ( incomplete_free_list_root == NULL ) {
		warn("incomplete_free_list_root is NULL - extending set dynamically");
		incomplete_free_list_root = alloc_incomplete_index_list(NULL, NULL);
	}

	this = incomplete_free_list_root;
	incomplete_free_list_root = incomplete_free_list_root->next;
	if ( incomplete_free_list_root ) incomplete_free_list_root->prev = NULL;

	memset(this,0x00,sizeof(index_list_t));

	this->is_complete_list = FALSE;
	return this;
}

index_list_t *get_complete_list(void)
{
	index_list_t *this = NULL;
	doc_hit_t *doc_hits = NULL;
	uint32_t *relevancy = NULL;

	if ( complete_free_list_root == NULL ) {
		warn("complete_free_list_root is NULL - extending set dynamically");
		complete_free_list_root = alloc_complete_index_list(NULL, NULL);
	}

	this = complete_free_list_root;
	complete_free_list_root = complete_free_list_root->next;
	if ( complete_free_list_root ) complete_free_list_root->prev = NULL;

	// pointer가 가리키는 값들을 잃어버리지 않도록..
	doc_hits = this->doc_hits;
	relevancy = this->relevancy;

	memset(this,0x00,sizeof(index_list_t));

	this->doc_hits = doc_hits;
	this->relevancy = relevancy;
	this->list_size = MAX_DOC_HITS_SIZE; /* XXX: to where?? */

//	memset(this->doc_hits, 0, sizeof(doc_hit_t) * MAX_DOC_HITS_SIZE);
//	memset(this->relevancy, 0, sizeof(uint32_t) * MAX_DOC_HITS_SIZE);

//	CRIT("get complete list: %p", this);

	this->is_complete_list = TRUE;
	return this;
}

static void init_incomplete_free_list()
{
	int i = 0;
	index_list_t *this;

	incomplete_free_list_root = &(incomplete_index_list_pool[0]);

	for ( i = 0; i<MAX_INCOMPLETE_INDEX_LIST_POOL; i++) {
		this = &(incomplete_index_list_pool[i]);

        if ( i == 0 ) this->prev = NULL;
        else this->prev = &(incomplete_index_list_pool[i-1]);

        if ( i == MAX_INCOMPLETE_INDEX_LIST_POOL - 1 ) this->next = NULL;
        else this->next = &(incomplete_index_list_pool[i+1]);
	}

	return;
}

static void init_complete_free_list()
{
	int i = 0;
	index_list_t *this;

	complete_free_list_root = &(complete_index_list_pool[0]);

	for ( i = 0; i<MAX_INDEX_LIST_POOL; i++) {
		this = &(complete_index_list_pool[i]);

        if ( i == 0 ) this->prev = NULL;
        else this->prev = &(complete_index_list_pool[i-1]);

        if ( i == MAX_INDEX_LIST_POOL - 1 ) this->next = NULL;
        else this->next = &(complete_index_list_pool[i+1]);

		this->doc_hits = (doc_hit_t*)sb_calloc(MAX_DOC_HITS_SIZE, sizeof(doc_hit_t));
		if (this->doc_hits == NULL) {
			error("calloc(%d bytes) failed: %s",
					MAX_DOC_HITS_SIZE * (int)sizeof(doc_hit_t), strerror(errno));
			return;
		}
		this->relevancy = (uint32_t*)sb_calloc(MAX_DOC_HITS_SIZE, sizeof(uint32_t));
		if (this->relevancy == NULL) {
			error("relevancy calloc(%d bytes) failed: %s",
					MAX_DOC_HITS_SIZE * (int)sizeof(uint32_t), strerror(errno));
			return;
		}
	}

	return;
}

static void init_free_list()
{
	init_incomplete_free_list();
	init_complete_free_list();
}

#define show_stack(stack) _show_stack(__FUNCTION__, stack)

void _show_stack(const char *function, sb_stack_t *stack)
{
	index_list_t *current = stack->first;
	DEBUG("%s(): stack status", function);

	while(current != stack->last) {
		DEBUG("%s", current->word);

		current = current->next;
	} 

	/* XXX: last */
	DEBUG("%s", current->word);
	DEBUG("----------------------------------");
}

void stack_push(sb_stack_t *stack, index_list_t *this)
{
	if ( stack->size == 0 ) {
		stack->first = this;
		stack->last = this;
		this->prev = NULL;
		this->next = NULL;
	} else {
		stack->last->next = this;
		this->prev = stack->last;
		this->next = NULL;
		stack->last = this;
	}

	stack->size++;
}

index_list_t *stack_peek(sb_stack_t *stack)
{
	if ( stack->size == 0 ) {
		warn("empty stack");
		return NULL;
	}

	return stack->last;
}

// 맨 마지막 바로 앞을 가져온다... 쯥...
index_list_t *stack_peek_1(sb_stack_t *stack)
{
	if ( stack->size <= 1 ) {
		warn("not enough stack size");
		return NULL;
	}

	return stack->last->prev;
}

index_list_t *stack_pop(sb_stack_t *stack)
{
	index_list_t *this;
	
	this = stack->last;

	if ( stack->size == 0 ) {
		warn("empty stack");
		return NULL;
	} else if ( stack->size == 1 ) {
		stack->first = NULL;
		stack->last = NULL;
		stack->size--;
	} else {
		stack->last = this->prev;
		stack->last->next = NULL;
		stack->size--;
	}

	this->prev = NULL;
	return this;
}

static void release_list(index_list_t *this)
{
	index_list_t *current;

	if ( this == NULL ) return;

	if (this->is_complete_list == TRUE)
		DEBUG("before release: release_complete_list - count == (%u)", get_num_of_complete_free_list());
	else 
		DEBUG("before release: release_incomplete_list - count == (%u)", get_num_of_incomplete_free_list());

//	CRIT("release_list: %p", this);

	if (this->is_complete_list == TRUE) {
		// check - chaeyk
		current = complete_free_list_root;
		while ( current ) {
			if ( current == this ) {
				error("this is already in complete_free_list");
				break;
			}
			current = current->next;
		}

		if ( this < complete_index_list_pool
				|| this >= complete_index_list_pool+MAX_INDEX_LIST_POOL ) {
			warn( "release allocated complete list memory" );
			sb_free( this->relevancy );
			sb_free( this->doc_hits );
			sb_free( this );
		}
		else {
			if ( complete_free_list_root ) complete_free_list_root->prev = this;
			this->next = complete_free_list_root;
			this->prev = NULL;

			complete_free_list_root = this;
		}
	}
	else {
		// check - chaeyk
		current = incomplete_free_list_root;
		while ( current ) {
			if ( current == this ) {
				error("this is already in incomplete_free_list");
				break;
			}
			current = current->next;
		}

		if ( this < incomplete_index_list_pool ||
				this >= incomplete_index_list_pool+MAX_INCOMPLETE_INDEX_LIST_POOL ) {
			warn( "release allocated incomplete list memory" );
			sb_free( this );
		}
		else {
			if ( incomplete_free_list_root ) incomplete_free_list_root->prev = this;
			this->next = incomplete_free_list_root;
			this->prev = NULL;

			incomplete_free_list_root = this;
		}
	}

	if (this->is_complete_list == TRUE)
		DEBUG("release_complete_list - count == (%u)", get_num_of_complete_free_list());
	else 
		DEBUG("release_incomplete_list - count == (%u)", get_num_of_incomplete_free_list());

	return;
}
static void release_list2(index_list_t *l1, index_list_t *l2)
{
	release_list(l1);
	release_list(l2);
}
static void release_list3(index_list_t *l1, index_list_t *l2, index_list_t *l3)
{
	release_list(l1);
	release_list(l2);
	release_list(l3);
}

static int finalize_search(request_t *r)
{
	release_list(r->result_list);
	return SUCCESS;
}

void init_stack(sb_stack_t *stack)
{
	stack->size = 0;
	stack->first = NULL;
	stack->last = NULL;
}

/* end of stack related stuff */

// return 값은 읽어온 doc_hits 갯수. ( >= 0 )
// 에러인 경우는 FAIL
static int read_from_db(uint32_t wordid, char* word, doc_hit_t* doc_hits)
{
	int offset, length;
	int ndochits, ret;

	if ( mDbType == TYPE_INDEXDB ) {
		length = sb_run_indexdb_getsize( mIfs, wordid );
		if ( length == INDEXDB_FILE_NOT_EXISTS ) {
			warn("length is 0 of word[%d]: %s. something is wrong", wordid, word);
			return 0;
		}
		else if ( length == FAIL ) {
			crit("indexdb_getsize failed. word[%d]: %s", wordid, word);
			return FAIL;
		}

		info("word[%s] - length: %d, count: %d", word, length, length/(int)sizeof(doc_hit_t));

		if ( length % sizeof(doc_hit_t) != 0 ) {
			warn( "word[%d] length[%d] is not multiple of sizeof(doc_hit_t), but it may be temporary state",
				wordid, length );
		}
		ndochits = length / sizeof(doc_hit_t);

		if ( ndochits > MAX_DOC_HITS_SIZE ) {
			warn("ndochits[%u] > MAX_DOC_HITS_SIZE[%u]", ndochits, MAX_DOC_HITS_SIZE);
			offset = (ndochits - MAX_DOC_HITS_SIZE)*sizeof(doc_hit_t);
			length -= offset;
			ndochits = MAX_DOC_HITS_SIZE;
		}
		else offset = 0;
		
		ret = sb_run_indexdb_read( mIfs, wordid, offset, length, (void*)doc_hits );
		if ( ret < 0 ) {
			crit("indexdb_read failed. word[%d]: %s, ret: %d", wordid, word, ret);
			return FAIL;
		}

		DEBUG("ret[%d] of indexdb_read with %s(id:%u)",ret, word, wordid);
		if ( ret < length ) {
			ndochits = ret / sizeof(doc_hit_t);
			warn( "ret[%d] is less than length[%d]. reassign ndochits[%d]", ret, length, ndochits );
		}

		return ndochits;
	}
	else if ( mDbType == TYPE_VRFI ) {
		ret=sb_run_vrfi_get_num_of_data(mVRFI, (uint32_t)wordid, &ndochits);
		if (ret < 0) {
			crit("error vrfi get num of data");
			return FAIL;
		}
		DEBUG("%s ndochits:%u", word, ndochits);

		if (ndochits > MAX_DOC_HITS_SIZE) {
			error("ndochits[%u] > MAX_DOC_HITS_SIZE[%u]", ndochits, MAX_DOC_HITS_SIZE);
		}
		else if (ndochits == 0) {
			warn("ndochits is 0 of word[%d]: %s", wordid, word);
		}

		ret=sb_run_vrfi_get_variable(mVRFI, (uint32_t)wordid, 0L,
							MAX_DOC_HITS_SIZE,(void *)doc_hits);
		if (ret < 0) {
			crit("error vrf get variable wordid:%u",wordid);
			return FAIL;
		}
		DEBUG("ret[%d] of vrfi_get_variable with %s(id:%u)",ret, word, wordid);

		return ret;
	}
	else {
		warn("unknown DbType:%d", mDbType);
		return FAIL;
	}

	// never reach here
	crit("it's impossible to reach here");
	sb_assert(0);
	return 0;
}

static index_list_t *read_index_list(index_list_t *list)
{
	uint32_t wordid = list->wordid;
	uint32_t field = list->field;

	int ndochits=0;
	char word[MAX_WORD_LEN];
	int i=0, idx=0, cnt=0, nWordHit=0, tot_index_doccnt=0, j=0;
	enum index_list_type list_type = NORMAL;
	uint32_t Title_hit=0;

	if (list->is_complete_list == TRUE ||
			list->list_type == FAKE_OP ||
			list->list_type == NOT_EXIST_WORD) {
		return list;
	}
	else {
		sz_strncpy(word, list->word, MAX_WORD_LEN);
		list_type = list->list_type;

		release_list(list);
		list = get_complete_list();

		sz_strncpy(list->word, word, MAX_WORD_LEN);
		list->wordid = wordid;
		list->field = field;
		list->is_complete_list = TRUE;
		list->list_type = list_type;
	}

	ndochits = read_from_db( wordid, word, list->doc_hits );
	if ( ndochits < 0 ) {
		release_list(list);
		return NULL;
	}

	list->ndochits = ndochits;
	if ( ndochits == 0 ) return list;

	DEBUG("field:%s", sb_strbin(field, sizeof(field)));
	if (field != 0xffffffff) {
		for (i=0,idx=0 ; i<ndochits; i++) {
			//CRIT("did[%u] field:%s",list->doc_hits[i].id, sb_strbin(list->doc_hits[i].field,sizeof(uint32_t)));
			if ( list->doc_hits[i].field & field ) {
				list->doc_hits[idx++] = list->doc_hits[i];
				
				/* khyang - hit */
				cnt = list->doc_hits[i].nhits;
				
				nWordHit = 0;		
				Title_hit = 0;
				
				for(j=0; j < cnt; j++)
				{
					if ( ( ((uint32_t)1) << list->doc_hits[i].hits[j].std_hit.field ) & field)
					{
						nWordHit ++;
						
#if 0 //정시욱 - 사용하지 않는다.
#ifdef _TITLE_HIT_						
printf("doc(%ld):%s  %ld\n", list->doc_hits[i].id, sb_strbin(list->doc_hits[i].hits[j].std_hit.field,sizeof(uint32_t)), list->doc_hits[i].hits[j].std_hit.field );
						
						if ( Title_Field_No ==  list->doc_hits[i].hits[j].std_hit.field  ) //제목에서 나온경우
							Title_hit = Title_hit + 100;
printf("Title_hit:%d(%d)\n", Title_hit, field);		
#endif					
#endif
					}
					
					
						
				}	
				
				tot_index_doccnt = sb_run_last_indexed_did();

				list->relevancy[idx-1] = (((int)log10( (tot_index_doccnt*3) / (ndochits))+1 ) * nWordHit)+Title_hit;

										
			}
		}
		list->ndochits = idx;
	}
	else {
		// 초기화 해야 한다
		memset( list->relevancy, 0, sizeof(uint32_t)*ndochits );
	}

	INFO("[%d:%s] ndochits(%u)(after field filtered) (before field filtered:%u)",
			list->wordid, list->word, list->ndochits, ndochits);
	return list;
}

static void set_index_list(QueryNode *qnode, index_list_t *list)
{
	if (qnode->word_st.id == 0) {
		DEBUG("qnode.word_st.wordid(%u) == 0", qnode->word_st.id);
		list->ndochits = 0;
		list->is_complete_list = FALSE;
		list->wordid = qnode->word_st.id;
		return ;
	}
	list->is_complete_list = FALSE;
	list->field = qnode->field;
	list->wordid = qnode->word_st.id;
	strncpy(list->word,qnode->word_st.string, MAX_WORD_LEN);

	INFO("[%s]qnode->field:%s",list->word, sb_strbin(list->field, sizeof(list->field)));
	return;
}

inline static int get_nhits(doc_hit_t *doc_hit)
{
	return doc_hit->nhits;
}

inline static int fast_square(int x)
{
	static int square_array[10] = {1,4,9,16,25,36,49,64,81,100};

	x = (x < 0) ? -x:x;

	if ( x < 10 )
		return square_array[x];
	return x*x;
}

/* weight = TF * IDF	
 *	
 * TF  = term frequency	
 * IDF = log(2N/Dk) * t
 *
 * Fik = the absolute frequency of word k in document i
 * N   = number of Document (get under qpi)
 * Dk  = number of Document containing word k
 * t   = constant (ln2)
 * 
 */
/* FIXME: obsolete for a whilte
static void get_weight(QueryNode *qnode, DocId total_doc_number,  index_list_t *list)
{
	int i=0;
	double temp=0.0;
	
	// get_weight 함수를 호출하기전에 걸러주는 것도....
	if (total_doc_number == 0 || qnode->word_st.id ==0) {
		warn("total_doc_num:%u,  wordid:%u",
			(uint32_t)total_doc_number, qnode->word_st.id);
		return;
	}
	
	debug("get_weight: word[%s] word.df[%d] total_doc_num[%ld]"
		  , qnode->word_st.string, qnode->word_st.word_attr.df , total_doc_number);
	debug("list->nodes[%d]",list->ndochits);

	temp = (2*total_doc_number) / qnode->word_st.word_attr.df;
	
	for (i=0 ; i<list->ndochits ; i++) {
		list->relevancy[i] = list->doc_hits[i].nhits * (uint32_t)log(temp);
	}
	
	return;
}
*/

void get_hits_for_phrase(int dist, 
		doc_hit_t *result, 
		index_list_t *l1, int idx1,
		index_list_t *l2, int idx2)
{
	int nhits1=0, nhits2=0, field1=0, field2=0;
	int pos1=0, pos2=0, i=0, j=0;
	int32_t tmp=0;

	nhits1 = get_nhits(&(l1->doc_hits[idx1]));
	nhits2 = get_nhits(&(l2->doc_hits[idx2]));

	nhits1 = nhits1 > STD_HITS_LEN ? STD_HITS_LEN : nhits1;
	nhits2 = nhits2 > STD_HITS_LEN ? STD_HITS_LEN : nhits2;

	for (i=0; i<nhits1; i++) {
		field1 = sb_run_get_field(&(l1->doc_hits[idx1].hits[i]));
		if ( !( l1->field & ((uint32_t)1<<field1) ) ) continue;

		pos1 = sb_run_get_position(&(l1->doc_hits[idx1].hits[i]));
		if (pos1==MAX_STD_POSITION) continue;

		for (j=0; j<nhits2; j++) {
			field2 = sb_run_get_field(&(l2->doc_hits[idx2].hits[j]));
			if ( !( l2->field & ((uint32_t)1<<field2) ) ) continue;	

			if (field1 != field2) continue;

			pos2 = sb_run_get_position(&(l2->doc_hits[idx2].hits[j]));
			tmp = pos2-pos1;

			if (pos2==MAX_STD_POSITION) continue;
			else if (tmp < 0) continue;

			if (tmp <= dist) {
				result->hits[result->nhits++] =
									l1->doc_hits[idx1].hits[i];
			}

			if (result->nhits == STD_HITS_LEN) goto DONE_GET_HITS;
		}
	}
DONE_GET_HITS:
	return;
}

int get_hits_for_within(int dist, 
		doc_hit_t *result, 
		index_list_t *l1, int idx1,
		index_list_t *l2, int idx2)
{
	int nhits1=0, nhits2=0, field1=0, field2=0;
	int pos1=0, pos2=0, i=0, j=0;
	int32_t tmp=0, abs_tmp=0;

	nhits1 = get_nhits(&(l1->doc_hits[idx1]));
	nhits2 = get_nhits(&(l2->doc_hits[idx2]));

	nhits1 = nhits1 > STD_HITS_LEN ? STD_HITS_LEN : nhits1;
	nhits2 = nhits2 > STD_HITS_LEN ? STD_HITS_LEN : nhits2;

	result->nhits = 0;

	for (i=0; i<nhits1; i++) {
		field1 = sb_run_get_field(&(l1->doc_hits[idx1].hits[i]));
		if ( !( l1->field & ((uint32_t)1<<field1) ) ) continue;

		pos1 = sb_run_get_position(&(l1->doc_hits[idx1].hits[i]));
		if (pos1==MAX_STD_POSITION) continue;

		for (j=0; j<nhits2; j++) {
			field2 = sb_run_get_field(&(l2->doc_hits[idx2].hits[j]));
			if ( !( l2->field & ((uint32_t)1<<field2) ) ) continue;	

			if (field1 != field2) continue;

			pos2 = sb_run_get_position(&(l2->doc_hits[idx2].hits[j]));
			tmp = pos2-pos1;
			abs_tmp = tmp > 0 ? tmp:-tmp;

			if (pos2==MAX_STD_POSITION) continue;

			if (abs_tmp <= dist) {
				result->hits[result->nhits++] =
									l2->doc_hits[idx2].hits[j];
			}

			if (result->nhits >= STD_HITS_LEN) goto DONE_GET_HITS;
		}
	}
DONE_GET_HITS:
	return result->nhits;
}

int get_hits_for_normal_and(doc_hit_t *result, 
				index_list_t *l1, int idx1,
				index_list_t *l2, int idx2)
{
	int nhits2=0, field2=0;
	int j=0;

	nhits2 = get_nhits(&(l2->doc_hits[idx2]));
	nhits2 = nhits2 > STD_HITS_LEN ? STD_HITS_LEN : nhits2;

	for (j=0; j<nhits2; j++) {
		field2 = sb_run_get_field(&(l2->doc_hits[idx2].hits[j]));
		if ( !( l2->field & ((uint32_t)1<<field2) ) ) continue;	

		result->hits[result->nhits++] = l2->doc_hits[idx2].hits[j];
	}
	return result->nhits;
}

// XXX: check this when doint ranking-related stuffs.. --jiwon, 2002/10/21
int get_posweight(index_list_t *l1, int idx1, index_list_t *l2, int idx2)
{
#define MAX_DISTANCE_FOR_WEIGHT (5)
#define REVERSE_ORDER_PENALTY	(4)
	int nhits1=0, nhits2=0;
	int distance=0, weight=0, nearness=0;
	int i=0,j=0,pos1=0,pos2=0;

	nhits1 = get_nhits(&(l1->doc_hits[idx1]));
	nhits2 = get_nhits(&(l2->doc_hits[idx2]));

	DEBUG("nhits1 [%d]  nhits2 [%d]",nhits1 , nhits2);
	
	weight = 0;
	for(i = 0; i < nhits1; i++) {
		pos1 = sb_run_get_position(&(l1->doc_hits[idx1].hits[i]));
		for(j = i; j < nhits2; j++) {
			pos2 = sb_run_get_position(&(l2->doc_hits[idx2].hits[j]));
			distance = pos2-pos1;

			if ( distance == 0 )
				nearness = fast_square(MAX_DISTANCE_FOR_WEIGHT);
			if ( distance>0 && distance < MAX_DISTANCE_FOR_WEIGHT )
				nearness = fast_square(MAX_DISTANCE_FOR_WEIGHT-distance);
			else if ( distance<0 && (0-distance < MAX_DISTANCE_FOR_WEIGHT) ) 
				nearness = fast_square(MAX_DISTANCE_FOR_WEIGHT+distance)
								/ REVERSE_ORDER_PENALTY;
			else
				nearness = 0;

			weight += nearness;
		}
	}
	return weight;
}

void insert_one_to_result(index_list_t *dest, int idx, 
					  index_list_t *l1, int idx1) {
	int pos_weight=0;
	DEBUG("pos_weight[%d]",pos_weight);
	
	dest->doc_hits[idx] = l1->doc_hits[idx1];
	dest->relevancy[idx] = l1->relevancy[idx1];

	return;
}

void insert_to_result(index_list_t *dest, int idx, 
					  index_list_t *l1, int idx1,
					  index_list_t *l2, int idx2, doc_hit_t *result_hits) {
	int pos_weight=0;
	DEBUG("pos_weight[%d]",pos_weight);
	
	pos_weight = get_posweight(l1,idx1, l2,idx2);
	
/*    dest->doc_hits[idx] = l2->doc_hits[idx2];*/
	memcpy(&(dest->doc_hits[idx]), result_hits, sizeof(doc_hit_t));

	dest->relevancy[idx] = ( l1->relevancy[idx1] / 2 ) +
							( l2->relevancy[idx2] / 2 ) +
							( pos_weight ) / 2;
	return;
}

int operator_and_with_not(sb_stack_t *stack, QueryNode *qnode)
{
	int num_of_operands = 0;
	int idx1 = 0, idx2 = 0, idx3 = 0;
	index_list_t *this=NULL, *operand=NULL, *not=NULL;
	index_list_t *operand1=NULL, *operand2=NULL, *operand_tmp1=NULL, *operand_tmp2=NULL;
	
	num_of_operands = qnode->num_of_operands;
	if (num_of_operands != 2) {
		error("not and must have 2 operands but has %d", num_of_operands);
		num_of_operands = 2;
	}

	operand_tmp2 = stack_pop(stack);
	operand_tmp1 = stack_pop(stack);
	if ( operand_tmp2 == NULL || operand_tmp1 == NULL ) {
		error("need more operand?");
		release_list2(operand_tmp2, operand_tmp1);
		return FAIL;
	}

	operand2 = read_index_list(operand_tmp2);
	operand1 = read_index_list(operand_tmp1);
	if ( operand1 == NULL || operand2 == NULL ) {
		error("no free qnode");
		release_list2(operand1, operand2);
		return FAIL;
	}

	if (operand1->list_type == EXCLUDE) {
		not = operand1;
		operand = operand2;
	}
	else if (operand2->list_type == EXCLUDE) {
		not = operand2;
		operand = operand1;
	}
	else {
		error("Neither operand1,operand2 has NOT");
		release_list2(operand1, operand2);
		return FAIL;
	}

	if (operand->list_type == FAKE_OP) {
		stack_push(stack,operand);
		release_list(not);
		return SUCCESS;
	}

	this = get_complete_list();
	if ( this == NULL ) {
		error("no free qnode");
		release_list2(operand1, operand2);
		return FAIL;
	}
	this->list_type = NORMAL;

	CRIT("Operator not and");
	INFO("operand->ndochits:%u, not->ndochits:%u",operand->ndochits, not->ndochits);
	// 실제로 document id를 비교해서 not하는 부분.
	// Actually compares document id and operate "NOT AND"
	idx2=0;idx3=0;
	for (idx1=0; idx1<operand->ndochits; idx1++) {
		while ( idx2 < not->ndochits &&
				not->doc_hits[idx2].id < operand->doc_hits[idx1].id) {
			idx2++;
		}
		
		if (not->doc_hits[idx2].id == operand->doc_hits[idx1].id)
			continue;

		insert_one_to_result(this,idx3,operand,idx1);
		idx3++;
	}
	this->ndochits = idx3;

	release_list2(operand,not);
	stack_push(stack, this);

	return SUCCESS;
}

static uint32_t get_num_of_successive_dochit_same_to_first_docid(
						doc_hit_t dochits[], uint32_t max)
{
	uint32_t first_docid = dochits[0].id;
	uint32_t i=0;

	for (i=1; i<max; i++) {
		if (dochits[i].id != first_docid) {
			return i;
		}
	}

	return max;
}

static void set_idx_to_first_hit(index_document_t *doc)
{
	doc->dochit_idx = 0;
	doc->hit_idx = 0;
}
static void show_index_document(index_document_t *doc, char* name)
{
	DEBUG("name:%s", name);
	DEBUG("id: %u, ndochits:%u", doc->id, doc->ndochits);
	DEBUG("field: %s", sb_strbin(doc->field, sizeof(doc->field)));
	DEBUG("dochit_idx:%d, hit_idx:%d", doc->dochit_idx, doc->hit_idx);
}
static void init_index_document(index_document_t *doc,
						index_list_t *list, uint32_t start, uint32_t nelm)
{
	uint32_t i=0;

	doc->id = list->doc_hits[start].id;
	doc->ndochits = nelm;
	doc->field = list->field;
	doc->dochit_idx = doc->hit_idx = 0;

	for (i=0; i<nelm; i++) {
		doc->dochits[i] = &(list->doc_hits[i+start]);
		doc->relevancy[i] = &(list->relevancy[i+start]);
	}
}
static void init_result_document(index_document_t *doc,
						index_list_t *list, uint32_t start, uint32_t nelm)
{
	int i;

	doc->id = 0;
	doc->ndochits = nelm;
	doc->field = 0;
	doc->dochit_idx = doc->hit_idx = 0;

	for (i=0; i<nelm; i++) {
		doc->dochits[i] = &(list->doc_hits[i+start]);
		doc->relevancy[i] = &(list->relevancy[i+start]);

		doc->dochits[i]->nhits = 0;
		doc->dochits[i]->field = 0;
		*(doc->relevancy[i]) = 0;
	}
}
static hit_t *get_next_hit(index_document_t *doc)
{
	hit_t *hit=NULL;

	sb_assert(doc->dochit_idx < doc->ndochits);

	if (doc->hit_idx >= doc->dochits[doc->dochit_idx]->nhits) {
		(doc->dochit_idx)++;
		(doc->hit_idx)=0;
	}

	if (doc->dochit_idx >= doc->ndochits) {
		return NULL;
	}

	hit = &(doc->dochits[doc->dochit_idx]->hits[doc->hit_idx]);

	(doc->hit_idx)++;
	return hit;
}
static uint32_t append_to_result_document(index_document_t *doc,
										index_document_t *result)
{
	uint32_t i=0;
	uint32_t max = result->ndochits;

	for (i=0; i<doc->ndochits && i<max; i++) {
		*(result->dochits[i]) = *(doc->dochits[i]);
		*(result->relevancy[i]) = *(doc->relevancy[i]);
	}
	result->ndochits = doc->ndochits < max ? doc->ndochits : max;

	return result->ndochits;
}

/* XXX: and operated list will hold last operand's position */
static uint32_t operate_and_each_document(index_document_t *doc1,
										index_document_t *doc2,
										index_document_t *result)
{
	return append_to_result_document(doc2, result);
}

static int max_result_dochits_per_doc = 5;
static void operate_and_each_list(index_list_t *l1, index_list_t *l2, 
							index_list_t *result)
{
	uint32_t idx1=0, idx2=0, idx3=0;
	uint32_t ndochits1=0, ndochits2=0, ndochits=0;
	uint32_t max_dochits=0;
	index_document_t doc1, doc2, result_doc;

	result->field = (l1->field & l2->field);
	/* XXX: should be moved to somewhere else */
	sb_assert(max_result_dochits_per_doc < MAX_DOCHITS_PER_DOCUMENT);

	while (idx1 < l1->ndochits && idx2 < l2->ndochits) {
		if (l1->doc_hits[idx1].id < l2->doc_hits[idx2].id) {
			idx1 += get_num_of_successive_dochit_same_to_first_docid(
									l1->doc_hits + idx1, l1->ndochits - idx1);
		}
		else if (l1->doc_hits[idx1].id > l2->doc_hits[idx2].id) {
			idx2 += get_num_of_successive_dochit_same_to_first_docid(
									l2->doc_hits + idx2, l2->ndochits - idx2);
		}
		else {
			ndochits1 = get_num_of_successive_dochit_same_to_first_docid(
									l1->doc_hits + idx1, l1->ndochits - idx1);
			ndochits2 = get_num_of_successive_dochit_same_to_first_docid(
									l2->doc_hits + idx2, l2->ndochits - idx2);

			max_dochits = max_result_dochits_per_doc;
			if (max_dochits > result->list_size - idx3) {
				max_dochits = result->list_size - idx3;
			}

			init_index_document(&doc1, l1, idx1, ndochits1);
			init_index_document(&doc2, l2, idx2, ndochits2);
			init_result_document(&result_doc, result, idx3, max_dochits);

			//ndochits = operate_and_each_document(&doc1, &doc2, &result_doc);
			
			ndochits =  append_to_result_document(&doc1, &result_doc);
			
			max_dochits = max_result_dochits_per_doc;
			if (max_dochits > result->list_size - (idx3+ndochits))
			{
				max_dochits = result->list_size - (idx3+ndochits);
			}
				
			init_result_document(&result_doc, result, (idx3+ndochits), max_dochits);
									
			ndochits += append_to_result_document(&doc2, &result_doc);
			
			sb_assert(ndochits > 0);

			idx1 += ndochits1;
			idx2 += ndochits2;
			idx3 += ndochits;
		}

		if (idx3 >= result->list_size) {
			warn("idx1:%u idx2:%u (idx3:%u >= result->list_size:%u)",
						idx1, idx2, idx3, result->list_size);
			sb_assert(idx3 == result->list_size);
			break;
		}
	}

	result->ndochits = idx3;
	sz_snprintf(result->word, STRING_SIZE, "%s and %s", l1->word, l2->word);
}

static int operate_and(sb_stack_t *stack, QueryNode *and)
{
	int num_of_operands = 0;
	index_list_t *this=NULL, *l1=NULL, *l2=NULL;
	index_list_t *operand1=NULL, *operand2=NULL;

	operand2 = stack_peek(stack);
	operand1 = stack_peek_1(stack);
	if ( operand2 != NULL && operand1 != NULL
			&& (operand1->list_type == EXCLUDE || operand2->list_type == EXCLUDE) ) {
		return operator_and_with_not(stack, and);
	}
	
	
	num_of_operands = and->num_of_operands;
	while (num_of_operands >= 2) {
		debug("num_of_operands:%d", num_of_operands);
		l2 = stack_pop(stack);
		l1 = stack_pop(stack);
		num_of_operands -= 2;
		if (l1 == NULL||l2 == NULL) {
			error("l1[%p], l2[%p] after stack_pop", l1, l2);
			release_list2(l1,l2); 
			return FAIL;
		}

		l2 = read_index_list(l2);
		l1 = read_index_list(l1);
		if (l1 == NULL || l2 == NULL) { 
			crit("l1[%p], l2[%p] after read_index_list", l1, l2);
			release_list2(l1, l2);
			return FAIL;
		}

		// XXX: must deal with fake operand more gracefully
        if (l1->list_type == FAKE_OP) {
			this = l2; l2 = NULL;
			goto DONE_OPERATE_AND;
        }
        if (l2->list_type == FAKE_OP) {
			this = l1; l1 = NULL;
			goto DONE_OPERATE_AND;
        }
		this = get_complete_list();
		if (this == NULL) {
			error("no free node to save the operated result");
			release_list3(this, l1, l2); 
			return FAIL;
		}

		this->list_type = NORMAL;
		operate_and_each_list(l1, l2, this);

		INFO("this[%s]->ndochits:%u, field:%s",
			this->word, this->ndochits, sb_strbin(this->field, sizeof(uint32_t)));
DONE_OPERATE_AND:
		release_list2(l1, l2);
		stack_push(stack, this);
		num_of_operands++;
	}
	show_num_of_free_nodes();
	show_stack(stack);
	return SUCCESS;
}

static int32_t get_shortest_dist_with_a_dochit(uint32_t field,
						doc_hit_t *dochit1, doc_hit_t *dochit2)
{
	int i=0, j=0;
	int32_t dist=INT32_MAX, tmp=0;
	uint32_t abs_dist=UINT32_MAX, abs_tmp=0;
	int32_t field1=0, field2=0;
	int32_t pos1=0, pos2=0;

	for (i=0; i<dochit1->nhits; i++) {
		field1 = sb_run_get_field(&(dochit1->hits[i]));
		if ( !(field & ((uint32_t)1<<field1)) ) continue;

		pos1 = sb_run_get_position(&(dochit1->hits[i]));
		if (pos1 == MAX_STD_POSITION) continue;

		for (j=0; j<dochit2->nhits; j++) {
			field2 = sb_run_get_field(&(dochit2->hits[j]));
			if ( !(field & ((uint32_t)1<<field2)) ) continue;

			if (field1 != field2) continue;

			pos2 = sb_run_get_position(&(dochit2->hits[j]));
			if (pos2==MAX_STD_POSITION) continue;

			tmp = pos2-pos1;
			abs_tmp = tmp>0 ? tmp: -tmp;
			if (abs_tmp < abs_dist) {
				abs_dist = abs_tmp;
				dist = tmp;
			}
		}
	}

	return dist;
}

static int32_t get_ordered_shortest_dist_with_a_dochit(uint32_t field,
						doc_hit_t *dochit1, doc_hit_t *dochit2)
{
	int i=0, j=0;
	int32_t dist=INT32_MAX, tmp=0;
	int32_t field1=0, field2=0;
	int32_t pos1=0, pos2=0;

	for (i=0; i<dochit1->nhits; i++) {
		field1 = sb_run_get_field(&(dochit1->hits[i]));
		if ( !(field & ((uint32_t)1<<field1)) ) {
			continue;
		}

		pos1 = sb_run_get_position(&(dochit1->hits[i]));
		if (pos1 == MAX_STD_POSITION) continue;

		for (j=0; j<dochit2->nhits; j++) {
			field2 = sb_run_get_field(&(dochit2->hits[j]));
			if ( !(field & ((uint32_t)1<<field2)) ) {
				continue;
			}

			if (field1 != field2) {
				continue;
			}

			pos2 = sb_run_get_position(&(dochit2->hits[j]));
			if (pos2==MAX_STD_POSITION) continue;

			tmp = pos2-pos1;
			if (tmp < 0) {
				continue;
			}

			if (tmp < dist) 
				dist = tmp;
		}
	}

	sb_assert(dist >= 0);
	return dist;
}

static int get_shortest_dist(uint32_t field,
							doc_hit_t dochits1[], uint32_t nelm1,
							doc_hit_t dochits2[], uint32_t nelm2)
{
	uint32_t i=0,j=0;
	int32_t dist=INT32_MAX, tmp=0;
	uint32_t abs_dist=UINT32_MAX, abs_tmp=0;

	for (i=0; i<nelm1; i++) {
		for (j=0; j<nelm2; j++) {
			tmp = 
				get_shortest_dist_with_a_dochit(field, dochits1+i, dochits2+j);
			abs_tmp = tmp>0 ? tmp:-tmp;
			if (abs_tmp < abs_dist) {
				abs_dist = abs_tmp;
				dist = tmp;
			}
		}
	}
	return dist;
}
static uint32_t get_abs_shortest_dist(uint32_t field,
							doc_hit_t dochits1[], uint32_t nelm1,
							doc_hit_t dochits2[], uint32_t nelm2)
{
	uint32_t i=0,j=0;
	int32_t dist=INT32_MAX, tmp=0;
	uint32_t abs_dist=UINT32_MAX, abs_tmp=0;

	for (i=0; i<nelm1; i++) {
		for (j=0; j<nelm2; j++) {
			tmp = 
				get_shortest_dist_with_a_dochit(field, dochits1+i, dochits2+j);
			abs_tmp = tmp>0 ? tmp:-tmp;
			if (abs_tmp < abs_dist) {
				abs_dist = abs_tmp;
				dist = tmp;
			}
		}
	}
	return abs_dist;
}
static uint32_t get_ordered_shortest_dist(uint32_t field,
							doc_hit_t dochits1[], uint32_t nelm1,
							doc_hit_t dochits2[], uint32_t nelm2)
{
	uint32_t i=0,j=0;
	int32_t dist=INT32_MAX, tmp=0;

	for (i=0; i<nelm1; i++) {
		for (j=0; j<nelm2; j++) {
			tmp = get_ordered_shortest_dist_with_a_dochit(
								field, dochits1+i, dochits2+j);
			sb_assert(tmp >= 0);

			if (tmp < dist) {
				dist = tmp;
			}
		}
	}

	sb_assert(dist >= 0);
	return dist;
}
static int is_within_dist(hit_t *hit1, hit_t *hit2, uint16_t given_dist)
{
	int32_t dist=0;
	uint32_t pos1=0, pos2=0, abs_dist=0;

	if ( sb_run_get_field(hit1) != sb_run_get_field(hit2) ) {
		return FAIL;
	}
	
	pos1 = sb_run_get_position(hit1);
	pos2 = sb_run_get_position(hit2);
	dist = pos2 - pos1;
	abs_dist = dist > 0 ? dist:-dist;

	if (pos1 == MAX_STD_POSITION || pos2 == MAX_STD_POSITION)
		return FAIL;

	if (abs_dist > given_dist)
		return FAIL;

	return SUCCESS;
}
static int is_within_ordered_dist(hit_t *hit1, hit_t *hit2, uint16_t given_dist)
{
	int32_t dist=0;
	uint32_t pos1=0, pos2=0;

	if ( sb_run_get_field(hit1) != sb_run_get_field(hit2) ) {
		return FAIL;
	}
	
	pos1 = sb_run_get_position(hit1);
	pos2 = sb_run_get_position(hit2);
	dist = pos2 - pos1;

	if (dist < 0) 
		return FAIL;

	if (pos1 == MAX_STD_POSITION || pos2 == MAX_STD_POSITION)
		return FAIL;

	if (dist > given_dist)
		return FAIL;

	return SUCCESS;
}

static uint32_t operate_within_each_document(index_document_t *doc1,
										index_document_t *doc2,
										index_document_t *result, uint16_t dist)
{
	hit_t *hit1=NULL, *hit2=NULL;
	uint32_t docid = doc1->id;
	uint32_t dochit_idx=0, hit_idx=0;
	uint32_t max = result->ndochits;

	sb_assert(doc1->id == doc2->id);
	
	set_idx_to_first_hit(doc2);
	while (1) {
		hit2 = get_next_hit(doc2);

		if (hit2 == NULL) break;

		set_idx_to_first_hit(doc1);
		while (1) {
			hit1 = get_next_hit(doc1);
			if (hit1 == NULL) break;

			if (is_within_dist(hit1, hit2, dist) == TRUE) {
				(result->dochits[dochit_idx]->id) = docid;
				(result->dochits[dochit_idx]->field) |= sb_run_get_field(hit2);
				(result->dochits[dochit_idx]->nhits)++;
				(result->dochits[dochit_idx]->hits[hit_idx]) = *hit2;

				*(result->relevancy[dochit_idx]) += (*(doc2->relevancy[doc2->dochit_idx]))+ (*(doc1->relevancy[doc1->dochit_idx])); //  *(doc1->relevancy[dochit_idx]);
				
				hit_idx++;

			}

			if (hit_idx >= STD_HITS_LEN) {
				dochit_idx++;
				hit_idx=0;
			}
			
			if (dochit_idx >= max) {
				goto DONE_OPERATE_WITHIN_EACH_DOCUMENT;
			}
		}
	}
	if (hit_idx != 0) dochit_idx++;
DONE_OPERATE_WITHIN_EACH_DOCUMENT:
	result->ndochits = dochit_idx;
	
	
	
	return dochit_idx;
}
static uint32_t operate_phrase_each_document(index_document_t *doc1,
										index_document_t *doc2,
										index_document_t *result, uint16_t dist)
{
	hit_t *hit1=NULL, *hit2=NULL;
	uint32_t docid = doc1->id;
	uint32_t dochit_idx=0, hit_idx=0;
	uint32_t max = result->ndochits;
		
	sb_assert(doc1->id == doc2->id);

	set_idx_to_first_hit(doc2);
	while (1) {
		hit2 = get_next_hit(doc2);
		if (hit2 == NULL) break;

		set_idx_to_first_hit(doc1);
		while (1) {
			hit1 = get_next_hit(doc1);
			if (hit1 == NULL) break;

			if (is_within_ordered_dist(hit1, hit2, dist) == TRUE) {
				(result->dochits[dochit_idx]->id) = docid;
				(result->dochits[dochit_idx]->field) |= sb_run_get_field(hit2);
				(result->dochits[dochit_idx]->nhits)++;
				/* phrase operation 시에는 앞 단어의 position을 들고가야 한다 */
				(result->dochits[dochit_idx]->hits[hit_idx]) = *hit1;  

				*(result->relevancy[dochit_idx]) += (*(doc2->relevancy[doc2->dochit_idx]))+ (*(doc1->relevancy[doc1->dochit_idx])); //  *(doc1->relevancy[dochit_idx]);
												
				hit_idx++;
				
				
			}

			if (hit_idx >= STD_HITS_LEN) {
				dochit_idx++;
				hit_idx=0;
			}
			
			if (dochit_idx >= max) {
				goto DONE_OPERATE_PHRASE_EACH_DOCUMENT;
			}
		}
	}
	if (hit_idx != 0) dochit_idx++;
DONE_OPERATE_PHRASE_EACH_DOCUMENT:
	result->ndochits = dochit_idx;

	return dochit_idx;
}

static void operate_within_each_list(index_list_t *l1, index_list_t *l2,
								index_list_t *result, uint16_t dist)
{
	uint32_t idx1=0, idx2=0, idx3=0;
	uint32_t ndochits1=0, ndochits2=0, ndochits3=0;
	uint32_t max_dochits=0;
	uint32_t field = (l1->field & l2->field);
	index_document_t doc1, doc2, result_doc;

	result->field = field;
	DEBUG("dist:%d, field:%s", dist, sb_strbin(result->field, sizeof(uint32_t)));
    sb_assert(max_result_dochits_per_doc < MAX_DOCHITS_PER_DOCUMENT);

//	debug("l1[%s] dochit list", l1->word);
//	debug_show_dochitlist(l1->doc_hits, 0, l1->ndochits, stderr);

//	debug("l2[%s] dochit list", l2->word);
//	debug_show_dochitlist(l2->doc_hits, 0, l2->ndochits, stderr);

	DEBUG("l1[%s]->ndochits:%u, l2[%s]->ndochits:%u", 
			l1->word, l1->ndochits, l2->word, l2->ndochits);
	while (idx1 < l1->ndochits && idx2 < l2->ndochits) {
		if (l1->doc_hits[idx1].id < l2->doc_hits[idx2].id) {
			idx1 += get_num_of_successive_dochit_same_to_first_docid(
									l1->doc_hits + idx1, l1->ndochits - idx1);
		}
		else if (l1->doc_hits[idx1].id > l2->doc_hits[idx2].id) {
			idx2 += get_num_of_successive_dochit_same_to_first_docid(
									l2->doc_hits + idx2, l2->ndochits - idx2);
		}
		else {
			ndochits1 = get_num_of_successive_dochit_same_to_first_docid(
									l1->doc_hits + idx1, l1->ndochits - idx1);
			ndochits2 = get_num_of_successive_dochit_same_to_first_docid(
									l2->doc_hits + idx2, l2->ndochits - idx2);
			if (get_abs_shortest_dist(field, l1->doc_hits+idx1, ndochits1, 
							l2->doc_hits+idx2, ndochits2) > dist) {
				idx1 += ndochits1; idx2 += ndochits2;
				continue;
			}

			max_dochits = max_result_dochits_per_doc;
			if (max_dochits > result->list_size - idx3) {
				max_dochits = result->list_size - idx3;
			}
		
			init_index_document(&doc1, l1, idx1, ndochits1);
			init_index_document(&doc2, l2, idx2, ndochits2);
			init_result_document(&result_doc, result, idx3, max_dochits);

			ndochits3 = 
				operate_within_each_document(&doc1, &doc2, &result_doc, dist);
			sb_assert(ndochits3 > 0);

			idx1 += ndochits1;
			idx2 += ndochits2;
			idx3 += ndochits3;
		}

		if (idx3 >= result->list_size) {
			warn("idx1:%u idx2:%u (idx3:%u >= result->list_size:%u)",
						idx1, idx2, idx3, result->list_size);
			sb_assert(idx3 == result->list_size);
			break;
		}
	}
	result->ndochits = idx3;
	sz_snprintf(result->word, STRING_SIZE, "%s /%d %s",l1->word, dist, l2->word);

	//CRIT("result[%s] dochit list", result->word);
	//debug_show_dochitlist(result->doc_hits, 0, result->ndochits, stderr);
}

static int operate_within(sb_stack_t *stack, QueryNode *within)
{
	int num_of_operands = 0;
	index_list_t *this=NULL, *l1=NULL, *l2=NULL;
	uint16_t needed_dist = 0;

	num_of_operands = within->num_of_operands;
	needed_dist = within->opParam;

	sb_assert(within->opParam >= 0);
	sb_assert(num_of_operands == 2 || within->opParam == 0);

	while (num_of_operands >= 2) {
		l2 = stack_pop(stack);
		l1 = stack_pop(stack);
		num_of_operands -= 2;
		if (l1 == NULL||l2 == NULL) {
			error("l1[%p] or l2[%p] is NULL, returning FAIL", l1, l2);
			release_list2(l1, l2);
			return FAIL;
		}
		l2 = read_index_list(l2);
		l1 = read_index_list(l1);
		if (l1 == NULL||l2 == NULL) {
			crit("l1[%p], l2[%p] after read_index_list", l1, l2);
			release_list2(l1, l2);
			return FAIL;
		}

		// XXX: must deal with fake operand more gracefully
		if (l1->list_type == FAKE_OP) {
			this = l2; l2 = NULL;
			goto DONE_OPERATE_WITHIN;
		}
		if (l2->list_type == FAKE_OP) {
			this = l1; l1 = NULL;
			goto DONE_OPERATE_WITHIN;
		}

		this = get_complete_list();
		if (this==NULL) {
			error("no free node to save the operated result");
			release_list2(l1, l2);
			return FAIL;
		}

		this->list_type = NORMAL;
		operate_within_each_list(l1, l2, this, needed_dist);
	
		INFO("this[%s]->ndochits:%u, field:%s, needed_dist:%d",
				this->word, this->ndochits,
				sb_strbin(this->field, sizeof(uint32_t)), needed_dist);

DONE_OPERATE_WITHIN:
		release_list2(l1, l2);
		stack_push(stack, this);
		num_of_operands++;
	}
	show_num_of_free_nodes();
	show_stack(stack);
	return SUCCESS;
}

static void operate_phrase_each_list(index_list_t *l1, index_list_t *l2,
								index_list_t *result, uint16_t dist)
{
	uint32_t idx1=0, idx2=0, idx3=0;
	uint32_t ndochits1=0, ndochits2=0, ndochits3=0;
	uint32_t max_dochits=0;
	uint32_t field = (l1->field & l2->field);
	index_document_t doc1, doc2, result_doc;
	int shortest=0;

	result->field = field;
	DEBUG("dist:%d, field:%s", dist, sb_strbin(result->field, sizeof(uint32_t)));
	DEBUG("l1[%s]->ndochits:%u, l2[%s]->ndochits:%u",
			l1->word, l1->ndochits, l2->word, l2->ndochits);
    sb_assert(max_result_dochits_per_doc < MAX_DOCHITS_PER_DOCUMENT);

	//debug("l1[%s] dochit list", l1->word);
	//debug_show_dochitlist(l1->doc_hits, 0, l1->ndochits, stderr);

	//debug("l2[%s] dochit list", l2->word);
	//debug_show_dochitlist(l2->doc_hits, 0, l2->ndochits, stderr);
	while (idx1 < l1->ndochits && idx2 < l2->ndochits) {
		if (l1->doc_hits[idx1].id < l2->doc_hits[idx2].id) {
			idx1 += get_num_of_successive_dochit_same_to_first_docid(
									l1->doc_hits + idx1, l1->ndochits - idx1);
		}
		else if (l1->doc_hits[idx1].id > l2->doc_hits[idx2].id) {
			idx2 += get_num_of_successive_dochit_same_to_first_docid(
									l2->doc_hits + idx2, l2->ndochits - idx2);
		}
		else {
			DEBUG("l1.docid:%u, l2.docid:%u", 
				l1->doc_hits[idx1].id, l2->doc_hits[idx2].id);
			ndochits1 = get_num_of_successive_dochit_same_to_first_docid(
									l1->doc_hits + idx1, l1->ndochits - idx1);
			ndochits2 = get_num_of_successive_dochit_same_to_first_docid(
									l2->doc_hits + idx2, l2->ndochits - idx2);
			if ( (shortest=
				get_ordered_shortest_dist(field, l1->doc_hits+idx1, ndochits1,
									l2->doc_hits+idx2, ndochits2) ) > dist) {
				idx1 += ndochits1; idx2 += ndochits2;
				DEBUG("[skipped] shortest:%d", shortest);
				continue;
			}
			DEBUG("[included] shortest:%d", shortest);

			max_dochits = max_result_dochits_per_doc;
			if (max_dochits > result->list_size - idx3) {
				max_dochits = result->list_size - idx3;
			}

			init_index_document(&doc1, l1, idx1, ndochits1);
			init_index_document(&doc2, l2, idx2, ndochits2);
			init_result_document(&result_doc, result, idx3, max_dochits);

			ndochits3 = 
				operate_phrase_each_document(&doc1, &doc2, &result_doc, dist);
			sb_assert(ndochits3 > 0);

			idx1 += ndochits1;
			idx2 += ndochits2;
			idx3 += ndochits3;
		}

		if (idx3 >= result->list_size) {
			warn("idx1:%u idx2:%u (idx3:%u >= result->list_size:%u)",
						idx1, idx2, idx3, result->list_size);
			sb_assert(idx3 == result->list_size);
			break;
		}
	}
	result->ndochits = idx3;
	sz_snprintf(result->word, STRING_SIZE, "%s ~ %s", l1->word, l2->word);

	//debug_show_dochitlist(result->doc_hits, 0, result->ndochits, stderr);
}

#define PHRASE_DISTANCE 1
static int operate_phrase(sb_stack_t *stack, QueryNode *phrase)
{
	int num_of_operands = 0;
	index_list_t *this=NULL, *l1=NULL, *l2=NULL;
	uint16_t phrase_distance = PHRASE_DISTANCE;
	
	num_of_operands = phrase->num_of_operands;
	while (num_of_operands >= 2) {
		DEBUG("num_of_operands:%d", num_of_operands);
		l2 = stack_pop(stack);
		l1 = stack_pop(stack);
		num_of_operands -= 2;
		if (l1 == NULL|| l2 == NULL) {
			error("l1[%p] or l2[%p] is NULL, returning FAIL", l1, l2);
			release_list2(l1, l2);
			return FAIL;
		}

		l2 = read_index_list(l2);
		l1 = read_index_list(l1);
		if (l1 == NULL|| l2 == NULL) {
			crit("l1[%p], l2[%p] after read_index_list", l1, l2);
			release_list2(l1, l2);
			return FAIL;
		}

		/* XXX: should deal with fake operand more gracefully */
		if (l1->list_type == FAKE_OP) {
			this = l2; l2 = NULL;
			goto DONE_OPERATE_PHRASE;
		}
		if (l2->list_type == FAKE_OP) {
			this = l1; l1 = NULL;
			goto DONE_OPERATE_PHRASE;
		}

		this = get_complete_list();
		if (this==NULL) {
			error("no free list to save the operated result");
			release_list2(l1, l2);
			return FAIL;
		}

		this->list_type = NORMAL;
		operate_phrase_each_list(l1, l2, this, phrase_distance);

		INFO("this[%s]->ndochits:%u, field:%s, phrase_dist:%d",
				this->word, this->ndochits,
				sb_strbin(this->field, sizeof(uint32_t)), phrase_distance);
DONE_OPERATE_PHRASE:
		release_list2(l1, l2);
		stack_push(stack, this);
		num_of_operands++;
	}
	show_num_of_free_nodes();
	show_stack(stack);
	return SUCCESS;
}

/* XXX: or operated list will hold last operand's position */
static uint32_t operate_or_each_document(index_document_t *doc1,
										index_document_t *doc2,
										index_document_t *result)
{
	return append_to_result_document(doc2, result);
}

static void operate_or_each_list(index_list_t *l1, index_list_t *l2,
								index_list_t *result)
{
	uint32_t idx1=0, idx2=0, idx3=0;
	uint32_t ndochits1=0, ndochits2=0, ndochits3=0;
	uint32_t max_dochits=0;
	index_document_t doc1, doc2, result_doc;

	result->field = (l1->field | l2->field); /* XXX: appropriate?? */
	sb_assert(max_result_dochits_per_doc < MAX_DOCHITS_PER_DOCUMENT);

	//debug("l1[%s] dochit list", l1->word);
	//debug_show_dochitlist(l1->doc_hits, 0, l1->ndochits, stderr);

	//debug("l2[%s] dochit list", l2->word);
	//debug_show_dochitlist(l2->doc_hits, 0, l2->ndochits, stderr);

	while (idx1 < l1->ndochits || idx2 < l2->ndochits) {
		max_dochits = max_result_dochits_per_doc;
		if (max_dochits > result->list_size - idx3) {
			max_dochits = result->list_size - idx3;
		}
		init_result_document(&result_doc, result, idx3, max_dochits);


		if (idx1 == l1->ndochits) {
			ndochits2 = get_num_of_successive_dochit_same_to_first_docid(
										l2->doc_hits + idx2, l2->ndochits - idx2);

			init_index_document(&doc2, l2, idx2, ndochits2);
			ndochits3 = append_to_result_document(&doc2, &result_doc);

			idx2 += ndochits2;
			idx3 += ndochits3;
		}
		else if (idx2 == l2->ndochits) {
			ndochits1 = get_num_of_successive_dochit_same_to_first_docid(
										l1->doc_hits + idx1, l1->ndochits - idx1);

			init_index_document(&doc1, l1, idx1, ndochits1);
			ndochits3 = append_to_result_document(&doc1, &result_doc);

			idx1 += ndochits1;
			idx3 += ndochits3;
		}
		else {
			if (l1->doc_hits[idx1].id < l2->doc_hits[idx2].id) {
				ndochits1 = get_num_of_successive_dochit_same_to_first_docid(
									l1->doc_hits + idx1, l1->ndochits - idx1);

				init_index_document(&doc1, l1, idx1, ndochits1);
				ndochits3 = append_to_result_document(&doc1, &result_doc);

				idx1 += ndochits1;
				idx3 += ndochits3;
			}
			else if (l1->doc_hits[idx1].id > l2->doc_hits[idx2].id) {
				ndochits2 = get_num_of_successive_dochit_same_to_first_docid(
									l2->doc_hits + idx2, l2->ndochits - idx2);

				init_index_document(&doc2, l2, idx2, ndochits2);
				ndochits3 = append_to_result_document(&doc2, &result_doc);

				idx2 += ndochits2;
				idx3 += ndochits3;
			}
			else { /* same docid */
				ndochits1 = get_num_of_successive_dochit_same_to_first_docid(
										l1->doc_hits + idx1, l1->ndochits - idx1);
				ndochits2 = get_num_of_successive_dochit_same_to_first_docid(
										l2->doc_hits + idx2, l2->ndochits - idx2);

				init_index_document(&doc1, l1, idx1, ndochits1);
				init_index_document(&doc2, l2, idx2, ndochits2);

				//ndochits3 = operate_or_each_document(&doc1, &doc2, &result_doc);
		
				ndochits3 = append_to_result_document(&doc1, &result_doc);
						
				max_dochits = max_result_dochits_per_doc;
				if (max_dochits > result->list_size - (idx3+ndochits3)) {
					max_dochits = result->list_size - (idx3+ndochits3);
				}
				
				init_result_document(&result_doc, result, (idx3+ndochits3), max_dochits);
						
				ndochits3 += append_to_result_document(&doc2, &result_doc);
	
				
				idx1 += ndochits1;
				idx2 += ndochits2;
				idx3 += ndochits3;
			}
		}

		if (idx3 >= result->list_size) {
			warn("idx1:%u idx2:%u (idx3:%u >= result->list_size:%u)",
						idx1, idx2, idx3, result->list_size);
			sb_assert(idx3 == result->list_size);
			break;
		}
	}
	
	result->ndochits = idx3;
	sz_snprintf(result->word, STRING_SIZE, "%s or %s", l1->word, l2->word);

	//CRIT("result[%s] dochit list", result->word);
	//debug_show_dochitlist(result->doc_hits, 0, result->ndochits, stderr);
}
static int operate_or(sb_stack_t *stack, QueryNode *or)
{
	index_list_t *l1=NULL, *l2=NULL, *this=NULL;
	int num_of_operands = 0;
	
	num_of_operands = or->num_of_operands;

	DEBUG("start of operation");
	while (num_of_operands >= 2) {
		l2 = stack_pop(stack);
		l1 = stack_pop(stack);
		num_of_operands -= 2;
		if (l1 == NULL || l2 == NULL) {
			error("no free node, releasing every node");
			release_list2(l1, l2);
			return FAIL;
		}

		l2 = read_index_list(l2);
		l1 = read_index_list(l1);
		if (l1 == NULL || l2 == NULL) {
			crit("l1[%p], l2[%p] after read_index_list", l1, l2);
			release_list2(l1, l2);
			return FAIL;
		}

		if (l1->list_type == EXCLUDE || l2->list_type == EXCLUDE) {
			warn("Not OR does not have meaning. ignoring NOT operator");
		}

		if (l1->list_type == FAKE_OP) {
			this = l2; l2 = NULL;
			goto DONE_OPERATE_OR;
		}
		if (l2->list_type == FAKE_OP) {
			this = l1; l1 = NULL;
			goto DONE_OPERATE_OR;
		}

		this = get_complete_list();
		if (this==NULL) {
			error("no free node to save the operated result");
			release_list2(l1, l2);
			return FAIL;
		}

		this->list_type = NORMAL;
		
		operate_or_each_list(l1, l2, this);
	
		INFO("this[%s]->ndochits:%u, field:%s",
			this->word, this->ndochits, sb_strbin(this->field, sizeof(uint32_t)));
DONE_OPERATE_OR:
		release_list2(l1, l2);

		stack_push(stack, this);
		num_of_operands++;
	}

	show_num_of_free_nodes();
	show_stack(stack);
	return SUCCESS;
}

/*int operator_and_not(stack_t *stack, QueryNode *node)*/
int operator_not(sb_stack_t *stack, QueryNode *node)
{
	index_list_t *operand = stack_pop(stack);
	if ( operand == NULL ) {
		error("OP_NOT require 1 operand");
		return FAIL;
	}

	if (node->num_of_operands != 1) {
		error("OP_NOT should have 1 operand but has (%d)",node->num_of_operands);
		stack_push(stack, operand);
		return FAIL;
	}
	operand->list_type = EXCLUDE;
	stack_push(stack, operand);
    return SUCCESS;
}

int operate(sb_stack_t *stack, QueryNode *node)
{
	switch ( node->operator ) {
		case QPP_OP_AND:
			return operate_and(stack, node);
		case QPP_OP_WITHIN:
			return operate_within(stack, node);
		case QPP_OP_OR:
			return operate_or(stack, node);
		case QPP_OP_PHRASE:
			DEBUG("doing phrase operation");
			return operate_phrase(stack, node);
#if 1 /*why ?? if 0 */
		case QPP_OP_NOT:
			return operator_not(stack, node);
#endif
		default:
			error("invalid operator[%d]", node->operator);
	}
	return FAIL;
}

// 마지막 NULL 은 빼고 길이를 maxLen 으로 맞춘다. 그러니까 text는 최소한 maxLen+1
// 한글을 고려해서 자른다.
void cut_string(char* text, int maxLen)
{
	int korCnt = 0, engIdx;
	int textLen = strlen( text );

	if ( textLen <= maxLen ) return;
	else textLen = maxLen;

	for ( engIdx = textLen; engIdx >= 0; engIdx-- ) {
		if ( (signed char)text[engIdx] >= 0 ) break; // 0~127

		korCnt++;
		continue;
	}

	if ( korCnt == 0 || korCnt % 2 == 1 )
		text[textLen] = '\0';
	else text[textLen-1] = '\0';

	return;
}

/*
 * agent_reqeust_t 의 dochits, recv_cnt만 유효하다.
 * doc_hits 의 docid에 대한 comment만 추출하면 된다.
 */
static void fill_title_and_comment_agent(agent_request_t *ar) 
{
    int i = 0, j = 0;

	for(i = 0, j = 0; i < ar->ali.recv_cnt; i++, j++) {
	    doc_hit_t* doc_hits = &ar->ali.agent_doc_hits[i]->doc_hits;
	    char* comment = ar->ali.comments[j];

        if(fill_title_and_comment(doc_hits, comment) != SUCCESS) {
			continue;
		}
	}

	return;
}

static void fill_title_and_comment_server(request_t *req)
{
	int i=0, j=0, last=0;
	int searched_list_size=0;

	if ( req->result_list == NULL || req->result_list->ndochits == 0) {
		info("no searched result");
		return;
	}

	if ( req->result_list->ndochits <  req->first_result ) {
		searched_list_size = 0;
	} else {
        int tmp = 0;
		tmp = req->result_list->ndochits - req->first_result;

		searched_list_size = (tmp > req->list_size) ? req->list_size : tmp;
	}

	if (searched_list_size > COMMENT_LIST_SIZE) {
		error("searched_list_size[%d] > COMMENT_LIST_SIZE[%d]",
					searched_list_size, COMMENT_LIST_SIZE);
		searched_list_size = COMMENT_LIST_SIZE;
	}

	// make up request of abstracted document
	last = req->first_result + searched_list_size;
	
	DEBUG("req->first_result:%d, last:%d",req->first_result,last);
	for (i = req->first_result,j=0; i < last; i++,j++) {
        if(fill_title_and_comment(&req->result_list->doc_hits[i], req->comments[j]) != SUCCESS) {
			continue;
		}
	}

	//XXX: ask jiwon or dong-hun
	for (i=req->first_result,j=0; i<last; i++,j++) {
		req->result_list->doc_hits[j].id = req->result_list->doc_hits[i].id;
		req->result_list->relevancy[j] = req->result_list->relevancy[i];
		req->result_list->doc_hits[j].hitratio = req->result_list->doc_hits[i].hitratio;
	}
}

static int fill_title_and_comment(doc_hit_t* doc_hits, char* comment) {
	int k=0;
	//XXX: result_list->doc_hit index and other index differs.
	DocObject *docBody = 0x00; 
	char *field_value = 0x00; 
	int sizeleft = 0;
	int ret = 0;
	uint32_t docid = doc_hits->id;

	if (docid == 0) {
		crit("docid(%u) < 0",(uint32_t)docid);
		return FAIL;
	}

	// 여기서 문서를 한번 가져온다
	ret = sb_run_doc_get(docid, &docBody); 
	if (ret < 0) { 
		warn("cannot get document object of document[%u]\n", docid); 
		return FAIL;
	} 

	comment[0]='\0'; /* ready for strcat */
	sizeleft = LONG_LONG_STRING_SIZE-1;

	for (k = 0; k < field_count; k++) {
		int ret = 0;

		if(field_info[k].type == NONE) { // enum field_type 참조.
			continue;
		}

		#define max_comment_bytes 1024
		field_value = NULL;

		ret = sb_run_doc_get_field(docBody, NULL, field_info[k].name, &field_value);
		if (ret < 0) {
			error("doc_get_field error for doc[%d], field[%s]", docid, field_info[k].name);
			continue;
		}

		// 구성 : FIELD_NAME:
		strncat(comment, field_info[k].name, sizeleft);
		sizeleft -= strlen(field_info[k].name);
		sizeleft = (sizeleft < 0) ? 0:sizeleft;

		strncat(comment,":",sizeleft);
		sizeleft -= 1;
		sizeleft = (sizeleft < 0) ? 0:sizeleft;

		// 구성 : VALUE;;
		switch(field_info[k].type) {
			case RETURN:
				// 길이가 너무 길면 좀 자른다. 한글 안다치게...
				cut_string( field_value, max_comment_bytes );

				strncat(comment, field_value, sizeleft);
				sizeleft -= strlen(field_value);
				sizeleft = (sizeleft < 0) ? 0:sizeleft;
			break;
			case SUM:
			case SUM_OR_FIRST:
				{
					char summary[210];
					int exist_summary = 0;
					int m = 0;

					summary[0] = '\0';

					for(m = 0; m < doc_hits->nhits; m++) {
						if ( field_info[k].id == doc_hits->hits[m].std_hit.field ) {
							int summary_pos = 0;
							memset(summary, 0x00, 210);

							summary_pos = getAutoComment(field_value, doc_hits->hits[m].std_hit.position-4);
							strncpy(summary, field_value + summary_pos, 201);
							cut_string( summary, 200 );
							exist_summary = 1;
							break;
						}
					}
					
					/* 본문에 단어가  없을경우 */
					if(field_info[k].type == SUM_OR_FIRST && exist_summary == 0) {
						memset(summary, 0x00, 210);
						strncpy(summary, field_value, 201);
						cut_string( summary, 200 );
					}

					strncat(comment, summary, sizeleft);
					sizeleft -= strlen(summary);
					sizeleft = (sizeleft < 0) ? 0:sizeleft;
				}
			break;
		}
		strncat(comment,";;",sizeleft);
		sizeleft -= 2;
		sizeleft = (sizeleft < 0) ? 0:sizeleft;
		sb_free(field_value);

		if (sizeleft <= 0) {
			error("req->comments size lack while pushing comment(field:%s, doc:%u)", field_info[k].name, docid);
			comment[LONG_LONG_STRING_SIZE-1] = '\0';
			error("%s", comment);
			break;
		}
	}

	sb_run_doc_free(docBody);

	return SUCCESS;
}

/*
 * agent 부터 comment 정보를 얻기위해 호출됨
 */
int abstract_info (agent_request_t *ar)
{
	fill_title_and_comment_agent(ar);

	return SUCCESS;
}

int full_info (request_t *r)
{
	return 0;
}

/*********************************************************/
// 시욱 변경 2006/03/06 - 굳이 바꿀필요가 없어서 바꾸지 않는다.
/*
int getAutoComment(char *txt, int position) {
    int pos = 0;
    int isword = 0, word_count = 0, found = 0;

    if (txt == NULL) return 0;

    while(1) {
        char ch = txt[pos];
        switch(ch) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
            case ':':
            case ',':
            case '?':
            case '!':
            case ';':
              if(isword == 1) {
                  word_count++;

                  if(word_count == position) {
                      found = 1; // 찾은 위치부터 시작 단어 사이에 쓰레기 문자를 제거하기 위해
                  }
              }
              isword = 0;
              break;
            default:
              isword = 1;
              if(found == 1) {
                  return pos; //실제 단어의 처음을 리턴한다.
              }
              break;
        }
        pos++;
        if(txt[pos] == 0x00) return 0;
    }

    return 0;
}
*/
int	getAutoComment(char *pszStr, int lPosition)
{
	
	int iStr = 0, lPos=0, nCheck=0;
	
	if (pszStr == NULL) return 0;

START:	
	for ( ; ; )
	{
		if ( !LEXM_IS_WHITE(pszStr[iStr]) )
			break;

		iStr++;
	}

	for ( ; ; )
	{
		if ( !LEXM_IS_EOS(pszStr[iStr]) )
			break;

		iStr++;
	}

	for ( ; ; )
	{
		if ( !LEXM_IS_WHITE(pszStr[iStr]) )
			break;

		iStr++;
	}

	if ( pszStr[iStr] == 0x00 )
		return 0;
		
	for ( ; ; )
	{
		if ( pszStr[iStr] == 0x00 )
			break;
		if ( LEXM_IS_WHITE(pszStr[iStr]) || LEXM_IS_EOS(pszStr[iStr]) )
		{
			lPos++;
			
			if ( lPosition == lPos)
			{
			
				nCheck=1;
			}
			goto START;
			
		}

		if (nCheck==1)
		{
			return iStr;
		}

		iStr++;
	}


	return 0;
}

int full_search (void* word_db, request_t *req)
{
	int ret=FAIL;

	INFO("req.list_size:%d",req->list_size);
	INFO("req.first_result:%d",req->first_result);

	INFO("light searching");
	req->sb4error = 0;
	req->result_list = NULL;
	ret = light_search(word_db, req);
	if (ret < 0) {
		error("light search error");
		if ( req->result_list != NULL ) {
			release_list( req->result_list );
			req->result_list = NULL;
		}
		return FAIL;
	}
	INFO("light_search done");

	INFO("filling title and comment");
	fill_title_and_comment_server(req);
	INFO("filling title and comment done");

#ifdef DEBUG_SOFTBOTD
	show_num_of_free_nodes();
#endif

	return SUCCESS;
}

static int count_operand(QueryNode *qnodes, int num_of_node)
{
    int i, count = 0;
    for (i=0; i<num_of_node; i++) {
        if (qnodes[i].type == OPERAND) {
            count++;
        }
    }
    return count;
}

static int is_fake_qnode(QueryNode *qnode) {
	if (qnode->opParam == -10)
		return TRUE;
	return FALSE;
}

static int do_operate(/*in*/QueryNode qnodes[], /*in*/uint16_t num_of_node,
					  /*out*/ char searchwords[], /*out*/ index_list_t **result)
{
	int i=0;
	index_list_t *new_node=NULL;
	sb_stack_t stack = { 0, NULL, NULL }; /* stack = { size, first, last } */

	DEBUG("num_of_node:%d", num_of_node);
	for (i=0; i<num_of_node ; i++) {
		if ( qnodes[i].type == OPERAND ) {
			new_node = get_incomplete_list();
			if ( new_node == NULL ) {
				warn("no incomplete free list");
				
				goto RELEASE_LIST_AND_FAIL;
			}
			
			/* set index_list_type */
			if (is_fake_qnode(&(qnodes[i])) == TRUE) {
				new_node->list_type = FAKE_OP;
			} else if (qnodes[i].word_st.id == 0) {
				new_node->ndochits = 0;
				new_node->list_type = NOT_EXIST_WORD;
				new_node->ndochits = 0;
			} 
			else {
				/*readin_inv_idx(qnodes[i], new_node); XXX: read when operated */
				set_index_list(&(qnodes[i]), new_node);
				new_node->list_type = NORMAL;
			}

			DEBUG("stack pushing word[%s](wordid[%u]))",
				   qnodes[i].word_st.string, 
				   qnodes[i].word_st.id);
	
			stack_push(&stack, new_node);
		
			strcat(searchwords, qnodes[i].word_st.string);
			strcat(searchwords, "^"); /* FIXME buffer overflow check */
		}
		else if ( qnodes[i].type == OPERATOR ) {
			if (operate(&stack, &(qnodes[i])) == FAIL) {
				goto RELEASE_LIST_AND_FAIL;
			}
				
		}
		else {
			error("node type is neither OPERATOR[%d] nor OPERAND[%d] :[%d]",
											OPERATOR,OPERAND,qnodes[i].type);
			goto RELEASE_LIST_AND_FAIL;
		}
	}

	if ( stack.size == 0 ) { // FIXME: "no document found 라고 나와야 함"
		error("stack is empty after query processed");
		return FAIL;
	} else if ( stack.size > 1 ) { // XXX: stack의 operand중 하나를 return?
		error("invalid stack size [%d]", stack.size);
		goto RELEASE_LIST_AND_FAIL;
	}

	// stack을 깨먹는 ugly code
	stack.first = read_index_list(stack.first);
	if (stack.first == NULL) {
		error("read_index_list failed");
		return FAIL; // stack.first is already released.
	}
	
	*result = stack.first;

	return SUCCESS;

RELEASE_LIST_AND_FAIL:
	{
		index_list_t *list, *tmp = stack.last;
		while ( tmp != NULL ) {
//			CRIT("우하하하: %p", tmp);
			list = tmp;
			tmp = list->prev;
			release_list(list);
		}
		/*
		index_list_t *list;
		while ((list = stack_pop(&stack)) != NULL) {
			release_list(list);
		}
		*/
	}
	return FAIL;
}

static int docattr_sorting(sort_base_t *list, char *sortquery);
static int docattr_filter_sort(index_list_t *list, request_t *req)
{
	docattr_cond_t cond;
	int rv=0;
	
	// AT
	if (sb_run_qp_docattr_query_process(&cond, req->attr_string) == FAIL)
		return FAIL;

	// AT2
	if (sb_run_qp_docattr_query2_process(&cond, req->attr2_string) == FAIL)
		return FAIL;

	// GR
	if (sb_run_qp_docattr_group_query_process(&cond, req->group_string) == FAIL)
		return FAIL;

	CRIT("before filter: %d", list->ndochits);
	if (sb_run_docattr_get_index_list(list, list, SC_COMP, &cond) == -1) {  /* 문서 filitering */
		return FAIL;
	}

	if (list->ndochits == 0) {
		return SUCCESS;
	}

	// rid를 사용해서 중복 문서 삭제
	CRIT("before modify: %d, filtering_id: %d", list->ndochits, req->filtering_id);
	rv = sb_run_docattr_modify_index_list(req->filtering_id, list);
	if (rv == FAIL) {
		return FAIL;
	}

	// 정렬
	CRIT("before sorting: %d", list->ndochits);
	list->sort_base.type = INDEX_LIST;
	if (docattr_sorting((sort_base_t*)list, req->sort_string) == FAIL) {
		return FAIL;
	}	

	CRIT("before filter: %d", list->ndochits);
#if 0
	if (sb_run_docattr_get_index_list(list, list, SC_COMP2, &cond) == FAIL) {  /* 문서 filitering */
		return FAIL;
	}

	list->group_result_count = MAX_GROUP_RESULT;
	if (sb_run_docattr_set_group_result_function(
				&cond, list->group_result, &list->group_result_count) == FAIL) {
		return FAIL;
	}
#endif
	return SUCCESS;
}

/* siouk 작업중 */
static int agent_info_sort(agent_request_t* req)
{
	// 정렬
	CRIT("before sorting: %d", req->ali.recv_cnt);
	req->ali.sort_base.type = AGENT_INFO;
	if (docattr_sorting((sort_base_t*)&(req->ali), req->sh) == FAIL) {
		return FAIL;
	}	
/*
	list->group_result_count = MAX_GROUP_RESULT;
	if (sb_run_docattr_set_group_result_function(
				&cond, list->group_result, &list->group_result_count) == FAIL) {
		return FAIL;
	}
*/
	return SUCCESS;
}

static void reduce_dochits_to_one_per_doc(index_list_t *list)
{
	uint32_t i=0,idx=0, j=0;
	uint32_t current_docid=0;

//	DEBUG("ndochits:%d (before reduce)", list->ndochits);

//	DEBUG("list->doc_hits[0].id :%u", list->doc_hits[0].id);
//	DEBUG("list->doc_hits[1].id :%u", list->doc_hits[1].id);

	idx=0;
	for (i=0; i<list->ndochits; i++) {
		if (list->doc_hits[i].id != current_docid) {
//			DEBUG("list->doc_hits[%d].id:%u", i, list->doc_hits[i].id);
			list->doc_hits[idx] = list->doc_hits[i];
			list->relevancy[idx] = list->relevancy[i];
			idx++;

			current_docid = list->doc_hits[i].id;
			
		}
		else
		{
			list->relevancy[idx-1] += list->relevancy[i];
		}
	}

	for (j=0; j < idx; j++)
	{
		list->doc_hits[j].hitratio = list->relevancy[j];
	}
	
	list->ndochits = idx;
//	DEBUG("ndochits:%d after reduce", list->ndochits);
}

#define MAX_QUERY_NODES 60
int light_search (void* word_db, request_t *req)
{
	int num_of_node, rv;
	QueryNode qnodes[MAX_QUERY_NODES];
	index_list_t *result=NULL;

	req->word_list[0] = '\0';

	if (req->query_string == NULL || strlen(req->query_string) == 0) {
		warn("length of query string is zero");
		return FAIL;
	}

	DEBUG("req.attr_string[%s]",req->attr_string);
	DEBUG("before preprocessing, query str: [%s]",req->query_string);

	num_of_node = 
		sb_run_preprocess(word_db, req->query_string, MAX_QUERY_STRING_SIZE,
									qnodes, MAX_QUERY_NODES);

#ifdef DEBUG_SOFTBOTD
	INFO("postfix query");
	sb_run_print_querynode(qnodes,num_of_node);
#endif

	if (num_of_node < 0) {
		error("preprocess error");
		return FAIL;
	}
	else if (num_of_node == 0) {
		warn("no operand is available");
		// word_list는 가라로 만든다.
		strncpy( req->word_list, " ", sizeof(req->word_list) );
		req->result_list = NULL;
		req->list_size = 0;
		return SUCCESS;
	}
	else if (num_of_node > MAX_QUERY_NODES) {
		crit("num_of_node[%d] > MAX_QUERY_NODES[%d]", num_of_node, MAX_QUERY_NODES);
		crit("setting num_of_node: MAX_QUERY_NODES[%d]", MAX_QUERY_NODES);
		num_of_node = MAX_QUERY_NODES;
		// node가 실종되는 위험을 안고 있다.
	}

	rv = do_operate(qnodes, num_of_node, req->word_list, &result);
	if (rv == FAIL) {
		error("operation failed");
		return FAIL;
	}

	/* in case no AND/OR/WITHIN.., operations are done */
	result = read_index_list(result);
	if (result == NULL) {
		error("read_index_list failed");
		return FAIL;
	}

    if (result->ndochits == 0) {
        req->result_list = result;
        return SUCCESS;
    }

	reduce_dochits_to_one_per_doc(result);
	
	rv = docattr_filter_sort(result, req);
	if (rv == FAIL) {
		release_list( result );
		return FAIL;
	} 


	req->result_list = result;
	// XXX: should be freed by caller

	return SUCCESS;
}

static int docattr_sorting(sort_base_t *list, char *sortquery)
{
	docattr_sort_t sc;
	char *cur, *d1, *d2;
	int i, n = atoi(sortquery);
	char sortingquerydup[MAX_SORT_STRING_SIZE];

	if ( n == 0 ) //관련성 sorting
	{
		DOCSORT_SET_ZERO(&sc);
	    sc.sort_base = list;
		sc.index = 0;
		strncpy(sc.keys[0].key, "0", MAX_SORT_STRING_SIZE);
		sc.keys[0].key[MAX_SORT_STRING_SIZE-1] = '\0';
		
		sc.keys[0].order = -1;
		
		if (sb_run_docattr_index_list_sortby(list, &sc, SC_SORT) == -1) {
			error("error in process of sorting by docattr module");
			return FAIL;
		}
	}
	else
	{
		if (sortingorder[n] == NULL || sortingorder[n][0] == '\0') {
			warn("no sorting condition");
			return SUCCESS;
		}
		strcpy(sortingquerydup, sortingorder[n]);
		sortquery = sortingquerydup;
		CRIT("sortquery:%s",sortquery);
	
		DOCSORT_SET_ZERO(&sc);
	    sc.sort_base = list;
		sc.index = n;
		cur = sortquery; i = 0;
		while ((d1 = strchr(cur, ':')) != NULL) {
			*d1 = '\0';
			strncpy(sc.keys[i].key, cur, MAX_SORT_STRING_SIZE);
			sc.keys[i].key[MAX_SORT_STRING_SIZE-1] = '\0';
			cur = d1 + 1;
	
			if ((d2 = strchr(cur, ';')) == NULL) {
				warn("wrong sorting query: no ordering rule");
				return SUCCESS;
			}
	
			*d2 = '\0';
			if (strcmp(cur, "DESC") == 0) {
				sc.keys[i].order = -1;
			}
			else if (strcmp(cur, "ASC") == 0) {
				sc.keys[i].order = 1;
			}
			else {
				warn("wrong sorting query: there is not a ordering rule[%s]", 
						cur);
				return SUCCESS;
			}
			cur = d2 + 1;
			i++;
		}
	
		if (sb_run_docattr_index_list_sortby(list, &sc, SC_SORT) == -1) {
			error("error in process of sorting by docattr module");
			return FAIL;
		}
	}
	

	return SUCCESS;
}



void sort_list_recursive(uint32_t *relevancy, doc_hit_t *doc_hits,
						 uint32_t lo, uint32_t hi)
{
	uint32_t h,l,p;
	uint32_t tmp;
	doc_hit_t tmp_doc_hit;

	if (lo < hi) {
		l = lo;
		h = hi;
		p = relevancy[hi];
		
		do {
			while ( (l<h) && (relevancy[l]<= p) )
				l++;
			while ( (h>l) && (relevancy[h]>= p) )
				h--;
			if (l<h) {
				tmp = relevancy[l];
				relevancy[l] = relevancy[h];
				relevancy[h] = tmp;

				tmp_doc_hit = doc_hits[l];
				doc_hits[l] = doc_hits[h];
				doc_hits[h] = tmp_doc_hit;
			}
		} while (l<h);

		tmp = relevancy[l];
		relevancy[l] = relevancy[hi];
		relevancy[hi] = tmp;

		tmp_doc_hit = doc_hits[l];
		doc_hits[l] = doc_hits[hi];
		doc_hits[hi] = tmp_doc_hit;

		if (lo < l)
			sort_list_recursive(relevancy,doc_hits,lo,l-1);
		if (l < hi)
			sort_list_recursive(relevancy,doc_hits,l+1,hi);
	}
}

void sort_list(index_list_t *list)
{
	sort_list_recursive(list->relevancy,list->doc_hits,0,list->ndochits-1);
}

static int private_init(void)
{
	int ret=0;

	init_free_list();

	if ( mDbType == TYPE_INDEXDB ) {
		ret = sb_run_indexdb_open( &mIfs, mIdxDbSet );
		if (ret == FAIL) { 
			crit("indexdb_open failed."); 
			return FAIL; 
		} 
	}
	else if ( mDbType == TYPE_VRFI ) {
		ret = sb_run_vrfi_alloc(&mVRFI);
		if (ret == FAIL) {
			error("failed to allocate vrfi");
			return FAIL;
		}

		if (mVRFI == NULL)
			warn("vrf object is null.");

		ret = sb_run_vrfi_open(mVRFI,
				mIdxFilePath,sizeof(inv_idx_header_t), sizeof(doc_hit_t), O_RDONLY);
		if (ret == FAIL) {
			crit("vrf_reopen failed.");
			return FAIL;
		}
	}
	else {
		warn("unknown DbType: %d", mDbType);
		return FAIL;
	}

	return SUCCESS;
}

static int module_init(void)
{
	return SUCCESS;
}

/*****************************************************************************/
static void setDbType(configValue v)
{
	if ( strcasecmp( v.argument[0], "vrfi" ) == 0 ) mDbType = TYPE_VRFI;
	else if ( strcasecmp( v.argument[0], "indexdb" ) == 0 ) mDbType = TYPE_INDEXDB;
	else {
		warn("unknown DbType:%s, ignored", v.argument[0]);
	}
}

static void setIndexDbPath(configValue v)
{
	strncpy(mIdxFilePath,v.argument[0],MAX_PATH_LEN);
	mIdxFilePath[MAX_PATH_LEN-1] = '\0';
	DEBUG("indexer db path: %s",mIdxFilePath);
}

static void setIndexDbSet(configValue v)
{
	mIdxDbSet = atoi( v.argument[0] );
}

static void get_commentfield(configValue v)
{
	if (v.argNum < 7) return;

	if (field_count >= MAX_EXT_FIELD) {
		error("field_count(%d) >= MAX_EXT_FIELD(%d).", field_count,MAX_EXT_FIELD);
		error("Increase MAX_EXT_FIELD and recompile");
		return;
	}


    // 필드정보 저장. 
    field_info[field_count].id = atoi(v.argument[0]);
    strncpy(field_info[field_count].name, v.argument[1], SHORT_STRING_SIZE);

	if (strncasecmp("RETURN",v.argument[6],SHORT_STRING_SIZE) == 0) {
        field_info[field_count].type = RETURN;
    } else if (strncasecmp("SUM",v.argument[6],SHORT_STRING_SIZE) == 0) {
        field_info[field_count].type = SUM;
    } else if (strncasecmp("SUM_OR_FIRST",v.argument[6],SHORT_STRING_SIZE) == 0) {
        field_info[field_count].type = SUM_OR_FIRST;
    } else {
        field_info[field_count].type = NONE;
    }

	INFO("commentfield:[%s] set", field_info[field_count].name);

	field_count++;
}

static void get_FieldSortingOrder(configValue v)
{
	static char _sortingorder[MAX_DOCATTR_SORTING_CONF][MAX_SORT_STRING_SIZE];
	int n = atoi(v.argument[0]);
	if (sortingorder[n]) {
		info("over writing sorting order[%d]: %s", n, sortingorder[n]);
	}
	strncpy(_sortingorder[n], v.argument[1], STRING_SIZE);
	_sortingorder[n][STRING_SIZE-1] = '\0';
	sortingorder[n] = _sortingorder[n];
	info("set sorting order[%d]: %s", n, sortingorder[n]);
}

static config_t config[] = {
	CONFIG_GET("DbType",setDbType,1, "vrfi or indexdb"),
	CONFIG_GET("IndexDbPath",setIndexDbPath,1,
			"inv indexer db path (only vrfi) (e.g: IndexDbPath /home/)"),
	CONFIG_GET("IndexDbSet",setIndexDbSet,1,
			"index db set (type is indexdb) (e.g: IndexDbSet 1)"),

	CONFIG_GET("Field",get_commentfield,VAR_ARG, "Field which needs to be shown in result"),
	CONFIG_GET("FieldSortingOrder",get_FieldSortingOrder,2, "Field sorting order"),
	{NULL}
};

static void register_hooks(void)
{
	/* XXX: module which uses qp should call sb_run_qp_init once after fork. */
	sb_hook_qp_init(private_init,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_light_search(light_search,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_agent_info_sort(agent_info_sort,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_full_search(full_search,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_abstract_info(abstract_info,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_full_info(full_info,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_finalize_search(finalize_search,NULL,NULL,HOOK_MIDDLE);
}

module qp_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	module_init,			/* initialize */
	//module_main,			/* child_main */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};

/* options for vim 
 * vim:ts=4
 */
