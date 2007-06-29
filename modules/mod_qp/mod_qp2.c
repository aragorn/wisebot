/* $Id$ */ 
#include <math.h>
#include <fcntl.h> /* O_RDONLY */
#include <string.h>
#include <errno.h>
#include <inttypes.h> // for PRIu32
#include "common_core.h"
#include "memory.h"
#include "util.h"
#include "common_util.h"
#include "mod_api/qp2.h"
#include "memfile.h"
#include "mod_api/qpp.h"
#include "mod_api/indexdb.h"
#include "mod_api/vrfi.h"
#include "mod_api/lexicon.h"
#include "mod_api/cdm.h"
#include "mod_api/cdm2.h"
#include "mod_api/docapi.h"
#include "mod_api/indexer.h"
#include "mod_api/protocol4.h" /* sb4 related error codes */

#include "mod_qp2.h"
#include "mod_morpheme/lib/lb_lex.h" /* 2228,2236: LEXM_IS_WHITE,LEXM_IS_EOS */

#define TIME_COUNT 1
//#define DEBUGTIME
#include "stopwatch.h"

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
			int32_t pos=dochits[i].hits[j].std_hit.position;
			int32_t field=dochits[i].hits[j].std_hit.field;
			fprintf(stream, "  dochit[%d].hits[%d] field:%u, pos:%u\n",
							i, j, field, pos);
		}
	}
}
#endif

///////////////////////////////////////////////////////////
// qp init 유무
static int qp_initialized = 0;

// word db
static int word_db_set = -1;
static word_db_t* word_db = NULL;
///////////////////////////////////////////////////////////
// field의 갯수 
static int field_count = 0;
static field_info_t field_info[MAX_EXT_FIELD];

///////////////////////////////////////////////////////////
// 최대 요약문 크기
static int max_comment_bytes = 200;
static int max_comment_return_bytes = 200;

///////////////////////////////////////////////////////////
// 구버전 cdm 사용여부
static int b_use_cdm = 0;
///////////////////////////////////////////////////////////

#define MAX_FIELD_SIZE (1024*1024)
///////////////////////////////////////////////////////////
// 검색어 bold 처리
#define MAX_HIGHTLIGHT_TAG 64
enum highlight_type_t {
	H_UNIT_EOJEOL = 0,
	H_UNIT_WORD,
	H_WORD_INPUT,
	H_WORD_PARSED
};
static enum highlight_type_t highlight_unit = H_UNIT_EOJEOL;
static enum highlight_type_t highlight_word = H_WORD_INPUT;
static char highlight_pre_tag[MAX_HIGHTLIGHT_TAG] = {"<B>"};
static char highlight_post_tag[MAX_HIGHTLIGHT_TAG] = {"</B>"};
static char highlight_word_len = 0;

#define MAX_HIGHLIGHT_SEP_LEN 256
static char seps1[MAX_HIGHLIGHT_SEP_LEN] = " \t\r\n!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
static char seps2[MAX_HIGHLIGHT_SEP_LEN] = "，」「·";

///////////////////////////////////////////////////////////
// query에서 필드 추출시 삭제해야할 문자
#define MAX_REMOVE_CHAR_QUERY 64
static char remove_char_query[MAX_REMOVE_CHAR_QUERY] = {"()+&!<>*\""};

enum DbType {
	TYPE_VRFI,
	TYPE_INDEXDB
};

static enum DbType        mDbType = TYPE_VRFI;

static VariableRecordFile *mVRFI = NULL;
static index_db_t         *mIfs  = NULL;

static int                mCdmSet = -1;
static cdm_db_t           *mCdm    = NULL;

static char mIdxFilePath[MAX_PATH_LEN]= "dat/indexer/index"; // used by vrfi
static int mIdxDbSet = -1;

#define MAX_DOCATTR_SORTING_CONF		32
static char *sortingorder[MAX_DOCATTR_SORTING_CONF] = { NULL };

static index_list_t complete_index_list_pool[MAX_INDEX_LIST_POOL];
static index_list_t incomplete_index_list_pool[MAX_INCOMPLETE_INDEX_LIST_POOL];

/* stack related stuff */
typedef struct {
	uint32_t size;
	struct index_list_t *first;
	struct index_list_t *last;
} sb_stack_t;
/* stack related stuff end */

typedef struct {
	uint32_t id;
	uint32_t ndochits; /* < MAX_DOCHITS_PER_DOCUMENT */
	uint32_t field;
	uint32_t *relevance[MAX_DOCHITS_PER_DOCUMENT];
	doc_hit_t *dochits[MAX_DOCHITS_PER_DOCUMENT];

	int dochit_idx;
	int hit_idx;
} index_document_t;

/////////////////////////////////////////////////////////
virtual_document_list_t* g_vdl;
index_list_t* g_result_list;
void* g_docattr_base_ptr = NULL;
int g_docattr_record_size = 0;

/* 
 *  * 모든 function의 parameter에 줄수 없어서 전역변수로 선언 
 *   * init_request에서 assign 된다.
 *   */
request_t* g_request = 0x00;

static char* clause_type_str[] = {
	"SELECT",
	"WEIGHT",
	"SEARCH",
    "VIRTUAL_ID", 
    "WHERE",
    "GROUP_BY",
    "ORDER_BY",
    "COUNT_BY",
    "LIMIT",
	"(",
	")",
	"#",
	"OUTPUT_STYLE",
};

static char* key_type_str[] = { "DOCATTR", "DID", "RELEVANCY", };
static char* order_type_str[] = { "DESC", "", "ASC", }; // DESC:-1, ASC:1
static char* doc_type_str[] = { "DOCUMENT", "VIRTUAL_DOCUMENT", };
static char* output_style_str[] = { "XML", "SOFTBOT4", };

// function protoype
static int	get_start_comment(char *pszStr, int lPosition);
static int	get_start_comment_dha(char *txt, int start_word_pos);

static int init_response(response_t* res);
static int init_request(request_t* res, char* query);
static int cb_index_list_sort(const void* dest, const void* sour, void* userdata);
static int make_virtual_document(index_list_t* list, request_t* req);

static int make_groupby_rule(char* clause, groupby_rule_t* rule);
static int make_groupby_rule_list(char* clause, groupby_rule_list_t* rules);
static int make_limit_rule(char* clause, limit_t* rule);
static int make_orderby_rule_list(char* clause, orderby_rule_list_t* rules);
static int get_query_string(request_t* req, char query[MAX_QUERY_STRING_SIZE]);

static void set_weight(weight_list_t* wl, char* clause);
static void set_output_style(request_t* req, char* clause);
static void set_virtual_id(virtual_rule_list_t* vrl, char* clause);

// document type별 operation
static int virtual_document_orderby(orderby_rule_list_t* rules);
static int virtual_document_where();
static int virtual_document_grouping(groupby_result_list_t* result);
static int virtual_document_group_count(virtual_document_t* vd, 
 	                                    groupby_result_list_t* result, 
										int* is_remove);

static int document_orderby(orderby_rule_list_t* rules);
static int document_where();
static int document_grouping(request_t* req, groupby_result_list_t* result);
static int document_get_group_values(doc_hit_t* doc_hit, int group_values[MAX_GROUP_RULE], groupby_rule_list_t* rules);

// document operation
static int operation_groupby_orderby(groupby_rule_list_t* groupby_rules,
                                     orderby_rule_list_t* orderby_rules,
                                     groupby_result_list_t* result,
									 request_t* req,
									 enum doc_type doc_type);
static int operation_orderby(orderby_rule_list_t* rules, enum doc_type doc_type);
static int operation_where(char* where, enum doc_type doc_type);
static int operation_limit(limit_t* rule, enum doc_type doc_type);

//////////////////////////////////////////////////////////////////////////


static int virtual_document_fill_comment(request_t* req, response_t* res);
static int do_filter_operation(request_t* req, response_t* res, enum doc_type doc_type);
static int get_comment(request_t* req, doc_hit_t* doc_hits, select_list_t* sl, char* comment);

static void add_delete_where(operation_list_t* op_list);
static void print_weight(weight_list_t* wl);
static void print_orderby(orderby_rule_t* rule);
static void print_limit(limit_t* rule);
static void print_groupby(groupby_rule_t* rule);
static void print_select(select_list_t* rule);

static int light_search (request_t *req, response_t *res);
static int full_search (request_t *req, response_t *res);
static int abstract_search (request_t *req, response_t *res);
//////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
/*    start util                                       */
/////////////////////////////////////////////////////////

// 마지막 NULL 은 빼고 길이를 maxLen 으로 맞춘다. 그러니까 text는 최소한 maxLen+1
// 한글을 고려해서 자른다.
//
// maxLen이 '\0'을 가리키고 있으면 한글이 깨지더라도 정상 리턴해버리므로
// '\0'으로 끝난 string을 자르려면 length-1을 maxLen에 줘야 한다.
static void cut_string(char* text, int maxLen)
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

/////////////////////////////////////////////////////////
/*    end util                                       */
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
/*    start stack related stuff                        */
/////////////////////////////////////////////////////////
index_list_t *complete_free_list_root = NULL;
index_list_t *incomplete_free_list_root = NULL;

static int get_num_of_complete_free_list()
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

static int get_num_of_incomplete_free_list()
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
static void _show_num_of_free_nodes(const char *function)
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
	this->relevance = (uint32_t*)sb_calloc(MAX_DOC_HITS_SIZE, sizeof(uint32_t));
	if (this->relevance == NULL) {
		error("fail calling calloc: %s", strerror(errno));
		return NULL;
	}

	return this;
}

static index_list_t *get_incomplete_list(void)
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

static index_list_t *get_complete_list(void)
{
	index_list_t *this = NULL;
	doc_hit_t *doc_hits = NULL;
	uint32_t *relevance = NULL;

	if ( complete_free_list_root == NULL ) {
		warn("complete_free_list_root is NULL - extending set dynamically");
		complete_free_list_root = alloc_complete_index_list(NULL, NULL);
	}

	this = complete_free_list_root;
	complete_free_list_root = complete_free_list_root->next;
	if ( complete_free_list_root ) complete_free_list_root->prev = NULL;

	// pointer가 가리키는 값들을 잃어버리지 않도록..
	doc_hits = this->doc_hits;
	relevance = this->relevance;

	memset(this,0x00,sizeof(index_list_t));

	this->doc_hits = doc_hits;
	this->relevance = relevance;
	this->list_size = MAX_DOC_HITS_SIZE; /* XXX: to where?? */

//	memset(this->doc_hits, 0, sizeof(doc_hit_t) * MAX_DOC_HITS_SIZE);
//	memset(this->relevance, 0, sizeof(uint32_t) * MAX_DOC_HITS_SIZE);

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
		this->relevance = (uint32_t*)sb_calloc(MAX_DOC_HITS_SIZE, sizeof(uint32_t));
		if (this->relevance == NULL) {
			error("relevance calloc(%d bytes) failed: %s",
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

static void _show_stack(const char *function, sb_stack_t *stack)
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

static void stack_push(sb_stack_t *stack, index_list_t *this)
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

static index_list_t *stack_peek(sb_stack_t *stack)
{
	if ( stack->size == 0 ) {
		warn("empty stack");
		return NULL;
	}

	return stack->last;
}

// 맨 마지막 바로 앞을 가져온다... 쯥...
static index_list_t *stack_peek_1(sb_stack_t *stack)
{
	if ( stack->size <= 1 ) {
		warn("not enough stack size");
		return NULL;
	}

	return stack->last->prev;
}

static index_list_t *stack_pop(sb_stack_t *stack)
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
			sb_free( this->relevance );
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

static int finalize_search(request_t *req, response_t *res)
{
	release_list(g_result_list);

	return SUCCESS;
}

static void init_stack(sb_stack_t *stack)
{
	stack->size = 0;
	stack->first = NULL;
	stack->last = NULL;
}

/////////////////////////////////////////////////////////
/*    end stack related stuff                          */
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
/*    start search operation stuff                     */
/////////////////////////////////////////////////////////
// return 값은 읽어온 doc_hits 갯수. ( >= 0 )
// 에러인 경우는 FAIL
static int read_from_db(uint32_t wordid, char* word, doc_hit_t* doc_hits)
{
	int offset, length;
	int ndochits, ret;

	if ( mDbType == TYPE_INDEXDB ) {
//time_mark("read db start");
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

//time_mark("read db end");
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
	int i=0, idx=0, cnt=0, nWordHit=0, tot_index_doccnt=0, j=0, w=0;
	enum index_list_type list_type = NORMAL;
	uint32_t weight=0;

    weight_list_t* wl = &g_request->weight_list;

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
				weight = 0;
				
				for(j=0; j < cnt; j++)
				{
					//debug("%u %u", list->doc_hits[i].hits[j].std_hit.field, field);
					if ( ( ((uint32_t)1) << list->doc_hits[i].hits[j].std_hit.field ) & field)
					{
						nWordHit ++;
						
                        for(w = 0; w < wl->cnt; w++) {
							if ( wl->list[w].field_id ==  list->doc_hits[i].hits[j].std_hit.field  ) {
								weight += wl->list[w].weight;
                                debug("did[%u], weight:%d(%s)\n", list->doc_hits[i].id, weight, wl->list[w].name);
                            }
                        }
					}
				}	
				
                // reset일 경우 기존 relevance를 모두 무시한다.
                if(wl->reset) {
					list->relevance[idx-1] = wl->reset_value + weight;
                } else {
					tot_index_doccnt = sb_run_last_indexed_did();
					list->relevance[idx-1] = (((int)log10( (tot_index_doccnt*3) / (ndochits))+1 ) * nWordHit)+weight;
                }
			}
		}
		list->ndochits = idx;
	}
	else {
		// 초기화 해야 한다
		memset( list->relevance, 0, sizeof(uint32_t)*ndochits );
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

static void insert_one_to_result(index_list_t *dest, int idx, 
					  index_list_t *l1, int idx1) {
	dest->doc_hits[idx] = l1->doc_hits[idx1];
	dest->relevance[idx] = l1->relevance[idx1];

	return;
}


static int operator_and_with_not(sb_stack_t *stack, QueryNode *qnode)
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
		doc->relevance[i] = &(list->relevance[i+start]);
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
		doc->relevance[i] = &(list->relevance[i+start]);

		doc->dochits[i]->nhits = 0;
		doc->dochits[i]->field = 0;
		*(doc->relevance[i]) = 0;
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
		*(result->relevance[i]) = *(doc->relevance[i]);
	}
	result->ndochits = doc->ndochits < max ? doc->ndochits : max;

	return result->ndochits;
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
		field1 = dochit1->hits[i].std_hit.field;
		if ( !(field & ((uint32_t)1<<field1)) ) continue;

		pos1 = dochit1->hits[i].std_hit.position;
		if (pos1 == MAX_STD_POSITION) continue;

		for (j=0; j<dochit2->nhits; j++) {
			field2 = dochit2->hits[j].std_hit.field;
			if ( !(field & ((uint32_t)1<<field2)) ) continue;

			if (field1 != field2) continue;

			pos2 = dochit2->hits[j].std_hit.position;
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
		field1 = dochit1->hits[i].std_hit.field;
		if ( !(field & ((uint32_t)1<<field1)) ) {
			continue;
		}

		pos1 = dochit1->hits[i].std_hit.position;
		if (pos1 == MAX_STD_POSITION) continue;

		for (j=0; j<dochit2->nhits; j++) {
			field2 = dochit2->hits[j].std_hit.field;
			if ( !(field & ((uint32_t)1<<field2)) ) {
				continue;
			}

			if (field1 != field2) {
				continue;
			}

			pos2 = dochit2->hits[j].std_hit.position;
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

	if ( hit1->std_hit.field != hit2->std_hit.field ) {
		return FAIL;
	}
	
	pos1 = hit1->std_hit.position;
	pos2 = hit2->std_hit.position;
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

	if ( hit1->std_hit.field != hit2->std_hit.field ) {
		return FAIL;
	}
	
	pos1 = hit1->std_hit.position;
	pos2 = hit2->std_hit.position;
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
				(result->dochits[dochit_idx]->field) |= 1<<hit2->std_hit.field;
				(result->dochits[dochit_idx]->nhits)++;
				(result->dochits[dochit_idx]->hits[hit_idx]) = *hit2;

				*(result->relevance[dochit_idx]) += (*(doc2->relevance[doc2->dochit_idx]))+ (*(doc1->relevance[doc1->dochit_idx])); //  *(doc1->relevance[dochit_idx]);
				
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
				(result->dochits[dochit_idx]->field) |= 1<<hit2->std_hit.field;
				(result->dochits[dochit_idx]->nhits)++;
				/* phrase operation 시에는 앞 단어의 position을 들고가야 한다 */
				(result->dochits[dochit_idx]->hits[hit_idx]) = *hit1;  

				*(result->relevance[dochit_idx]) += (*(doc2->relevance[doc2->dochit_idx]))+ (*(doc1->relevance[doc1->dochit_idx])); //  *(doc1->relevance[dochit_idx]);
												
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
static int operator_not(sb_stack_t *stack, QueryNode *node)
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

static int operate(sb_stack_t *stack, QueryNode *node)
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

static int is_fake_qnode(QueryNode *qnode) {
	if (qnode->opParam == -10)
		return TRUE;
	return FALSE;
}

static int do_search_operation(/*in*/QueryNode qnodes[], /*in*/uint16_t num_of_node,
					 /*out*/ index_list_t **result)
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
			list = tmp;
			tmp = list->prev;
			release_list(list);
		}
	}
	return FAIL;
}

/////////////////////////////////////////////////////////
/*    end search operation stuff                       */
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
/*    start search result filtering operation stuff    */
/////////////////////////////////////////////////////////
static int is_exist_return_field(char* field_name, int* idx)
{
    int i = 0;

    for(i = 0; i < field_count; i++) {
	    if ( strcasecmp(field_info[i].name, field_name ) == 0 ) {
            *idx = i;
            return TRUE;
        }
    }

    return FALSE;        
}

static int is_seperator(char* c, int* sep_size)
{
	int seps1_cnt = strlen(seps1);
	int seps2_cnt = strlen(seps2);

	int i = 0;

    for(i = 0; i < seps1_cnt; i++) {
        if(*c == seps1[i]) {
            *sep_size = 1;
            return TRUE; 
        }
	}

    for(i = 0; i < seps2_cnt; i+=2) {
        if(*c == seps2[i] && *(c+1) == seps2[i+1]) {
            *sep_size = 2;
            return TRUE; 
        }
	}

    *sep_size = 1;

	return FALSE;
}


static void highlight_string_by_eojeol(char* str, word_list_t* wl)
{
	int i = 0;
	char buf[STRING_SIZE] = "";

    for(i = 0; i < wl->cnt; i++) {
        char* p = strstr(str, wl->word[i]);
		if (p == NULL) continue;

		strcat(buf, highlight_pre_tag);
		strcat(buf, str);
		strcat(buf, highlight_post_tag);

		sz_strncpy(str, buf, STRING_SIZE);
		/* 어절단위로 강조표시하므로, 하나라도 일치하면, 강조표시 완료이다. */
		return;
	}
}

static void highlight_string_by_word(char* str, word_list_t* wl)
{
	int i = 0;
	char buf[STRING_SIZE] = "";

    for(i = 0; i < wl->cnt; i++)
	{
        char* pos = __strcasestr(str, wl->word[i]);
		if (pos == NULL) continue;

        /* 알파벳 1글자인 경우, 단어 중 부분일치하는 것은 넘어간다.
         * ex) i 로 검색한 경우, which 에서 i는 match하지 않아야 한다.
               그러나 i포드 에서는 match한다. */
        if ( strlen(wl->word[i]) == 1 
             && (isalpha(*(pos-1)) || isalpha(*(pos+1))) ) continue;

		strncpy(buf, str, pos-str); buf[pos-str] = '\0';
		//debug("strncpy(buf[%s],str[%s],pos[%p]-str[%p]=%d)", buf, str, pos, str, pos-str);
		strcat(buf, highlight_pre_tag);
		//debug("1. buf[%s] word[%s]", buf, wl->word[i]);
		strcat(buf, wl->word[i]);
		//debug("2. buf[%s] word[%s]", buf, wl->word[i]);
		strcat(buf, highlight_post_tag);
		//debug("3. buf[%s] word[%s]", buf, wl->word[i]);
		strcat(buf, pos+strlen(wl->word[i]));
		//debug("4. buf[%s] word[%s]", buf, wl->word[i]);
		
		sz_strncpy(str, buf, STRING_SIZE);
		//debug("E. buf[%s] word[%s]", buf, wl->word[i]);
	}
}

/* 
 * FIXME: field_value의 tag 추가로 인해 크기가 늘어나게 되는데
 *        buffer 경계를 체크하지 않는 문제가 있다. --정시욱
 */
static int highlight(char* field_value, word_list_t* wl) 
{
	memfile *buffer = memfile_new();
	char* p = field_value;
    char* s = field_value;
	char eojeol[STRING_SIZE*2];
	char sep[3] = {0,0,0};
	int len = 0;
	int rv = 0;
	int i;

	if (wl->cnt == 0)
	{
		debug("no word to highlight");
		return SUCCESS;
	}
    for(i = 0; i < wl->cnt; i++)
	{
		debug("word[%d]=[%s]", i, wl->word[i]);
	}

    while( 1 ) {
        int sep_size = 1;
		if( is_seperator(p, &sep_size) || *p == '\0') {
			sep[0] = *p;
			sep[1] = '\0';
			*p = '\0';

            if(sep_size == 2) {
			    sep[1] = *(p+1);
			    sep[2] = '\0';
			    *(p+1) = '\0';
            }

			sz_strncpy(eojeol, s, STRING_SIZE);

			if (highlight_unit == H_UNIT_WORD)
			  highlight_string_by_word(eojeol, wl);
			else
			  highlight_string_by_eojeol(eojeol, wl);

			debug("[%s] -> [%s]", s, eojeol);
	        rv = memfile_append(buffer, eojeol, strlen(eojeol));
			if(rv < 0) {
				error("can not append memfile");
				memfile_free(buffer);
				return FAIL;
			}

			/* 마지막 어절 체크 */
			if(sep[0] == '\0') break;

	        rv = memfile_append(buffer, sep, strlen(sep));
			if(rv < 0) {
				error("can not append memfile");
				memfile_free(buffer);
				return FAIL;
			}

			s = p + sep_size;
		}

		p += sep_size;
	}

	memfile_setOffset(buffer, 0);
	len = memfile_getSize(buffer);

	if(len >= MAX_FIELD_SIZE) {
		error("field value size[%d] larger than MAX_FIELD_SIZE[%d]",
				len, MAX_FIELD_SIZE);
		memfile_free(buffer);
		return FAIL;
	}

	rv = memfile_read(buffer, field_value, len);
	if(rv != len) {
		error("can not appendF memfile");
		memfile_free(buffer);
		return FAIL;
	}
	field_value[len] = '\0';

	memfile_free(buffer);
	return SUCCESS;
}

static int get_comment(request_t* req, doc_hit_t* doc_hits, select_list_t* sl, char* comment) 
{
	int i = 0, k = -1, len = 0;
	//XXX: result_list->doc_hit index and other index differs.
	DocObject *docBody = 0x00; 
	cdm_doc_t* cdmdoc;
	memfile *buffer = memfile_new();
    int all_highlight = 0;

	char *field_value = 0x00; 
	static char* _field_value = NULL;

	int rv = 0;
	uint32_t docid = doc_hits->id;
	enum output_style output_style = req->output_style;

    if(_field_value == NULL) {
        _field_value = sb_malloc(MAX_FIELD_SIZE);
		if(_field_value == NULL) {
            error("not enough memory");
			return FAIL;
		}
    }

	if (docid == 0) {
		MSG_RECORD(&req->msg, error, "docid(%u) < 0",(uint32_t)docid);
		memfile_free(buffer);
		return FAIL;
	}

	// 여기서 문서를 한번 가져온다
	if ( b_use_cdm ) { // mod_cdm
		rv = sb_run_doc_get(docid, &docBody); 
	}
	else { // cdm2 api
		rv = sb_run_cdm_get_doc(mCdm, docid, &cdmdoc);
	}

	if (rv < 0) { 
		MSG_RECORD(&req->msg, error, "cannot get document object of document[%u]\n", docid); 
		memfile_free(buffer);
		return FAIL;
	} 

	if(sl->field[0].name[0] == '*') {
		sl->cnt = field_count;

        all_highlight = sl->field[0].is_highlight;
    }

	if(output_style == STYLE_XML) {
		rv = memfile_appendF(buffer, "<fields count=\"%d\">", sl->cnt);
		if(rv < 0) {
			MSG_RECORD(&req->msg, error, "can not appendF memfile");
			memfile_free(buffer);
			return FAIL;
		}
	} else {
		; //do nothing
	}

	for (i = 0; i < sl->cnt; i++) {
		if(sl->field[0].name[0] == '*') {
			if(k < field_count) {
				k++;
			}
		} else if(is_exist_return_field(sl->field[i].name, &k) == TRUE) {
            // select 항목에 존재하고 NONE이 아닐경우에만 추출한다.
			if(field_info[k].type == NONE) { // enum field_type 참조.
				continue;
			}
        } else {
            continue;
        }

		field_value = NULL;

		if ( b_use_cdm ) { // mod_cdm
			rv = sb_run_doc_get_field(docBody, NULL, field_info[k].name, &field_value);
			if (rv < 0) {
				error("doc_get_field error for doc[%d], field[%s]", docid, field_info[k].name);
				continue;
			}
		}
		else { // cdm2 api
			field_value = _field_value;
			rv = sb_run_cdmdoc_get_field(cdmdoc, field_info[k].name, field_value, MAX_FIELD_SIZE);
			if ( rv < 0 && rv != CDM2_NOT_ENOUGH_BUFFER ) {
				error("cannot get field[%s] from doc[%"PRIu32"]", field_info[k].name, docid);
				continue;
			}
		}

		// 구성 : FIELD_NAME:
	    if(output_style == STYLE_XML) {
			rv = memfile_appendF(buffer, "<field name=\"%s\">", field_info[k].name);
			if(rv < 0) {
				MSG_RECORD(&req->msg, error, "can not appendF memfile");
				memfile_free(buffer);
				return FAIL;
			}
		} else {
			rv = memfile_appendF(buffer, "%s:", field_info[k].name);
			if(rv < 0) {
				MSG_RECORD(&req->msg, error, "can not appendF memfile");
				memfile_free(buffer);
				return FAIL;
			}
		}

		// 구성 : VALUE;;
		switch(field_info[k].type) {
			case RETURN:
			{
				int comment_length = max_comment_return_bytes;
				if (sl->field[i].comment_length > 0
					&& sl->field[i].comment_length < max_comment_return_bytes)
				  comment_length = sl->field[i].comment_length;

				// 길이가 너무 길면 좀 자른다. 한글 안다치게...
				cut_string( field_value, comment_length );

	            if(output_style == STYLE_XML) {
					if(all_highlight || sl->field[i].is_highlight) {
						highlight(field_value, &req->word_list);
					}
				    rv = memfile_appendF(buffer, "<![CDATA[%s]]>", field_value);

					if(rv < 0) {
						MSG_RECORD(&req->msg, error, "can not appendF memfile");
						memfile_free(buffer);
						return FAIL;
					}
				} else {
					if(strlen(field_value) != 0) {
						if(all_highlight || sl->field[i].is_highlight) {
							highlight(field_value, &req->word_list);
						}
						rv = memfile_appendF(buffer, "%s", field_value);

						if(rv < 0) {
							MSG_RECORD(&req->msg, error, "can not appendF memfile");
							memfile_free(buffer);
							return FAIL;
						}
					}
				}
			}
			break;
			case SUM:
			case SUM_OR_FIRST:
				{
					int exist_summary = 0;
					int m = 0;
					int comment_length = max_comment_bytes;
					char* summary = sb_calloc(sizeof(char), max_comment_bytes*2);

					if (sl->field[i].comment_length > 0
						&& sl->field[i].comment_length < max_comment_bytes)
					  comment_length = sl->field[i].comment_length;

					for(m = 0; m < doc_hits->nhits; m++) {
						if ( field_info[k].id == doc_hits->hits[m].std_hit.field ) {
							int summary_pos = 0;

                            if(field_info[k].qpp_morpid == 16) {
							    summary_pos = get_start_comment_dha(field_value, doc_hits->hits[m].std_hit.position-4);
                            } else {
							    summary_pos = get_start_comment(field_value, doc_hits->hits[m].std_hit.position-4);
                            }

//warn("field_value[%s], summary_pos[%d], position[%u]", field_value, summary_pos, doc_hits->hits[m].std_hit.position);
							strncpy(summary, field_value + summary_pos, comment_length+1);
							cut_string( summary, comment_length);
							exist_summary = 1;
							break;
						}
					}
					
					/* 본문에 단어가  없을경우 */
					if(field_info[k].type == SUM_OR_FIRST && exist_summary == 0) {
						strncpy(summary, field_value, comment_length+1);
						cut_string( summary, comment_length);
					}

					// field_value size : 1M, highlight 시 충분한 버퍼가 필요하기 때문에 다시 옮김. 
					strcpy(field_value, summary);
				    if(summary != NULL) sb_free(summary);

					if(output_style == STYLE_XML) {
						if(all_highlight || sl->field[i].is_highlight) {
							highlight(field_value, &req->word_list);
						}

						rv = memfile_appendF(buffer, "<![CDATA[%s]]>", field_value);
						if(rv < 0) {
							MSG_RECORD(&req->msg, error, "can not appendF memfile");
							memfile_free(buffer);
							return FAIL;
						}
					} else {
					    if(strlen(field_value) != 0) {
							if(all_highlight || sl->field[i].is_highlight) {
								highlight(field_value, &req->word_list);
							}

							rv = memfile_appendF(buffer, "%s", field_value);
							if(rv < 0) {
								MSG_RECORD(&req->msg, error, "can not appendF memfile");
								memfile_free(buffer);
								return FAIL;
							}
						}
					}
				}
			break;
			default:
                warn("field return type configuration is wrong");
			break;
		}

	    if(output_style == STYLE_XML) {
			rv = memfile_appendF(buffer, "</field>\n");
			if(rv < 0) {
				MSG_RECORD(&req->msg, error, "can not appendF memfile");
				memfile_free(buffer);
				return FAIL;
			}
		} else {
			rv = memfile_appendF(buffer, ";;");
			if(rv < 0) {
				MSG_RECORD(&req->msg, error, "can not appendF memfile");
				memfile_free(buffer);
				return FAIL;
			}
		}
	}

	if(output_style == STYLE_XML) {
		rv = memfile_appendF(buffer, "</fields>\n");
		if(rv < 0) {
			MSG_RECORD(&req->msg, error, "can not appendF memfile");
			memfile_free(buffer);
			return FAIL;
		}
	} else {
        ; //do nothing
	}

	memfile_setOffset(buffer, 0);
	if(memfile_getSize(buffer) > LONG_LONG_STRING_SIZE-1) {
		MSG_RECORD(&req->msg, error, "over comment size, max[%d]", LONG_LONG_STRING_SIZE);
		memfile_free(buffer);
		return FAIL;
	}

	len = memfile_getSize(buffer);
	rv = memfile_read(buffer, comment, len);
	if(rv != len) {
		MSG_RECORD(&req->msg, error, "can not appendF memfile");
		memfile_free(buffer);
		return FAIL;
	}
	comment[len] = '\0';

	if ( b_use_cdm ) sb_run_doc_free(docBody);
	else sb_run_cdmdoc_destroy(cdmdoc);

	memfile_free(buffer);
	return SUCCESS;
}


#define IS_WHITE(c)     ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))
static int	get_start_comment_dha(char *txt, int start_word_pos)
{
    int word_pos = 0;
    char* p = txt;
    int byte_pos = 0;
    int c = 0;

	if (p == NULL) return 0;
    if (start_word_pos <= 0) return 0;

    while(1) {
        if(p == NULL) break;

        c = *p;

		if(IS_WHITE(c)) {
/*
           char buf[128];
           strncpy(buf, p, 32);
           buf[32] = '\0';
           debug("word[%s]", buf);
*/
		   word_pos++;
		}

		byte_pos++;

        p++;
//info("word_pos[%d], byte_pos[%d]", word_pos, byte_pos);
        if(word_pos == start_word_pos) break;
		if(*p == '\0') return 0;
    } 

	// 뒤가 너무 짧으면.
	/*
	if( strlen(txt) - byte_pos < 150) {
		byte_pos = strlen(txt) - 150; // summary를 210만 출력한다. 150 정도로 해준다.
		if(byte_pos < 0) byte_pos = 0;
	}
	*/

    return byte_pos;
}

static int	get_start_comment(char *pszStr, int lPosition)
{
	
	int iStr = 0, lPos=0, nCheck=0;
	
	if (pszStr == NULL || lPosition == 0) return 0;

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

static void reduce_dochits_to_one_per_doc(index_list_t *list)
{
	uint32_t i=0,idx=0, j=0;
	uint32_t current_docid=0;

	idx=0;
	for (i=0; i<list->ndochits; i++) {
		if (list->doc_hits[i].id != current_docid) {
			list->doc_hits[idx] = list->doc_hits[i];
			list->relevance[idx] = list->relevance[i];
			idx++;

			current_docid = list->doc_hits[i].id;
			
		}
		else
		{
			list->relevance[idx-1] += list->relevance[i];
		}
	}

	for (j=0; j < idx; j++)
	{
		list->doc_hits[j].hitratio = list->relevance[j];
	}
	
	list->ndochits = idx;
}

static int init_response(response_t* res)
{
	if (res == NULL) {
		error("response_t* res == NULL");
		return FAIL;
	}

    memset(res, 0x00, sizeof(response_t));
	res->vdl = g_vdl;

    return SUCCESS;
}


static int get_clause_type(char* clause)
{
    int i = 0;

    for(i = 0; i < MAX_CLAUSE_TYPE; i++) {
        if(strncasecmp(clause, clause_type_str[i], 
                    strlen(clause_type_str[i])) == 0) {
            return i;
        }
    } 

    return UNKNOWN;
}

static char* get_clause_str(int clause_type)
{
    if(clause_type < UNKNOWN || clause_type >= MAX_CLAUSE_TYPE)
        return NULL;

    return clause_type_str[clause_type];
}

static void set_select_clause(select_list_t* sl, char* clause)
{
    char* s = NULL;
    char* e = NULL;

    s = sb_trim(clause);

    if(s == NULL || strlen(s) == 0) return;

    strncpy(sl->clause, s, sizeof(sl->clause)-1);

    while(1) {
		e = strchr(s, ',');
        if(e == NULL && strlen(s) == 0) break;

		if(e == NULL) {
			// 마지막 clause도 처리하기 위해.
		} else {
            *e = '\0';
		}

		if(strncmp(s, "*", 1) == 0) {
			if(sl->cnt >= MAX_EXT_FIELD) {
				warn("select list is full, can not add field[%s]", clause);
				break;
			} 

            sl->field[0].name[0] = '*';
            sl->cnt = 1;

			if(strncasecmp(s+1, ":H", 2) == 0) {
                sl->field[0].is_highlight = 1;
			}

			/* 다른건 확인할 필요가 없다. */
			break;
		} else {
			int has_comment_length = 0;
			char* name = sb_trim(s);
			char* highlight = NULL;
			char* left_paren = NULL;
			char* right_paren = NULL;

			/* name에서 :H 의 존재유무만 확인한다. The strcasestr() function is a non-standard extension. */
			if( ( highlight = strstr(name, ":H") ) == NULL) {
			    highlight = strstr(name, ":h");
			}

			/* FieldName(100) 형식의 코멘트 길이값을 입력받는다. */
			left_paren = strchr(name, '(');
			right_paren = strchr(name, ')');
			if (left_paren != NULL && right_paren != NULL && left_paren < right_paren)
				has_comment_length = 1;

			if ( highlight != NULL )
			{ /* FieldName:H or FieldName:H(200) */
				*highlight = '\0';
				sl->field[sl->cnt].is_highlight = 1;

			}
			if ( has_comment_length == 1 )
			{ /* FieldName(200) or FieldName:H(200) */
				*left_paren = '\0';
				*right_paren = '\0';
				sl->field[sl->cnt].comment_length = atoi(left_paren+1);
			}
            strncpy(sl->field[sl->cnt++].name, name, SHORT_STRING_SIZE);

        }

		if(e == NULL) break;
		if(sl->cnt >= MAX_EXT_FIELD) {
			warn("over max select field[%d]", MAX_EXT_FIELD);
			break;
		}

        s = e+1;
    }
} 

static void print_weight(weight_list_t* wl)
{
    int i = 0;

    for(i = 0; i < wl->cnt; i++) {
        debug("weight field[%s], value[%d], field_id[%d]", 
               wl->list[i].name, wl->list[i].weight, wl->list[i].field_id);
    } 
}

static void set_weight(weight_list_t* wl, char* clause)
{
    char* s = NULL;
    char* e = NULL;
    char* p = NULL;

    s = sb_trim(clause);

    if(s == NULL || strlen(s) == 0) return;

    strncpy(wl->clause, s, sizeof(wl->clause)-1);

    while(1) {
		e = strchr(s, ',');
        if(e == NULL && strlen(s) == 0) break;

		if(e == NULL) {
			// 마지막 clause도 처리하기 위해.
		} else {
            *e = '\0';
		}

		p = strchr(s, ':');
        if(p == NULL) {
            continue;
        } else {
            int idx = 0;
            char* name = NULL;
            *p = '\0';

            name = sb_trim(s);
            if( strcasecmp("RESET", name) == 0) {
				wl->reset = 1;
				wl->reset_value = atoi(p+1);
            } else {
				strncpy(wl->list[wl->cnt].name, name, SHORT_STRING_SIZE); 
				wl->list[wl->cnt].weight = atoi(p+1);

				if( is_exist_return_field(wl->list[wl->cnt].name, &idx) != FALSE) {
					wl->list[wl->cnt].field_id = field_info[idx].id;
					wl->cnt++;
				}
            }
        }

		if(e == NULL) break;
		if(wl->cnt >= MAX_WEIGHT) {
			warn("over max weight field[%d]", MAX_WEIGHT);
			break;
		}

        s = e+1;
    }
} 

static void set_output_style(request_t* req, char* clause)
{
    char* s = NULL;

    s = sb_trim(clause);

	if(s == NULL || strncasecmp(s, "XML", 3) == 0) {
		req->output_style = STYLE_XML;
	} else if(strncasecmp(s, "SOFTBOT4", 8) == 0) {
		req->output_style = STYLE_SOFTBOT4;
	}
}

static void set_virtual_id(virtual_rule_list_t* vrl, char* clause)
{
    char* s = NULL;
    char* e = NULL;
	key_rule_t* rule = NULL;

    s = sb_trim(clause);

	if(s == NULL || strlen(s) == 0) {
		rule = &vrl->list[vrl->cnt++];

		rule->type = DID;
		strncpy(rule->name, "DID", SHORT_STRING_SIZE-1);
		info("virtual_id field is null, set default DID");
	} else {
        strncpy(vrl->clause, s, sizeof(vrl->clause)-1);

		while(1) {
			e = strchr(s, ',');
			if(e == NULL && strlen(s) == 0) break;

			if(e == NULL) {
				// 마지막 clause도 처리하기 위해.
			} else {
				*e = '\0';
			}

			/*
			 * virtual_document는 did, docattr 만이 key가 될수 있다.
			 */
			rule = &vrl->list[vrl->cnt++];
            s = sb_trim(s);

			if(strncasecmp(s, "DID", 3) == 0) {
				rule->type = DID;
				strncpy(rule->name, "DID", SHORT_STRING_SIZE-1);
			} else {
				rule->type = DOCATTR;
				strncpy(rule->name, s, SHORT_STRING_SIZE-1);
			}

			if(e == NULL) break;
			if(vrl->cnt >= MAX_VID_RULE) {
				warn("over max vid rule[%d]", MAX_VID_RULE);
				break;
			}

			s = e+1;
		}
	}
}

static int add_operation(operation_list_t* op_list, char* clause, int clause_type)
{
	if(op_list->cnt >= MAX_OPERATION) {
        error("cant not add operation. operation overflow. max[%d]", MAX_OPERATION);
		return FAIL;
	}

    switch(clause_type) {
        case WHERE:
            strncpy(op_list->list[op_list->cnt].rule.where, clause, LONG_STRING_SIZE-1);
            break;
        case GROUP_BY:
        case COUNT_BY:
            make_groupby_rule_list(clause, &(op_list->list[op_list->cnt].rule.groupby));
            break;
        case ORDER_BY:
            make_orderby_rule_list(clause, &op_list->list[op_list->cnt].rule.orderby); 
            break;
        case LIMIT:
            make_limit_rule(clause, &op_list->list[op_list->cnt].rule.limit); 
            break;
        default:
            warn("it is not operator : [%d][%s] - [%s]", clause_type, 
                                       get_clause_str(clause_type), clause);
            return FAIL;
    }

    op_list->cnt++;
    
    return SUCCESS;
}

static void print_operations(operation_list_t* op_list)
{
	int i = 0;
	int j = 0;

	for(j = 0; j < op_list->cnt; j++) {
		operation_t* op = &op_list->list[j]; 

		switch(op->type) {
			case WHERE:
				debug("where[%s]", op->clause);
				break;
			case COUNT_BY:
			case GROUP_BY:
			   {
				   debug("group rule count[%d]", op->rule.groupby.cnt);
				   for(i = 0; i < op->rule.groupby.cnt; i++) {
					   print_groupby(&op->rule.groupby.list[i]);
				   }
			   }
			   break;
			case ORDER_BY:
			   {
				   debug("orderby rule count[%d]", op->rule.orderby.cnt);
				   for(i = 0; i < op->rule.orderby.cnt; i++) {
					   print_orderby(&op->rule.orderby.list[i]);
				   }
			   }
			   break;
			case LIMIT:
			   print_limit(&op->rule.limit);
			   break;
			default:
               warn("it is not operator : [%d][%s]", op->type, 
                                       get_clause_str(op->type));
			   break;
		}
	}
}

static void print_search_word(word_list_t* wl)
{
    int i = 0;
    for(i = 0; i < wl->cnt; i++) {
        debug("highlight word[%s]", wl->word[i]);
    }
}

static void print_request(request_t* req)
{
	int i  = 0;

    debug("req->query[%s]", req->query);
    debug("req->search[%s]", req->search);

	print_search_word(&req->word_list);
	print_select(&req->select_list);
	print_weight(&req->weight_list);
	debug("req->op_list_did.cnt[%d]", req->op_list_did.cnt);
	print_operations(&req->op_list_did);
	debug("req->op_list_vid.cnt[%d]", req->op_list_vid.cnt);
	print_operations(&req->op_list_vid);

	debug("req->virtual_rule_list.cnt[%d]", req->virtual_rule_list.cnt);
	for(i = 0; i < req->virtual_rule_list.cnt; i++) {
		key_rule_t* rule = &req->virtual_rule_list.list[i];

	    debug("virtual key[%s], type[%s]", rule->name, key_type_str[rule->type]);
	}

	debug("req->output_style[%s]", output_style_str[req->output_style]);
}

static void add_delete_where(operation_list_t* op_list)
{
	int i = 0;
    int has_where = 0;

	for(i = 0; i < op_list->cnt; i++) {
		operation_t* op = &op_list->list[i]; 

		if(op->type == WHERE) {
            has_where = 1;
        }
    }

    if(has_where == 0 && op_list->cnt < MAX_OPERATION) {
        operation_list_t tmp_op_list = *op_list;

        memcpy(&op_list->list[1], &tmp_op_list.list[0], sizeof(operation_t)*op_list->cnt);

        op_list->list[0].type = WHERE;
        strcpy(op_list->list[0].clause, "WHERE delete=0");
        strcpy(op_list->list[0].rule.where, "delete=0");
        op_list->cnt++;
    }

    return;
}

/* body:대검 -> 대검 */
static char* remove_field(char* word)
{
    char* p = strchr(word, ':');

	if(p == NULL) {
		return word;
	} else {
	    return (p+1);
	}
}

static void set_search_words_as_parsed(request_t* req, response_t *res)
{
    int i = 0;
    int j = 0;
    char* s = NULL; /* start: 남은 질의식 문자열의 시작위치 */
    char* e = NULL; /* end:   이번 어절 끝위치 */
	/* q: SEARCH 절의 검색질의식. 예) Field1:단어 & Field2:단어 */
    char* q = sb_calloc(sizeof(char), strlen(res->parsed_query)+1);
	int len = 0;
	int remove_char_len = strlen(remove_char_query);
    word_list_t* wl = &req->word_list;

	debug("res->parsed_query[%s]", res->parsed_query);
    strncpy(q, res->parsed_query, strlen(res->parsed_query));
    len = strlen(q);

	for(i = 0; i < len; i++) {
	    for(j = 0; j < remove_char_len; j++) {
			if ( q[i] == remove_char_query[j]) {
				 q[i] = ' ';
				 break;
			}
		}
	}

    s = sb_trim(q);

	while(1) {
		e = strchr(s, ' ');
		if(e == NULL && strlen(s) == 0) break;

		if(e == NULL) {
			// 마지막 clause도 처리하기 위해.
		} else {
			*e = '\0';
		}

		s = sb_trim(remove_field(s));
		if ( strcmp(s, "&") == 0 ) {
			; /* 어절이 "&" 인 경우는 제외한다. */
		} else 
		if(strlen(s) > highlight_word_len) {
            strncpy(wl->word[wl->cnt++], s, MAX_WORD_LEN-1);
	    }

		if(e == NULL) break;
		if(wl->cnt >= MAX_QUERY_NODES) {
			warn("over max word count[%d]", MAX_QUERY_NODES);
			break;
		}
	
		s = e+1;
	}

    sb_free(q);
}

static void set_search_words_as_input(request_t* req, response_t *res)
{
    int i = 0;
    int j = 0;
    char* s = NULL; /* start: 남은 질의식 문자열의 시작위치 */
    char* e = NULL; /* end:   이번 어절 끝위치 */
	/* q: SEARCH 절의 검색질의식. 예) Field1:단어 & Field2:단어 */
    char* q = sb_calloc(sizeof(char), strlen(req->search)+1);
	int len = 0;
	int remove_char_len = strlen(remove_char_query);
    word_list_t* wl = &req->word_list;

	debug("req->search[%s]", req->search);
    strncpy(q, req->search, strlen(req->search));
    len = strlen(q);

	for(i = 0; i < len; i++) {
	    for(j = 0; j < remove_char_len; j++) {
			if ( q[i] == remove_char_query[j]) {
				 q[i] = ' ';
				 break;
			}
		}
	}

    s = sb_trim(q);

	while(1) {
		e = strchr(s, ' ');
		if(e == NULL && strlen(s) == 0) break;

		if(e == NULL) {
			// 마지막 clause도 처리하기 위해.
		} else {
			*e = '\0';
		}

		s = sb_trim(remove_field(s));
		if(strlen(s) > highlight_word_len) {
            strncpy(wl->word[wl->cnt++], s, MAX_WORD_LEN-1);
	    }

		if(e == NULL) break;
		if(wl->cnt >= MAX_QUERY_NODES) {
			warn("over max word count[%d]", MAX_QUERY_NODES);
			break;
		}
	
		s = e+1;
	}

    sb_free(q);
}

static void set_search_words(request_t* req, response_t *res)
{
	if (highlight_word == H_WORD_PARSED)
		set_search_words_as_parsed(req, res);
	else
		set_search_words_as_input(req, res);
}

static int init_request(request_t* req, char* query)
{
    char* s = NULL;
    char* e = NULL;
	int did_rule_status = 0;

    memset(req, 0x00, sizeof(request_t));
	if(query == NULL) {
        MSG_RECORD(&req->msg, error, "no query string");
        return FAIL;
	}

    g_request = req;
	g_vdl->cnt = 0;

    s = sb_trim(query);
	strncpy(req->query, s, MAX_QUERY_STRING_SIZE);

    while(1) {
        int clause_type = 0, len = 0;

        s = sb_trim(s);  // '\r'을 찾았을 경우 '\n'을 삭제하기 위해

		e = strchr(s, '\r');  // window 형식의 newline을 찾아보고 없으면 unix 형색의 newline을 찾는다.
        if(e == NULL) e = strchr(s, '\n');

        if(e == NULL && strlen(s) == 0) break;

		if(e == NULL) {
			// 마지막 clause도 처리하기 위해.
		} else {
            *e = '\0';
		}

		// 공백라인 skip
		if(s != NULL) {
		    if(strlen(sb_trim(s)) == 0) {
				if(e == NULL) break;
				s = e + 1;

				debug("blank line");
				continue;
			}
		}

        clause_type = get_clause_type(s);

        if(clause_type == UNKNOWN) {
            MSG_RECORD(&req->msg, warn, "clause started with unknown symbol[%s]", s);
			return FAIL;
		} else {
			len = strlen(clause_type_str[clause_type]);
			
			switch(clause_type) {
				case SELECT:
					set_select_clause(&req->select_list, s+len);
					break;
				case SEARCH:
					strncpy(req->search, sb_trim(s+len), MAX_QUERY_STRING_SIZE-1);
					break;
				case VIRTUAL_ID:
					set_virtual_id(&req->virtual_rule_list, s+len);
					break;
                case WEIGHT:
                    set_weight(&req->weight_list, s+len);
                    break;
				case WHERE:
				case GROUP_BY:
				case COUNT_BY:
				case ORDER_BY:
				case LIMIT:
					{
                        operation_list_t* op_list = NULL;
						if(did_rule_status) {
							op_list = &req->op_list_did;
						} else {
							op_list = &req->op_list_vid;
						}

					    strncpy(op_list->list[op_list->cnt].clause, s, LONG_STRING_SIZE);
                        op_list->list[op_list->cnt].type = clause_type;

						if(add_operation(op_list, s+len, clause_type) != SUCCESS) {
                            MSG_RECORD(&req->msg, warn, "clause started by unknown symbol[%s]", s);
							return FAIL;
						}
						break;
					}
				case START_DID_RULE:
					did_rule_status = 1;
					break;
				case END_DID_RULE:
					did_rule_status = 0;
					break;
				case COMMENT:
					debug("comment[%s]", s);
					break;
				case OUTPUT_STYLE:
					debug("out_style[%s]", s);
					set_output_style(req, s+len);
					break;
				default:
                    MSG_RECORD(&req->msg, warn, "clause started by unknown symbol[%s]", s);
					return FAIL;
			}
		}

		if(e == NULL) break;
		s = e + 1;
    }

    /* virtual id가 없을경우 default로 did로 셋팅하도록 */
	if(req->virtual_rule_list.cnt == 0) { 	 
        set_virtual_id(&req->virtual_rule_list, NULL); 	 
	}

    add_delete_where(&req->op_list_did);
    /* did operation만 하면 된다 */
    //add_delete_where(&req->op_list_vid);

	print_request(req);

    return SUCCESS;
}

static int get_query_string(request_t* req, char query[MAX_QUERY_STRING_SIZE])
{
	int rv = 0;
	int i = 0;
	memfile *buffer = memfile_new();
    operation_list_t* op_list = NULL;
	select_list_t* sl = &req->select_list;
	weight_list_t* wl = &req->weight_list;
	virtual_rule_list_t* vrl = &req->virtual_rule_list;

	memset(query, 0x00, MAX_QUERY_STRING_SIZE);

	if(sl->cnt > 0) {
		// SELECT
		rv = memfile_appendF(buffer, "%s %s\n", clause_type_str[SELECT], sl->clause);
		if(rv < 0) {
			MSG_RECORD(&req->msg, error, "can not appendF memfile");
			memfile_free(buffer);
			return FAIL;
		}
    } else {
		MSG_RECORD(&req->msg, error, "can not find select clause");
		memfile_free(buffer);
        return FAIL;
    }

	if(wl->cnt > 0) {
		// WEIGHT
		rv = memfile_appendF(buffer, "%s %s\n", clause_type_str[WEIGHT], wl->clause);
		if(rv < 0) {
			MSG_RECORD(&req->msg, error, "can not appendF memfile");
			memfile_free(buffer);
			return FAIL;
		}
   }

	// SEARCH
    if(strlen(req->search) == 0) {
		MSG_RECORD(&req->msg, error, "can not find search word");
		memfile_free(buffer);
        return FAIL;
    } else {
		rv = memfile_appendF(buffer, "%s %s\n", clause_type_str[SEARCH], req->search);
		if(rv < 0) {
			MSG_RECORD(&req->msg, error, "can not appendF memfile");
			memfile_free(buffer);
			return FAIL;
		}
    }

	if(vrl->cnt > 0) {
		rv = memfile_appendF(buffer, "%s %s\n", clause_type_str[VIRTUAL_ID], vrl->clause);
		if(rv < 0) {
			MSG_RECORD(&req->msg, error, "can not appendF memfile");
			memfile_free(buffer);
			return FAIL;
		}

		// VIRTUAL_ID RULE
		if(req->op_list_did.cnt > 0) {
			op_list = &req->op_list_did;

			rv = memfile_append(buffer, "(\n", 2);
			if(rv < 0) {
				MSG_RECORD(&req->msg, error, "cannot appendF memfile");
				memfile_free(buffer);
				return FAIL;
			}

			for(i = 0; i < op_list->cnt; i++) {
				operation_t* op = &(op_list->list[i]);
				rv = memfile_appendF(buffer, "%s\n", op->clause);
				if(rv < 0) {
					MSG_RECORD(&req->msg, error, "cannot appendF memfile");
					memfile_free(buffer);
					return FAIL;
				}
			}

			rv = memfile_append(buffer, ")\n", 2);
			if(rv < 0) {
				MSG_RECORD(&req->msg, error, "cannot appendF memfile");
				memfile_free(buffer);
				return FAIL;
			}
		}
    }
	
	op_list = &req->op_list_vid;
	for(i = 0; i < op_list->cnt; i++) {
		operation_t* op = &(op_list->list[i]);
		rv = memfile_appendF(buffer, "%s\n", op->clause);
		if(rv < 0) {
            MSG_RECORD(&req->msg, error, "cannot appendF memfile");
		    memfile_free(buffer);
			return FAIL;
		}
	}

	if(MAX_QUERY_STRING_SIZE < memfile_getSize(buffer)) {
        MSG_RECORD(&req->msg, error, "enough buffer, buffer size[%d], query size[%ld]",
				MAX_QUERY_STRING_SIZE, memfile_getSize(buffer));
		memfile_free(buffer);
		return FAIL;
	}

	memfile_setOffset(buffer, 0);
	rv = memfile_read(buffer, query, memfile_getSize(buffer));
	if(rv != memfile_getSize(buffer)) {
        MSG_RECORD(&req->msg, error, "can not appendF memfile");
		memfile_free(buffer);
		return FAIL;
	}

	debug("query[\n%s]", query);

	memfile_free(buffer);
	return SUCCESS;
}

static int cb_index_list_sort(const void* dest, const void* sour, void* userdata)
{
    int diff = 0;
    key_rule_t *rule;
    doc_hit_t *dest_dh;
    doc_hit_t *sour_dh;

    rule = (key_rule_t*)userdata;

    if ( rule == NULL ) return 0;

    dest_dh = (doc_hit_t*)dest;
    sour_dh = (doc_hit_t*)sour;
        
	if ( rule->type == DOCATTR ) {
		docattr_t *attr1, *attr2;
		int value1, value2;

		attr1 = g_docattr_base_ptr + 
				g_docattr_record_size*(dest_dh->id - 1);
		attr2 = g_docattr_base_ptr + 
				g_docattr_record_size*(sour_dh->id - 1);

		if (sb_run_docattr_get_field_integer_function(attr1, rule->name, &value1) == -1) {
			error("cannot get value of [%s] field", rule->name);
			return FAIL;
		}

		if (sb_run_docattr_get_field_integer_function(attr2, rule->name, &value2) == -1) {
			error("cannot get value of [%s] field", rule->name);
			return FAIL;
		}

		diff = value1 - value2;
        if(diff == 0) { // 같을 경우 default로 관련성으로 정렬함.
            diff = dest_dh->hitratio - sour_dh->hitratio;
        }
	} else { // virtual_document의 key는 RELEVANCY가 될수 없다.
		diff = dest_dh->id - sour_dh->id;
        if(diff == 0) { // 같을 경우 default로 관련성으로 정렬함.
            diff = dest_dh->hitratio - sour_dh->hitratio;
        }
	}

	if ( diff != 0 ) {
		diff = diff > 0 ? 1 : -1;
		return diff;
	}

    return 0;
}

static int make_virtual_document(index_list_t* list, request_t* req)
{
    uint32_t i=0, j = 0;
	int virtual_id_list[MAX_VID_RULE];
	int next_virtual_id_list[MAX_VID_RULE];

    for (i=0; i<list->ndochits;) {
        virtual_document_t* vd = &(g_vdl->data[g_vdl->cnt]);

		/* virtual_document를 초기화 하고 만들어야 한다 전체 초기화를 피하기위해 만들때 한다. */
		memset(vd, 0x00, sizeof(virtual_document_t));

		vd->docattr = g_docattr_base_ptr + 
					  g_docattr_record_size*(list->doc_hits[i].id - 1); // did는 1 base임.
		vd->relevance += list->doc_hits[i].hitratio;
		vd->dochits = &list->doc_hits[i];
		vd->dochit_cnt++;
		vd->comment_cnt++;

		for(j = 0; j < req->virtual_rule_list.cnt; j++) {
			key_rule_t* rule = &req->virtual_rule_list.list[j];

			if(rule->type == DOCATTR) {
				if (sb_run_docattr_get_field_integer_function(vd->docattr, 
												 rule->name, &virtual_id_list[j]) == -1) {
					error("cannot get value of [%s] field", rule->name);
					return FAIL;
				}
			} else { // virtual_document의 key는 RELEVANCY가 될수 없다.
				virtual_id_list[j] = list->doc_hits[i].id;
			}
		}
		// 첫번째 기준 필드를 vid로 가진다.
		vd->id = virtual_id_list[0];

        for (i++; i < list->ndochits; i++) {
			int equals_vid = 1;

			docattr_t* docattr = g_docattr_base_ptr + 
				   		       g_docattr_record_size*(list->doc_hits[i].id - 1); // did는 1 base임.

		    for(j = 0; j < req->virtual_rule_list.cnt; j++) {
			    key_rule_t* rule = &req->virtual_rule_list.list[j];

				if(rule->type == DOCATTR) {
					if (sb_run_docattr_get_field_integer_function(docattr, 
													 rule->name, &next_virtual_id_list[j]) == -1) {
						error("cannot get value of [%s] field", rule->name);
						return FAIL;
					}
				} else { // virtual_document의 key는 RELEVANCY가 될수 없다.
					next_virtual_id_list[j] = list->doc_hits[i].id;
				}
			}

		    for(j = 0; j < req->virtual_rule_list.cnt; j++) {
				if(virtual_id_list[j] != next_virtual_id_list[j]) {
					equals_vid = 0;
					break;
				}
			}

		    if(equals_vid == 1) {
				vd->relevance += list->doc_hits[i].hitratio;
				vd->dochit_cnt++;
		        vd->comment_cnt++;
            } else {
                break;
            }
        }

        g_vdl->cnt++;
    }

    return SUCCESS;
}

/*
 * clause sample : cate1 LIMIT 1,   10; cate2 ;cate3 DESC LIMIT 10;
 * gr : [cate1:ASC:1:10]
 * gr : [cate2:ASC:-1:-1]
 * gr : [cate1:DESC:0:10]
 */
static int make_groupby_rule(char* clause, groupby_rule_t* rule)
{
    char* token = NULL;
    int field = 0;

	rule->is_enum = 1;              // default : enum filed임
    rule->sort.rule.type = DOCATTR; // default
    rule->sort.type = ASC;          // default
    rule->limit.start = -1;         // default : 제한없음.
    rule->limit.cnt = -1;           // default : 제한없음.

    token = strtok(clause, " ");
    while(token != NULL) {
        token = sb_trim(token);

        if(!field) {
            field = 1;
            strncpy(rule->sort.rule.name, token, SHORT_STRING_SIZE);

			if(strncasecmp(token, 
						   key_type_str[RELEVANCY], 
						   strlen(key_type_str[RELEVANCY])) == 0) {
				rule->sort.rule.type = RELEVANCY;
			} else if(strncasecmp(token, 
						   key_type_str[DID], 
						   strlen(key_type_str[DID])) == 0) {
				rule->sort.rule.type = DID;
			} else {
				rule->sort.rule.type = DOCATTR;
			}
        } else {
			if(strncasecmp(token, "INT", 3) == 0) {
                rule->is_enum = 0;
			} else if(strncasecmp(token, "ASC", 3) == 0) {
                rule->sort.type = ASC;
            } else if(strncasecmp(token, "DESC", 4) == 0) {
                rule->sort.type = DESC;
            } else if(strncasecmp(token, "LIMIT", 4) == 0) {
                char* split = NULL;
                token = strtok(NULL, ""); // LIMIT 이후로는 전부 limit value로 인식.
                token = sb_trim(token);

                // LIMIT 1, 10 : start, count 분리
                split = strchr(token, ',');
                if(split == NULL) {
                    rule->limit.start = 0;
                    rule->limit.cnt = atoi(token);
                } else {
                    *split = '\0';

                    rule->limit.start = atoi(token);
                    rule->limit.cnt = atoi(split+1);
                }
            } else {
				warn("unknown symbol[%s]", token);
			}
        }

        token = strtok(NULL, " ");
    }

    return SUCCESS;
}

static int make_groupby_rule_list(char* clause, groupby_rule_list_t* grl)
{
    char* s = NULL;
    char* e = NULL;

    s = sb_trim(clause);

    if(s == NULL || strlen(s) == 0) return SUCCESS;

    strncpy(grl->clause, s, sizeof(grl->clause)-1);

    while(1) {
		if(grl->cnt > MAX_GROUP_RULE) {
			warn("over gropuby rule max count[%d]", MAX_GROUP_RULE);
			break;
		}

        e = strchr(s, ';');
        if(e == NULL && strlen(s) == 0) break;

		if(e == NULL) {
			// 마지막 clause도 처리하기 위해.
		} else {
            *e = '\0';
		}
        make_groupby_rule(s, &grl->list[grl->cnt]);
		grl->cnt++;

		if(e == NULL) break;
        s = e+1;
    }

    return SUCCESS;
}

/*
 * clause sample : 1, 10
 * limit : [1:10]
 */
static int make_limit_rule(char* clause, limit_t* rule)
{
    char* split = NULL;
    clause = sb_trim(clause);

    if(clause == NULL || strlen(clause) == 0) return SUCCESS;

    strncpy(rule->clause, clause, sizeof(rule->clause)-1);

    // LIMIT 1, 10 : start, count 분리
    split = strchr(clause, ',');
    if(split == NULL) {
        rule->start = 0;
        rule->cnt = atoi(clause);
    } else {
        *split = '\0';

        rule->start = atoi(clause);
        rule->cnt = atoi(split+1);
    }

	return SUCCESS;
}

/*
 * clause sample :	  title	asc  , 	 date 	  desc     , gubun ,,
 * si : [title:ASC] [date:DESC] [gubun:ASC]
 */
static int make_orderby_rule_list(char* clause, orderby_rule_list_t* orl)
{
    char* sort_field = NULL;
	int cnt = 0;

	if (clause == NULL) {
		warn("no sorting condition");
		return SUCCESS;
	}

    clause = sb_trim(clause);
    if(clause == NULL || strlen(clause) == 0) return SUCCESS;

    strncpy(orl->clause, clause, sizeof(orl->clause)-1);

    sort_field = strtok(clause, ",");
    while(sort_field != NULL) {
        char* s = sb_trim(sort_field);
        char* sort_type = NULL;
        char* p = NULL;

		if(orl->cnt > MAX_SORT_RULE) {
            warn("over orderby rule max count[%d]", MAX_SORT_RULE);
			break;
		}

	    cnt = orl->cnt;
        
        if(strlen(s) != 0) {
            p = strchr(s, ' ');

            if(p == NULL) {
                strncpy(orl->list[cnt].rule.name, sb_trim(s), SHORT_STRING_SIZE);

				if(strncasecmp(s, 
						       key_type_str[RELEVANCY], 
							   strlen(key_type_str[RELEVANCY])) == 0) {
                    orl->list[cnt].rule.type = RELEVANCY;
				} else if(strncasecmp(s, 
						       key_type_str[DID], 
							   strlen(key_type_str[DID])) == 0) {
                    orl->list[cnt].rule.type = DID;
				} else {
                    orl->list[cnt].rule.type = DOCATTR;
				}

                 orl->list[cnt].type = ASC;
            } else { 
                *p = '\0';
                sort_type = sb_trim(p+1);

                strncpy(orl->list[cnt].rule.name, sb_trim(s), SHORT_STRING_SIZE);
				if(strncasecmp(s, 
						       key_type_str[RELEVANCY], 
							   strlen(key_type_str[RELEVANCY])) == 0) {
                    orl->list[cnt].rule.type = RELEVANCY;
				} else if(strncasecmp(s, 
						       key_type_str[DID], 
							   strlen(key_type_str[DID])) == 0) {
                    orl->list[cnt].rule.type = DID;
				} else {
                    orl->list[cnt].rule.type = DOCATTR;
				}

                if(strncasecmp(sort_type, "DESC", 4) == 0) {
                    orl->list[cnt].type = DESC;
                } else {
                    orl->list[cnt].type = ASC;
                }
            }
        }

        orl->cnt++;
        sort_field = strtok(NULL, ",");
    }

    return SUCCESS;
}

static int virtual_document_orderby(orderby_rule_list_t* rules)
{
	user_data_t userdata;
	userdata.doc_type = VIRTUAL_DOCUMENT;
	userdata.rules = rules;
	userdata.docattr_base_ptr = g_docattr_base_ptr;
	userdata.docattr_record_size = g_docattr_record_size;

    qsort2(g_vdl->data, g_vdl->cnt, sizeof(virtual_document_t), &userdata, 
          sb_run_qp_cb_orderby_virtual_document);

	return SUCCESS;
}

static int document_orderby(orderby_rule_list_t* rules)
{
	int i = 0;

	user_data_t userdata;
	userdata.doc_type = DOCUMENT;
	userdata.rules = rules;
	userdata.docattr_base_ptr = g_docattr_base_ptr;
	userdata.docattr_record_size = g_docattr_record_size;

	for(i = 0; i < rules->cnt; i++) {
	    print_orderby(&rules->list[i]);
	}

    qsort2(g_result_list->doc_hits, g_result_list->ndochits, sizeof(doc_hit_t), &userdata, 
          sb_run_qp_cb_orderby_document);

	return SUCCESS;
}

static int document_where()
{
    int i = 0;
    int save_pos = 0;
    int rv = 0;
	docattr_t* docattr = NULL;

	for (i = 0; i < g_result_list->ndochits ; i++) {
        doc_hit_t* d = (doc_hit_t*)&g_result_list->doc_hits[i];
		docattr = g_docattr_base_ptr + g_docattr_record_size*(d->id-1);

		rv = sb_run_qp_cb_where(docattr);
		if ( rv == MINUS_DECLINE ) {
			warn("callback function return [%d]", rv);
			return TRUE;
	    } else if ( rv ) {
			g_result_list->doc_hits[save_pos] = g_result_list->doc_hits[i];
			save_pos++;
		}
	}

	g_result_list->ndochits = save_pos;

    return SUCCESS;
}

static int virtual_document_where()
{
    int i = 0;
    int save_pos = 0;
    int rv = 0;

	for (i = 0; i < g_vdl->cnt ; i++) {
		rv = sb_run_qp_cb_where(g_vdl->data[i].docattr);

		if ( rv == MINUS_DECLINE ) {
			warn("callback function return [%d]", rv);
			return TRUE;
	    } else if ( rv ) {
			g_vdl->data[save_pos] = g_vdl->data[i];
			save_pos++;
		}
	}

	g_vdl->cnt = save_pos;

    return SUCCESS;
}

static int virtual_document_group_count(virtual_document_t* vd, groupby_result_list_t* result, int* is_remove)
{
	int j = 0;
	uint32_t value = 0;
	key_rule_t* rule = NULL;
	limit_t* limit = NULL;
    groupby_rule_list_t* rules = NULL;

	rules = &result->rules;

	for(j = 0; j < rules->cnt; j++) {
		rule = &rules->list[j].sort.rule;
		limit = &rules->list[j].limit;

		if (sb_run_docattr_get_field_integer_function(vd->docattr, rule->name, &value) == -1) {
			error("cannot get value of [%s] field", rule->name);
			return FAIL;
		}

		if(value > MAX_CARDINALITY) {
			warn("group(%s) enum[%d] should be smaller than MAX_CARDINALITY[%d], count skip",
				 rule->name, value, MAX_CARDINALITY);
			*is_remove = (*is_remove == 0) ? 1 : *is_remove;
			break;
		}

		if(limit->start == -1 || limit->cnt == -1) {
			*is_remove = (*is_remove == 0) ? 0 : *is_remove;
		} else if(result->result[j][value] < limit->start ||
				result->result[j][value] > (limit->start + limit->cnt -1)) {
			*is_remove = (*is_remove == 0) ? 1 : *is_remove;
		} else {
			*is_remove = (*is_remove == 0) ? 0 : *is_remove;
		}

		result->result[j][value]++;
	}

	return SUCCESS;
}

static int virtual_document_grouping(groupby_result_list_t* result)
{
	int i = 0;
	int save_pos = 0;
	int is_remove = 0;
	virtual_document_t* vd = NULL;

    if(result->rules.cnt == 0) 
		return SUCCESS; 

	for(i = 0; i < g_vdl->cnt; i++) {
		vd = &(g_vdl->data[i]);
		is_remove = 0;

		if(virtual_document_group_count(vd, result, &is_remove) != SUCCESS) {
			error("can not count group");
			return FAIL;
		}

		if(is_remove == 0) {
			g_vdl->data[save_pos] = g_vdl->data[i];
			save_pos++;
		}
	}

	g_vdl->cnt = save_pos;

	return SUCCESS;
}

static int document_get_group_values(doc_hit_t* doc_hit, int group_values[MAX_GROUP_RULE], groupby_rule_list_t* rules)
{
	int j = 0;
	key_rule_t* rule = NULL;
	docattr_t* docattr = NULL;

	for(j = 0; j < rules->cnt; j++) {
		rule = &rules->list[j].sort.rule;
		docattr = g_docattr_base_ptr + g_docattr_record_size*(doc_hit->id-1);

		if (sb_run_docattr_get_field_integer_function(docattr, rule->name, &group_values[j]) == -1) {
			error("cannot get value of [%s] field", rule->name);
			return FAIL;
		}
	}

	return SUCCESS;
}

static int document_grouping(request_t* req, groupby_result_list_t* result)
{
	int i = 0;
	int j = 0;
	int save_pos = 0;
	doc_hit_t* doc_hit = NULL;
    int is_remove = 0;
	docattr_t* docattr = NULL;

	int curr_group_values[MAX_GROUP_RULE];
	int next_group_values[MAX_GROUP_RULE];
	int group_count[MAX_GROUP_RULE];

	int virtual_id_list[MAX_VID_RULE];
	int next_virtual_id_list[MAX_VID_RULE];

	groupby_rule_list_t* rules = &result->rules;

    if(rules->cnt == 0) 
		return SUCCESS; 

	for(i = 0; i < g_result_list->ndochits;) {
		doc_hit = &(g_result_list->doc_hits[i]);
        is_remove = 0;

	    memset(group_count, 0x00, sizeof(int)*MAX_GROUP_RULE);

		/* did의 group value를 가져온다 */
		if(document_get_group_values(doc_hit, curr_group_values, rules) != SUCCESS) {
			error("can not get group value");
			return FAIL;
		}

		for(j = 0; j < rules->cnt; j++) {
			limit_t* limit = &rules->list[j].limit;

			if(limit->start == -1 || limit->cnt == -1) {
				is_remove = (is_remove == 0) ? 0 : is_remove;
			} else if(group_count[j] < limit->start ||
					group_count[j] > (limit->start + limit->cnt -1)) {
				is_remove = (is_remove == 0) ? 1 : is_remove;
			} else {
				is_remove = (is_remove == 0) ? 0 : is_remove;
			}
		
		    if(curr_group_values[j] < MAX_CARDINALITY) {
			    result->result[j][curr_group_values[j]]++;
			}
		}


		// 조건을 만족하면 저장한다.
		if(is_remove == 0) {
			g_result_list->doc_hits[save_pos] = g_result_list->doc_hits[i];
			save_pos++;
		}

		// group counting
		for(j = 0; j < rules->cnt; j++) {
			group_count[j]++;
		}

		for(j = 0; j < req->virtual_rule_list.cnt; j++) {
			key_rule_t* rule = &req->virtual_rule_list.list[j];

			if(rule->type == DOCATTR) {
		        docattr = g_docattr_base_ptr + g_docattr_record_size*(doc_hit->id-1);

				if (sb_run_docattr_get_field_integer_function(docattr, 
												 rule->name, &virtual_id_list[j]) == -1) {
					error("cannot get value of [%s] field", rule->name);
					return FAIL;
				}
			} else { // virtual_document의 key는 RELEVANCY가 될수 없다.
				virtual_id_list[j] = doc_hit->id;
			}
		}

		for(i++; i < g_result_list->ndochits; i++) {
            int equals_vid = 1;
            is_remove = 0;
			doc_hit = &(g_result_list->doc_hits[i]);

            /* 같은 가상문서인지를 먼저 체크해야 한다. */ 
		    for(j = 0; j < req->virtual_rule_list.cnt; j++) {
			    key_rule_t* rule = &req->virtual_rule_list.list[j];

				if(rule->type == DOCATTR) {
		            docattr = g_docattr_base_ptr + g_docattr_record_size*(doc_hit->id-1);

					if (sb_run_docattr_get_field_integer_function(docattr, 
													 rule->name, &next_virtual_id_list[j]) == -1) {
						error("cannot get value of [%s] field", rule->name);
						return FAIL;
					}
				} else { // virtual_document의 key는 RELEVANCY가 될수 없다.
				    next_virtual_id_list[j] = doc_hit->id;
				}
			}

		    for(j = 0; j < req->virtual_rule_list.cnt; j++) {
				if(virtual_id_list[j] != next_virtual_id_list[j]) {
					equals_vid = 0;
					break;
				}
			}

		    if(equals_vid == 0) {
                break;
            }

		    /* 다음 did의 group value를 가져온다 */
			if(document_get_group_values(doc_hit, next_group_values, rules) != SUCCESS) {
				error("can not get group value");
				return FAIL;
			}

			for(j = 0; j < rules->cnt; j++) {
				limit_t* limit = &rules->list[j].limit;

				if(limit->start == -1 || limit->cnt == -1) {
					is_remove = (is_remove == 0) ? 0 : is_remove;
				} else if(group_count[j] < limit->start ||
						group_count[j] > (limit->start + limit->cnt -1)) {
					is_remove = (is_remove == 0) ? 1 : is_remove;
				} else {
					is_remove = (is_remove == 0) ? 0 : is_remove;
				}

				if(next_group_values[j] < MAX_CARDINALITY) {
					result->result[j][curr_group_values[j]]++;
				}
			}

			// 조건을 만족하면 저장한다.
            if(is_remove == 0) {
				g_result_list->doc_hits[save_pos] = g_result_list->doc_hits[i];
				save_pos++;
            }

			// group counting
			for(j = 0; j < rules->cnt; j++) {
				if(curr_group_values[j] == next_group_values[j]) {
					group_count[j]++;
				}
			}
		}
	}

	g_result_list->ndochits = save_pos;

	return SUCCESS;
}

static int operation_orderby(orderby_rule_list_t* rules, enum doc_type doc_type)
{
    int rv = 0;

	if(doc_type == DOCUMENT) {
        rv = document_orderby(rules); 
	} else {
        rv = virtual_document_orderby(rules); 
	}

	if(rv != SUCCESS) {
		error("can not orderby virtual_document");
		return FAIL;
	}

	return SUCCESS;
}

static int operation_where(char* where, enum doc_type doc_type)
{
    int rv = 0;

	if(doc_type == DOCUMENT) {
		if (sb_run_qp_set_where_expression(where) == FAIL)
			return FAIL;

		rv = document_where(); 
	} else {
		if (sb_run_qp_set_where_expression(where) == FAIL)
			return FAIL;

		rv = virtual_document_where(); 
	}

	if(rv != SUCCESS) {
		error("can not where virtual_document");
		return FAIL;
	}

    return SUCCESS;
}

static int operation_groupby_orderby(groupby_rule_list_t* groupby_rules,
                                     orderby_rule_list_t* orderby_rules,
                                     groupby_result_list_t* result,
									 request_t* req,
									 enum doc_type doc_type)
{
    int i = 0;
    int rv = 0;

    orderby_rule_list_t join_orderby_rules;

    join_orderby_rules.cnt = 0;

	// DID 기준으로 정렬할때는 가상문서 키로 정렬이 되어 있어야 한다.
	for(i = 0; i < req->virtual_rule_list.cnt; i++) {
		key_rule_t* virtual_key = &req->virtual_rule_list.list[i];

		if(doc_type == DOCUMENT && virtual_key->type != DID) {
			join_orderby_rules.list[join_orderby_rules.cnt].rule = *virtual_key;
			join_orderby_rules.list[join_orderby_rules.cnt].type = ASC;
			join_orderby_rules.cnt++;
		}
	}

	/* groupby, orerby의 조건을 하나로 합친다. 순서중요 */
	for(i = 0; groupby_rules != NULL && i < groupby_rules->cnt; i++) {
        join_orderby_rules.list[join_orderby_rules.cnt++] = groupby_rules->list[i].sort;
	}

	for(i = 0; orderby_rules != NULL && i < orderby_rules->cnt; i++) {
        join_orderby_rules.list[join_orderby_rules.cnt++] = orderby_rules->list[i];
	}

	if(doc_type == DOCUMENT) {
		rv = document_orderby(&join_orderby_rules); 
	} else {
		rv = virtual_document_orderby(&join_orderby_rules); 
	}

	if(rv != SUCCESS) {
        error("can not orderby virtual_document");
		return FAIL;
	}

	// groupby 후에는 group counting 한다.
	if(groupby_rules != NULL) {
		if(doc_type == DOCUMENT) {
			rv = document_grouping(req, result);
		} else {
			rv = virtual_document_grouping(result);
		}
	}

	if(rv != SUCCESS) {
		error("can not grouping");
		return FAIL;
	}

    return SUCCESS;
}

static int operation_countby(groupby_rule_list_t* groupby_rules,
                                     orderby_rule_list_t* orderby_rules,
                                     groupby_result_list_t* result,
									 request_t* req,
									 enum doc_type doc_type)
{
    int i = 0;
    int rv = 0;

    orderby_rule_list_t join_orderby_rules;

    join_orderby_rules.cnt = 0;

	// DID 기준으로 정렬할때는 가상문서 키로 정렬이 되어 있어야 한다.
	for(i = 0; i < req->virtual_rule_list.cnt; i++) {
		key_rule_t* virtual_key = &req->virtual_rule_list.list[i];

		if(doc_type == DOCUMENT && virtual_key->type != DID) {
			join_orderby_rules.list[join_orderby_rules.cnt].rule = *virtual_key;
			join_orderby_rules.list[join_orderby_rules.cnt].type = ASC;
			join_orderby_rules.cnt++;
		}
	}

	/* groupby, orerby의 조건을 하나로 합친다. 순서중요 */
	for(i = 0; groupby_rules != NULL && i < groupby_rules->cnt; i++) {
        join_orderby_rules.list[join_orderby_rules.cnt++] = groupby_rules->list[i].sort;
	}

	for(i = 0; orderby_rules != NULL && i < orderby_rules->cnt; i++) {
        join_orderby_rules.list[join_orderby_rules.cnt++] = orderby_rules->list[i];
	}

	if(groupby_rules != NULL) {
		if(doc_type == DOCUMENT) {
			rv = document_grouping(req, result);
		} else {
			rv = virtual_document_grouping(result);
		}
	}

	if(rv != SUCCESS) {
		error("can not grouping");
		return FAIL;
	}

    return SUCCESS;
}

static int operation_limit(limit_t* rule, enum doc_type doc_type)
{
    int i = 0;
    int j = 0;
    int save_pos = 0;

	if(doc_type == DOCUMENT) {
		for (i = rule->start; i < g_result_list->ndochits && j < rule->cnt ; i++, j++) {
			g_result_list->doc_hits[save_pos] = g_result_list->doc_hits[i];
			save_pos++;
		}

		g_result_list->ndochits = save_pos;
	} else {
		for (i = rule->start; i < g_vdl->cnt && j < rule->cnt ; i++, j++) {
			g_vdl->data[save_pos] = g_vdl->data[i];
			save_pos++;
		}

	    g_vdl->cnt = save_pos;
	}


    return SUCCESS;
}

static void print_select(select_list_t* rule)
{
    int i = 0;

    for(i = 0; i < rule->cnt; i++) {
        debug("select field[%s], is_highlight[%d], comment_length[%d]", 
		      rule->field[i].name,
			  rule->field[i].is_highlight,
			  rule->field[i].comment_length);
    } 
}

static void print_orderby(orderby_rule_t* rule)
{
	debug("key name[%s], type[%s], orderby[%s]", 
		  rule->rule.name,
		  key_type_str[rule->rule.type],
          order_type_str[rule->type+1]); // -1 base : 이전 코드와 호환을 위해
}

static void print_limit(limit_t* rule)
{
    debug("limit start[%d], cnt[%d]", rule->start, rule->cnt);
}

static void print_groupby(groupby_rule_t* rule)
{
	debug("==== groupby start ====");
	print_orderby(&rule->sort);
	print_limit(&rule->limit);
	debug("==== groupby end ====");
}

static int do_filter_operation(request_t* req, response_t* res, enum doc_type doc_type)
{
	int i = 0;
	int j = 0;
	int rv = 0;
	int* result_cnt = NULL;
    operation_t* op = NULL;
    operation_t* next_op = NULL;
	operation_list_t* op_list = NULL;
	groupby_result_list_t* groupby_result = NULL;

	if(doc_type == DOCUMENT) {
	    op_list = &req->op_list_did;
		groupby_result = &res->groupby_result_did;
		result_cnt = &g_result_list->ndochits;
	} else {
	    op_list = &req->op_list_vid;
		groupby_result = &res->groupby_result_vid;
		result_cnt = &g_vdl->cnt;
	}

	info("before do_filter_operate doc_type[%s], cnt[%d]", doc_type_str[doc_type], *result_cnt);
    for(i = 0; i < op_list->cnt; i++) {
		j = i+1; // next op
		op = &op_list->list[i];
		next_op = &op_list->list[j];

	    info("before %s : doc_type[%s], cnt[%d]", get_clause_str(op->type), 
				                             doc_type_str[doc_type], *result_cnt);
	    switch(op->type) {
		   case GROUP_BY:
			   while(next_op->type == GROUP_BY || next_op->type == COUNT_BY) {
                   // groupby 조건을 join 한다.
                   memcpy(&op->rule.groupby.list[op->rule.groupby.cnt], 
                       &next_op->rule.groupby.list, 
                       next_op->rule.groupby.cnt*sizeof(groupby_rule_t));
                   op->rule.groupby.cnt += next_op->rule.groupby.cnt;

				   j++;
		           next_op = &op_list->list[j];
			   }

               // response에 결과 출력을 위한 group 정보 저장.
			   groupby_result->rules = op->rule.groupby;

			   if(next_op->type == ORDER_BY) {
				   rv = operation_groupby_orderby(&op->rule.groupby, 
									  &next_op->rule.orderby, groupby_result, req, doc_type);
				   if(rv != SUCCESS) {
                       MSG_RECORD(&req->msg, error,
                             "can not operate operation_groupby_orderby");
					   return FAIL;
				   }
				   j++;
			   } else {
				   rv = operation_groupby_orderby(&op->rule.groupby, NULL, 
								 groupby_result, req, doc_type);
				   if(rv != SUCCESS) {
                       MSG_RECORD(&req->msg, error, 
                             "can not operate operation_groupby_orderby");
					   return FAIL;
				   }
			   }
			   i = j-1;

			   break;
		   case COUNT_BY:
			   while(next_op->type == COUNT_BY) {
                   // groupby 조건을 join 한다.
                   memcpy(&op->rule.groupby.list[op->rule.groupby.cnt], 
                       &next_op->rule.groupby.list, 
                       next_op->rule.groupby.cnt*sizeof(groupby_rule_t));
                   op->rule.groupby.cnt += next_op->rule.groupby.cnt;

				   j++;
		           next_op = &op_list->list[j];
			   }

               // response에 결과 출력을 위한 group 정보 저장.
			   groupby_result->rules = op->rule.groupby;

			   rv = operation_countby(&op->rule.groupby, NULL, 
							 groupby_result, req, doc_type);
			   if(rv != SUCCESS) {
				   MSG_RECORD(&req->msg, error, 
						 "can not operate operation_groupby_orderby");
				   return FAIL;
			   }
			   i = j-1;

			   break;
		   case ORDER_BY:
			   rv = operation_groupby_orderby(NULL, &op->rule.orderby,
                                 groupby_result, req, doc_type);
			   if(rv != SUCCESS) {
                   MSG_RECORD(&req->msg, error, "can not operate operation_orderby");
				   return FAIL;
			   }
			   break;
		   case WHERE:
			   rv = operation_where(op->rule.where, doc_type);
			   if(rv != SUCCESS) {
                   MSG_RECORD(&req->msg, error, "can not operate operation_where");
				   return FAIL;
			   }

			   if(doc_type == VIRTUAL_DOCUMENT) {
                   res->search_result = res->vdl->cnt;
			   }
			   break;
		   case LIMIT:
			   rv = operation_limit(&op->rule.limit, doc_type);
			   if(rv != SUCCESS) {
                   MSG_RECORD(&req->msg, error, "cannot operate operation_limit");
				   return FAIL;
			   }
			   break;
           default:
               MSG_RECORD(&req->msg, warn, "it is not a valid operator: [%d][%s]", op->type, 
                                       get_clause_str(op->type));
               break;
	    }
	    info("after %s : doc_type[%s], cnt[%d]", get_clause_str(op->type), 
				                              doc_type_str[doc_type], *result_cnt);
    }

    info("after do_filter_operate doc_type[%s] cnt[%d]", doc_type_str[doc_type], *result_cnt);
    return SUCCESS;
}

/////////////////////////////////////////////////////////
/*    end search result filtering operation stuff      */
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
/* start search interface                              */
/////////////////////////////////////////////////////////
static int full_search (request_t *req, response_t *res)
{
	int ret=FAIL;

time_start();
	ret = light_search(req, res);
	if (ret < 0) {
        MSG_RECORD(&req->msg, error, "light search error");
		return FAIL;
	}
	INFO("light_search done");
time_mark("light_search");

	ret = abstract_search(req, res);
	if (ret < 0) {
        MSG_RECORD(&req->msg, error, "abstract search error");
		return FAIL;
	}
	INFO("abstract_search done");
time_mark("abstract_search");
time_status();

#ifdef DEBUG_SOFTBOTD
	show_num_of_free_nodes();
#endif

	return SUCCESS;
}

static int light_search (request_t *req, response_t *res)
{
	int num_of_node, rv;
	QueryNode qnodes[MAX_QUERY_NODES];

	if (strlen(req->search) == 0) {
        MSG_RECORD(&req->msg, warn, "length of query string is zero");
		return FAIL;
	}

	num_of_node = 
		sb_run_preprocess(word_db, req->search, MAX_QUERY_STRING_SIZE,
									qnodes, MAX_QUERY_NODES);
	if ( num_of_node > 0 ) {
		sb_run_buffer_querynode(res->parsed_query, qnodes, num_of_node);
	}
time_mark("preprocess");

#ifdef DEBUG_SOFTBOTD
	debug("res->parsed_query[%s]", res->parsed_query);
	INFO("postfix query");
	sb_run_print_querynode(qnodes,num_of_node);
#endif

	if (num_of_node < 0) {
        MSG_RECORD(&req->msg, error, "preprocess error");
		return FAIL;
	}
	else if (num_of_node == 0) {
        MSG_RECORD(&req->msg, warn, "no operand is available");
		res->parsed_query[0] = '\0';
		g_result_list = NULL;
		return SUCCESS;
	}
	else if (num_of_node > MAX_QUERY_NODES) {
		crit("num_of_node[%d] > MAX_QUERY_NODES[%d]", num_of_node, MAX_QUERY_NODES);
		crit("setting num_of_node: MAX_QUERY_NODES[%d]", MAX_QUERY_NODES);
		num_of_node = MAX_QUERY_NODES;
		// node가 실종되는 위험을 안고 있다.
	}

	rv = do_search_operation(qnodes, num_of_node, &g_result_list);
	if (rv == FAIL) {
        MSG_RECORD(&req->msg, error, "operation failed");
		return FAIL;
	}
time_mark("do_search");

	/* in case no AND/OR/WITHIN.., operations are done */
	g_result_list = read_index_list(g_result_list);
	if (g_result_list == NULL) {
        MSG_RECORD(&req->msg, error, "read_index_list failed");
		return FAIL;
	}

    if (g_result_list->ndochits == 0) {
		//g_result_list = NULL;
        return SUCCESS;
    }

	reduce_dochits_to_one_per_doc(g_result_list);
time_mark("reduce");

    rv = sb_run_docattr_get_base_ptr((docattr_t**)&g_docattr_base_ptr, &g_docattr_record_size);
    if(rv != SUCCESS) {
        MSG_RECORD(&req->msg, error, "can not get docattr base ptr");
        return FAIL;
    }

    rv = do_filter_operation(req, res, DOCUMENT);
    if(rv == FAIL) {
        MSG_RECORD(&req->msg, error, "can not fileter operation - document");
		release_list( g_result_list );
        return FAIL;
    }
time_mark("filter");

    rv = make_virtual_document(g_result_list, req);
    if(rv == FAIL) {
        MSG_RECORD(&req->msg, error, "can not make virtual document list");
		release_list( g_result_list );
        return FAIL;
    }
time_mark("virtual doc");

    // filtering 전의 건수
    res->search_result = res->vdl->cnt;

    rv = do_filter_operation(req, res, VIRTUAL_DOCUMENT);
    if(rv == FAIL) {
        MSG_RECORD(&req->msg, error, "can not fileter operation - virtual_document");
		release_list( g_result_list );
        return FAIL;
    }
time_mark("filter2");

	return SUCCESS;
}

static int virtual_document_fill_comment(request_t* req, response_t* res)
{
    int i = 0;
    int j = 0;
	int cmt_idx = 0;

	/* highlight를 위한 단어 목록을 생성한다. */
	set_search_words(req, res);

    for(i = 0; i < g_vdl->cnt; i++) {
		if(cmt_idx >= COMMENT_LIST_SIZE) {
			g_vdl->data[i].comment_cnt = 0;
            MSG_RECORD(&req->msg, error, "over comment max count[%d]", COMMENT_LIST_SIZE);
			return FAIL;
		} else {
			for(j = 0; j < g_vdl->data[i].dochit_cnt && cmt_idx < COMMENT_LIST_SIZE; j++) {
				debug("cmt_idx[%d]", cmt_idx);

				res->comments[cmt_idx].did = g_vdl->data[i].dochits[j].id;
				get_comment(req, &g_vdl->data[i].dochits[j], &req->select_list, res->comments[cmt_idx].s);
		        if(cmt_idx >= COMMENT_LIST_SIZE) {
                    MSG_RECORD(&req->msg, error, "over comment max count[%d]", COMMENT_LIST_SIZE);
					break;
				}
                cmt_idx++;
			    g_vdl->data[i].comment_cnt = j+1;
			}
		}
    }

    return SUCCESS;
}

static int abstract_search(request_t *req, response_t *res)
{
	if ( highlight_word == H_WORD_PARSED )
	{ /* NOTE: abstract_search의 경우, SEARCH 질의식을 구문해석할 필요가
	   *       없다. 그러나 highlight를 PARSED 단어 기준으로 하는 경우,
	   *       구문해석을 할 수 밖에 없다.
       */
		int num_of_node;
		QueryNode qnodes[MAX_QUERY_NODES];

		num_of_node = 
			sb_run_preprocess(word_db, req->search, MAX_QUERY_STRING_SIZE,
									qnodes, MAX_QUERY_NODES);
		if ( num_of_node > 0 ) {
			sb_run_buffer_querynode(res->parsed_query, qnodes, num_of_node);
		}
	}

    return virtual_document_fill_comment(req, res);
}

/////////////////////////////////////////////////////////
/* end search interface                                */
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
/* framework related stuff                             */
/////////////////////////////////////////////////////////
static int private_init(void)
{
	int ret=0;

	if(qp_initialized) 
		return SUCCESS;
	else
	    qp_initialized = 1;

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

	// word db open
	ret = sb_run_open_word_db( &word_db, word_db_set );
	if ( ret != SUCCESS && ret != DECLINE ) {
		error("word db open failed");
		return FAIL;
	}
    		
	b_use_cdm = (find_module("mod_cdm.c") != NULL);
	if ( b_use_cdm ) {
		// cdm db open
		ret = sb_run_server_canneddoc_init();
		if ( ret != SUCCESS && ret != DECLINE ) {
			error("cdm db open failed");
			return FAIL;
		}
	}
	else { // use cdm2 api
		if ( mCdmSet == -1 ) {
			error("invalid CdmSet[%d]. set CdmSet in <"__FILE__">", mCdmSet);
			return FAIL;
		}
		ret = sb_run_cdm_open( &mCdm, mCdmSet );
		if ( ret != SUCCESS ) {
			error("cdm[set:%d] open failed", mCdmSet);
			return FAIL;
		}
	}

	if(g_vdl == NULL) {
		g_vdl = (virtual_document_list_t*)sb_malloc(sizeof(virtual_document_list_t));
		g_vdl->data = (virtual_document_t*)sb_malloc(sizeof(virtual_document_t)*MAX_DOC_HITS_SIZE);
		g_vdl->cnt = 0;

		debug("virtual_document data size[%d]", sizeof(virtual_document_t)*MAX_DOC_HITS_SIZE);
	}

	return SUCCESS;
}

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

static void set_max_comment_bytes(configValue v)
{
    max_comment_bytes = atoi(v.argument[0]);

	if(max_comment_bytes >= LONG_LONG_STRING_SIZE) {
		max_comment_bytes = LONG_LONG_STRING_SIZE-1;

		warn("fixed max comment bytes[%d]", LONG_LONG_STRING_SIZE-1);
	}
}

static void set_max_comment_return_bytes(configValue v)
{
    max_comment_return_bytes = atoi(v.argument[0]);

	if(max_comment_return_bytes >= LONG_LONG_STRING_SIZE) {
		max_comment_return_bytes = LONG_LONG_STRING_SIZE-1;

		warn("fixed max comment return bytes[%d]", LONG_LONG_STRING_SIZE-1);
	}
}
static void setCdmSet(configValue v)
{
	mCdmSet = atoi( v.argument[0] );
}

static void set_highlight_post_tag(configValue v)
{
	strncpy(highlight_post_tag, v.argument[0], MAX_HIGHTLIGHT_TAG);
	highlight_post_tag[MAX_HIGHTLIGHT_TAG-1] = '\0';
}

static void set_highlight_pre_tag(configValue v)
{
	strncpy(highlight_pre_tag, v.argument[0], MAX_HIGHTLIGHT_TAG);
	highlight_pre_tag[MAX_HIGHTLIGHT_TAG-1] = '\0';
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
    field_info[field_count].indexer_morpid = atoi(v.argument[3]);
    field_info[field_count].qpp_morpid = atoi(v.argument[4]);

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

static void get_word_db_set(configValue v)
{
	word_db_set = atoi( v.argument[0] );
}

static void set_highlight_seperator1byte(configValue v)
{
	strncpy(seps1, v.argument[0], MAX_HIGHLIGHT_SEP_LEN);
	seps1[MAX_HIGHLIGHT_SEP_LEN-1] = '\0';

	debug("a byte seperator[%s]", seps1);
}

static void set_highlight_seperator2byte(configValue v)
{
	strncpy(seps2, v.argument[0], MAX_HIGHLIGHT_SEP_LEN);
	seps2[MAX_HIGHLIGHT_SEP_LEN-2] = '\0';

	debug("two byte seperator[%s]", seps2);
}

static void set_highlight_word_length(configValue v)
{
	highlight_word_len = atoi( v.argument[0] );
}

static void set_highlight_unit(configValue v)
{
	if (strncasecmp(v.argument[0], "word", 4) == 0)
	{
		highlight_unit = H_UNIT_WORD;
	}
}

static void set_highlight_word(configValue v)
{
	if (strncasecmp(v.argument[0], "parsed", 5) == 0)
	{
		highlight_word = H_WORD_PARSED;
	}
}

static void set_remove_char_query(configValue v)
{
	strncpy(remove_char_query, v.argument[0], MAX_REMOVE_CHAR_QUERY);
	remove_char_query[MAX_REMOVE_CHAR_QUERY-1] = '\0';

	debug("remove char query[%s]", remove_char_query);
}

static config_t config[] = {
	CONFIG_GET("WordDbSet", get_word_db_set, 1, "WordDbSet {number}"),
	CONFIG_GET("DbType",setDbType,1, "vrfi or indexdb"),
	CONFIG_GET("IndexDbPath",setIndexDbPath,1,
			"inv indexer db path (only vrfi) (e.g: IndexDbPath /home/)"),
	CONFIG_GET("IndexDbSet",setIndexDbSet,1,
			"index db set (type is indexdb) (e.g: IndexDbSet 1)"),
	CONFIG_GET("CdmSet",setCdmSet,1, "cdm db set (e.g: CdmSet 1)"),

	CONFIG_GET("Field",get_commentfield,VAR_ARG, "Field which needs to be shown in result"),
	CONFIG_GET("FieldSortingOrder",get_FieldSortingOrder,2, "Field sorting order"),
	CONFIG_GET("MaxCommentBytes",set_max_comment_bytes, 1, "Max comment bytes"),
	CONFIG_GET("MaxCommentReturnBytes",set_max_comment_return_bytes, 1, "Max comment return bytes"),
	CONFIG_GET("HighlightPreTag",set_highlight_pre_tag, 1, "highliight pre tag ex) <b>"),
	CONFIG_GET("HighlightPostTag",set_highlight_post_tag, 1, "highliight post tag ex) </b>"),
	CONFIG_GET("HighlightSeperator1Byte",set_highlight_seperator1byte, 1, "highliight seperator 1 byte"),
	CONFIG_GET("HighlightSeperator2Byte",set_highlight_seperator2byte, 1, "highliight seperator 2 byte"),
	CONFIG_GET("HighlightWordLength",set_highlight_word_length, 1, "highlight word len"),
	CONFIG_GET("HighlightUnit",set_highlight_unit, 1, "highlight unit ex) word, eojeol"),
	CONFIG_GET("HighlightWord",set_highlight_word, 1, "highlight type ex) input, parsed"),
	CONFIG_GET("RemoveCharQuery",set_remove_char_query, 1, "remove char query"),
	{NULL}
};

static void register_hooks(void)
{
	/* XXX: module which uses qp should call sb_run_qp_init once after fork. */
	sb_hook_qp_init(private_init,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_init_request(init_request,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_init_response(init_response,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_light_search(light_search,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_full_search(full_search,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_abstract_search(abstract_search,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_do_filter_operation(do_filter_operation,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_get_query_string(get_query_string,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_finalize_search(finalize_search,NULL,NULL,HOOK_MIDDLE);
}

module qp2_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	NULL,        			/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};

/* options for vim 
 * vim:ts=4
 */
