/* $Id$ */ 
#include "softbot.h"

#include <math.h> /* XXX: log(3) move to softbot.h */
#include <sys/mman.h>

#include <pthread.h> //XXX: autoconf?

#include "mod_qp.h"
#include "mod_api/qp.h"
#include "mod_api/vrfi.h"
#include "mod_api/mod_api.h"
#include "mod_indexer/mod_index_doc.h"

#include "mod_api/register.h"

static cdm_db_t *qpcdmdb = NULL;
static char cdmdbname[MAX_DBNAME_LEN];
static char cdmdbpath[MAX_DBPATH_LEN];

static char rootelement[STRING_SIZE] = "Document";

static int mCommentFieldNum=0;
static char mCommentField[MAX_EXT_FIELD][SHORT_STRING_SIZE];
static uint32_t mDefaultSearchFilter=0xffffffff;

REGISTRY int rVrfSemid=-1;
static VariableRecordFile *mVRFI=NULL;

static char mIdxFilePath[MAX_PATH_LEN]="dat/indexer/index";
static pthread_mutex_t listmutex;

#define MAX_DOCATTR_SORTING_CONF		32
static char *sortingorder[MAX_DOCATTR_SORTING_CONF] = { NULL };

static index_list_t index_list_pool[MAX_INDEX_LIST_POOL];

/* stack related stuff */

index_list_t *free_list_root = NULL;

index_list_t *get_free_node(void)
{
	index_list_t *this = NULL;
	doc_hit_t *doc_hits = NULL;
	uint32_t *relevancy = NULL;

	pthread_mutex_lock(&listmutex);
	if ( free_list_root == NULL ) {
		warn("free_list_root is NULL");
		pthread_mutex_unlock(&listmutex);
		return NULL;
	}
	else if ( free_list_root->next == NULL ) {
		warn("free_list_root->next is NULL");
		pthread_mutex_unlock(&listmutex);
		return NULL;
	}

	this = free_list_root;
	free_list_root = free_list_root->next;
	free_list_root->prev = NULL;

	// pointer가 가리키는 값들을 잃어버리지 않도록..
	doc_hits = this->doc_hits;
	relevancy = this->relevancy;
	memset(this,0x00,sizeof(index_list_t));
	this->doc_hits = doc_hits;
	this->relevancy = relevancy;

	pthread_mutex_unlock(&listmutex);
	return this;
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
	index_list_t *this;
	
	this = stack->last;

	if ( stack->size == 0 ) {
		warn("empty stack");
		return NULL;
	}
	return this;
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

void release_node(index_list_t *this)
{
	if ( this == NULL ) return;

	pthread_mutex_lock(&listmutex);

	free_list_root->prev = this;
	this->next = free_list_root;

	free_list_root = this;
	free_list_root->prev = NULL;

	pthread_mutex_unlock(&listmutex);
	return;
}

int finalize_search(request_t *r)
{
	release_node(r->result_list);
	return SUCCESS;
}

void init_stack(sb_stack_t *stack)
{
	stack->size = 0;
	stack->first = NULL;
	stack->last = NULL;
}

void init_free_list()
{
	int i = 0;
	index_list_t *this;

	free_list_root = &(index_list_pool[0]);

	for ( i = 0; i<MAX_INDEX_LIST_POOL; i++) {
		this = &(index_list_pool[i]);
		if ( i == 0 ) {
			this->prev = NULL;
			this->next = &(index_list_pool[i+1]);
		} else if ( i == MAX_INDEX_LIST_POOL - 1 ) {
			this->prev = &(index_list_pool[i-1]);
			this->next = NULL;
		} else {
			this->prev = &(index_list_pool[i-1]);
			this->next = &(index_list_pool[i+1]);
		}

/* XXX: manpage says valloc is obsolete. do we need to stick to this? 
 *      the all elements of list *will* be always referenced, so, no swap in/swap out
 *      will occur! we must believe *the* OS --jiwon, 2002/08/29 */
/*
#ifdef HAVE_VALLOC
		this->doc_hits = (doc_hit_t*)valloc(MAX_DOC_HITS_SIZE * sizeof(doc_hit_t));
		if (this->doc_hits == NULL) {
			error("fail calling calloc: %s", strerror(errno));
			return;
		}
		bzero(this->doc_hits, MAX_DOC_HITS_SIZE * sizeof(doc_hit_t));
#ifdef HAVE_MADVISE
		if (madvise(this->doc_hits, 
					MAX_DOC_HITS_SIZE * sizeof(doc_hit_t), 
					MADV_SEQUENTIAL) < 0) {
			error("fail calling madvice: %s", strerror(errno));
		}
#endif
#else // if ! HAVE_VALLOC 
*/
		this->doc_hits = (doc_hit_t*)sb_calloc(MAX_DOC_HITS_SIZE, sizeof(doc_hit_t));
		if (this->doc_hits == NULL) {
			error("fail calling calloc: %s", strerror(errno));
			return;
		}
/*
#endif
*/
		this->relevancy = (uint32_t*)sb_calloc(MAX_DOC_HITS_SIZE, sizeof(uint32_t));
		if (this->relevancy == NULL) {
			error("fail calling calloc: %s", strerror(errno));
			return;
		}
	}

	return;
}

/* end of stack related stuff */

int readin_inv_idx(QueryNode qnode, index_list_t *list)
{
	int i=0, idx=0, ret=0;
	uint32_t ndocs=0;
	uint32_t wordid=qnode.word_st.id;
	uint32_t field = qnode.field;
	
	if (qnode.word_st.id <= 0) {
		SB_DEBUG_ASSERT(qnode.word_st.id == 0);
		info("qnode.word_st.wordid(%u) is less than or equal to 0",
								qnode.word_st.id);
		list->ndocs = 0;
		return FAIL;
	}

	acquire_lock(rVrfSemid);
		sb_run_vrfi_get_fixed_dup(mVRFI, (uint32_t)wordid,(void *)&(list->header));
		ndocs = (list->header.ndocs < MAX_DOC_HITS_SIZE) ?
				list->header.ndocs : MAX_DOC_HITS_SIZE;

		debug("getting fixed data(ndocs) for key[%u] ndocs:%d",
												 wordid, ndocs);
		ret=sb_run_vrfi_get_variable(mVRFI, (uint32_t)wordid, 0L, 
								ndocs,(void *)(list->doc_hits));
		if (ret < 0) {error("error vrf get variable wordid:%u",wordid);}
	release_lock(rVrfSemid);
	
	debug("copy dochits of selected field");
	if (field != 0xffffff) {
		for (i=0,idx=0 ; i<ndocs ; i++) {
			if ( list->doc_hits[i].field & field ) {
				list->doc_hits[idx++] = list->doc_hits[i];
			}
		}
		list->ndocs = idx;
	}

	list->field = field;
	strncpy(list->word,qnode.word_st.string, MAX_WORD_LEN);
	CRIT("qnode.field:%s",sb_strbin(field, sizeof(uint32_t)));
	CRIT("ndocs(%d)",list->ndocs);

	return SUCCESS;
}

inline static int get_nhits(doc_hit_t *doc_hit)
{
	return doc_hit->nhits;
/*
	if ( doc_hit->nhits < STD_HITS_LEN )
		return doc_hit->nhits;
	else
		return STD_HITS_LEN;

	return STD_HITS_LEN;*/
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
static void get_weight(QueryNode *qnode, DocId total_doc_number,  index_list_t *list)
{
	int i=0;
	double temp=0.0;
	
	// get_weight 함수를 호출하기전에 걸러주는 것도....
	if (total_doc_number == 0 || qnode->word_st.id ==0) {
		warn("total_doc_num:%u,  wordid:%u",
			(uint32_t)total_doc_number, (uint32_t)qnode->word_st.id);
		return;
	}
	
	debug("get_weight: word[%s] word.df[%d] total_doc_num[%ld]"
		  , qnode->word_st.string, qnode->word_st.word_attr.df , total_doc_number);
	debug("list->nodes[%d]",list->ndocs);

	temp = (2*total_doc_number) / qnode->word_st.word_attr.df;
	
	for (i=0 ; i<list->ndocs ; i++) {
		list->relevancy[i] = list->doc_hits[i].nhits * (uint32_t)log(temp);
	}
	
	return;
}
int get_shortest_ordered_dist(index_list_t *l1, int idx1, index_list_t *l2, int idx2)
{
	int nhits1=0, nhits2=0;
	int i=0,j=0,pos1=0,pos2=0;
	int32_t dist,tmp;
	int field=0;

	nhits1 = get_nhits(&(l1->doc_hits[idx1]));
	nhits2 = get_nhits(&(l2->doc_hits[idx2]));

	nhits1 = nhits1 > STD_HITS_LEN ? STD_HITS_LEN : nhits1;
	nhits2 = nhits2 > STD_HITS_LEN ? STD_HITS_LEN : nhits2;

	dist = MAX_STD_POSITION;
	for (i=0; i<nhits1; i++) {
		field = get_field(&(l1->doc_hits[idx1].hits[i]));
		if ( !( l1->field & ((uint32_t)1<<field) ) ) continue;

		pos1 = get_position(&(l1->doc_hits[idx1].hits[i]));
		if (pos1==MAX_STD_POSITION) continue;

		for (j=0; j<nhits2; j++) {
			field = get_field(&(l2->doc_hits[idx2].hits[j]));
			if ( !( l2->field & ((uint32_t)1<<field) ) ) continue;	

			if (cmp_field( &(l1->doc_hits[idx1].hits[i]), 
						   &(l2->doc_hits[idx2].hits[j])) != TRUE) 
				continue;

			pos2 = get_position(&(l2->doc_hits[idx2].hits[j]));
			tmp = pos2-pos1;

			if (pos2==MAX_STD_POSITION) {
				continue;
			}
			else if (tmp < 0) {
				continue;
			}

			if (tmp < dist)
				dist = tmp;
		}
	}

	return dist;
}

int get_shortest_dist(index_list_t *l1, int idx1, index_list_t *l2, int idx2)
{
	int nhits1=0, nhits2=0;
	int i=0,j=0,pos1=0,pos2=0;
	int32_t dist,abs_dist,tmp,abs_tmp;
	int field=0;

	nhits1 = get_nhits(&(l1->doc_hits[idx1]));
	nhits2 = get_nhits(&(l2->doc_hits[idx2]));

	nhits1 = nhits1 > STD_HITS_LEN ? STD_HITS_LEN : nhits1;
	nhits2 = nhits2 > STD_HITS_LEN ? STD_HITS_LEN : nhits2;

	abs_dist = dist = MAX_STD_POSITION;

	for (i=0; i<nhits1; i++) {
		field = get_field(&(l1->doc_hits[idx1].hits[i]));
		if ( !( l1->field & ((uint32_t)1<<field) ) ) continue;

		pos1 = get_position(&(l1->doc_hits[idx1].hits[i]));
		if (pos1==MAX_STD_POSITION) continue;
			
		for (j=0; j<nhits2; j++) {
			field = get_field(&(l2->doc_hits[idx2].hits[j]));
			if ( !( l2->field & ((uint32_t)1<<field) ) ) continue;

			if (cmp_field( &(l1->doc_hits[idx1].hits[i]), 
						   &(l2->doc_hits[idx2].hits[j])) != TRUE) 
				continue;

			pos2 = get_position(&(l2->doc_hits[idx2].hits[j]));
			tmp = pos2-pos1;

			if (pos2==MAX_STD_POSITION) {
				continue;
			}

			abs_tmp = (tmp>0) ? tmp: -tmp;

			if (abs_tmp < abs_dist) {
				abs_dist = abs_tmp;
				dist = tmp;
			}
		}
	}

	return dist;
}

int get_posweight(index_list_t *l1, int idx1, index_list_t *l2, int idx2)
{
#define MAX_DISTANCE_FOR_WEIGHT (5)
#define REVERSE_ORDER_PENALTY	(4)
	int nhits1=0, nhits2=0;
	int distance=0, weight=0, nearness=0;
	int i=0,j=0,pos1=0,pos2=0;

	nhits1 = get_nhits(&(l1->doc_hits[idx1]));
	nhits2 = get_nhits(&(l2->doc_hits[idx2]));

	debug("nhits1 [%d]  nhits2 [%d]",nhits1 , nhits2);
	
	weight = 0;
	for(i = 0; i < nhits1; i++) {
		pos1 = get_position(&(l1->doc_hits[idx1].hits[i]));
		for(j = i; j < nhits2; j++) {
			pos2 = get_position(&(l2->doc_hits[idx2].hits[j]));
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
	debug("pos_weight[%d]",pos_weight);
	
	dest->doc_hits[idx] = l1->doc_hits[idx1];
	dest->relevancy[idx] = l1->relevancy[idx1];

	return;
}

void insert_to_result(index_list_t *dest, int idx, 
					  index_list_t *l1, int idx1,
					  index_list_t *l2, int idx2) {
	int pos_weight=0;
	debug("pos_weight[%d]",pos_weight);
	
	pos_weight = get_posweight(l1,idx1, l2,idx2);
	
	dest->doc_hits[idx] = l2->doc_hits[idx2];
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
	index_list_t *operand1=NULL, *operand2=NULL;
	
	num_of_operands = qnode->num_of_operands;
	if (num_of_operands != 2) {
		error("not and must have 2 operands but has %d", num_of_operands);
		num_of_operands = 2;
	}

	operand2 = stack_pop(stack);
	operand1 = stack_pop(stack);
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
		return FAIL;
	}

	if (operand->list_type == FAKE_OP) {
		stack_push(stack,operand);
		return SUCCESS;
	}

	this = get_free_node();
	this->list_type = NORMAL;
	
	if ( this == NULL || operand1 == NULL || operand2 == NULL ) {/*{{{*/
		error("no free qnode");
		release_node(this);
		release_node(operand1);
		release_node(operand2);
		return FAIL;
	}/*}}}*/

	CRIT("Operator not and");
	INFO("operand->ndocs:%u, not->ndocs:%u",operand->ndocs, not->ndocs);
	// 실제로 document id를 비교해서 not하는 부분.
	// Actually compares document id and operate "NOT AND"
	idx2=0;idx3=0;
	for (idx1=0; idx1<operand->ndocs; idx1++) {
		while ( idx2 < not->ndocs &&
				not->doc_hits[idx2].id < operand->doc_hits[idx1].id) {
			idx2++;
		}
		
		if (not->doc_hits[idx2].id == operand->doc_hits[idx1].id)
			continue;

		INFO("inserting to result");
		insert_one_to_result(this,idx3,operand,idx1);
		idx3++;
	}
	this->ndocs = idx3;

	release_node(operand);
	release_node(not);
	stack_push(stack, this);

	return SUCCESS;
}

int operator_and(sb_stack_t *stack, QueryNode *qnode)
{
	int num_of_operands = 0;
	int idx1 = 0, idx2 = 0, idx3 = 0;
/*	int lookahead1 = 100, lookahead2 = 100; XXX*/
	index_list_t *this=NULL, *operand1=NULL, *operand2=NULL, *tmp=NULL;
	
	num_of_operands = qnode->num_of_operands;
	debug("num_of_operands[%d]",num_of_operands);
	debug("op_param :%d",qnode->opParam);

	operand2 = stack_peek(stack);
	operand1 = stack_peek(stack);
	if (operand1->list_type == EXCLUDE || operand2->list_type == EXCLUDE) {
		return operator_and_with_not(stack, qnode);
	}

	while ( num_of_operands >= 2 ) {
		idx1=0; idx2=0; idx3=0;
		num_of_operands -= 2;
		operand2 = stack_pop(stack);
		operand1 = stack_pop(stack);
	
		this = get_free_node();
		this->list_type = NORMAL;
		
		if ( this == NULL || operand1 == NULL || operand2 == NULL ) {
			error("no free qnode");
			release_node(this);
			release_node(operand1);
			release_node(operand2);
			return FAIL;
		}

		if (operand1->list_type == FAKE_OP && operand2->list_type == FAKE_OP){
			this->list_type = FAKE_OP;
			goto DONE_OPERATE_AND;
		}

        if (operand1->list_type == FAKE_OP) {/*{{{*/
			tmp = this;
			this = operand2;
			operand2 = this;
			goto DONE_OPERATE_AND;
        }/*}}}*/
        if (operand2->list_type == FAKE_OP) {/*{{{*/
			tmp = this;
			this = operand1;
			operand1 = tmp;
			goto DONE_OPERATE_AND;
        }/*}}}*/

		// 실제로 document id를 비교해서 and하는 부분.
		// Actually compares document id and operate "AND"
		while ( idx1 < operand1->ndocs && idx2 < operand2->ndocs ) {
			if ( operand1->doc_hits[idx1].id
					< operand2->doc_hits[idx2].id ) {
				idx1++;
			} else if ( operand1->doc_hits[idx1].id
					> operand2->doc_hits[idx2].id ) {
				idx2++;
			} else {
				insert_to_result(this,idx3, operand1,idx1, operand2,idx2);
				idx1++; idx2++;
				idx3++;
				this->ndocs++;
			}
		}
DONE_OPERATE_AND:		
		release_node(operand1);
		release_node(operand2);
		stack_push(stack, this);
		num_of_operands++;
	} /* while ( num_of_... */
	
	return SUCCESS;
}
int operator_phrase(sb_stack_t *stack, QueryNode *qnode)
{
	int num_of_operands = 0;
	int idx1 = 0, idx2 = 0, idx3 = 0;
/*	int lookahead1 = 100, lookahead2 = 100; XXX*/
	index_list_t *this=NULL, *operand1=NULL, *operand2=NULL, *tmp=NULL;
	int dist=0,needed_dist=3;
	
	num_of_operands = qnode->num_of_operands;
	debug("num_of_operands[%d]",num_of_operands);
	debug("op_param :%d",qnode->opParam);

	while ( num_of_operands >= 2 ) {
		idx1=0; idx2=0; idx3=0;
		num_of_operands -= 2;
		operand2 = stack_pop(stack);
		operand1 = stack_pop(stack);
	
		debug("operand1->list_type [%d]",operand1->list_type);
		debug("operand2->list_type [%d]",operand2->list_type);
		INFO("operand1->word[%s], operand2->word[%s]",operand1->word, operand2->word);
		
		this = get_free_node();
		this->list_type = NORMAL;
		
		if ( this == NULL || operand1 == NULL || operand2 == NULL ) {
			error("no free qnode");
			release_node(this);
			release_node(operand1);
			release_node(operand2);
			return FAIL;
		}

		if (operand1->list_type == FAKE_OP && operand2->list_type == FAKE_OP){
			this->list_type = FAKE_OP;
			goto DONE_OPERATE_PHRASE;
		}

        if (operand1->list_type == FAKE_OP) {/*{{{*/
			tmp = this;
			this = operand2;
			operand2 = this;
			goto DONE_OPERATE_PHRASE;
        }/*}}}*/
        if (operand2->list_type == FAKE_OP) {/*{{{*/
			tmp = this;
			this = operand1;
			operand1 = tmp;
			goto DONE_OPERATE_PHRASE;
        }/*}}}*/

		while ( idx1 < operand1->ndocs && idx2 < operand2->ndocs ) {
			if ( operand1->doc_hits[idx1].id
					< operand2->doc_hits[idx2].id ) {
				idx1++;
			} else if ( operand1->doc_hits[idx1].id
					> operand2->doc_hits[idx2].id ) {
				idx2++;
			} else {
				dist = get_shortest_ordered_dist(operand1,idx1, operand2,idx2);

				if (dist > needed_dist) {
					idx1++; idx2++;
					continue;
				}

				insert_to_result(this,idx3, operand2,idx2, operand1,idx1);
				idx1++; idx2++;
				idx3++;
				this->ndocs++; //XXX: this->ndocs=idx3 ??
			}
		}

		this->field = operand1->field;

        strncpy(this->word,operand1->word,STRING_SIZE);
        this->word[STRING_SIZE-1] = '\0';
        strncat(this->word,"+",1);
        strncat(this->word,operand2->word,STRING_SIZE-strlen(operand1->word)-1);
        this->word[STRING_SIZE-1] = '\0';

		CRIT("this->ndocs:%u",this->ndocs);

DONE_OPERATE_PHRASE:
		release_node(operand1);
		release_node(operand2);
		stack_push(stack, this);
		num_of_operands++;
	} /* while ( num_of_... */
	
	return SUCCESS;
}
int operator_within(sb_stack_t *stack, QueryNode *qnode)
{
	int num_of_operands = 0;
	int idx1 = 0, idx2 = 0, idx3 = 0;
/*	int lookahead1 = 100, lookahead2 = 100; XXX*/
	index_list_t *this=NULL, *operand1=NULL, *operand2=NULL, *tmp=NULL;
	int dist=0,abs_dist=0,needed_dist=qnode->opParam;
	
	num_of_operands = qnode->num_of_operands;
	debug("num_of_operands[%d]",num_of_operands);
	debug("op_param :%d",qnode->opParam);

	while ( num_of_operands >= 2 ) {
		idx1=0; idx2=0; idx3=0;
		num_of_operands -= 2;
		operand2 = stack_pop(stack);
		operand1 = stack_pop(stack);
	
		debug("operand1->list_type [%d]",operand1->list_type);
		debug("operand2->list_type [%d]",operand2->list_type);
		
		this = get_free_node();
		this->list_type = NORMAL;
		
		if ( this == NULL || operand1 == NULL || operand2 == NULL ) {
			error("no free qnode");
			release_node(this);
			release_node(operand1);
			release_node(operand2);
			return FAIL;
		}

		if (operand1->list_type == FAKE_OP && operand2->list_type == FAKE_OP){
			this->list_type = FAKE_OP;
			goto DONE_OPERATE_WITHIN;
		}

        if (operand1->list_type == FAKE_OP) {/*{{{*/
			tmp = this;
			this = operand2;
			operand2 = this;
			goto DONE_OPERATE_WITHIN;
        }/*}}}*/
        if (operand2->list_type == FAKE_OP) {/*{{{*/
			tmp = this;
			this = operand1;
			operand1 = tmp;
			goto DONE_OPERATE_WITHIN;
        }/*}}}*/

		// 실제로 document id를 비교해서 and하는 부분.
		// Actually compares document id and operate "AND"
		while ( idx1 < operand1->ndocs && idx2 < operand2->ndocs ) {
			if ( operand1->doc_hits[idx1].id
					< operand2->doc_hits[idx2].id ) {
				idx1++;
			} else if ( operand1->doc_hits[idx1].id
					> operand2->doc_hits[idx2].id ) {
				idx2++;
			} else {
				dist = get_shortest_dist(operand1,idx1, operand2,idx2);
				abs_dist = (dist>0) ? dist:-dist;
				if (abs_dist > needed_dist) {
					idx1++; idx2++;
					continue;
				}

				insert_to_result(this,idx3, operand2,idx2, operand1,idx1);
				idx1++; idx2++;
				idx3++;
				this->ndocs++; //XXX: this->ndocs=idx3 ??
			}
		}

		this->field = operand2->field;

        strncpy(this->word,operand1->word,STRING_SIZE);
        this->word[STRING_SIZE-1] = '\0';
        strncat(this->word,"+",1);
        strncat(this->word,operand2->word,STRING_SIZE-strlen(operand1->word)-1);
        this->word[STRING_SIZE-1] = '\0';

DONE_OPERATE_WITHIN:		
		release_node(operand1);
		release_node(operand2);
		stack_push(stack, this);
		num_of_operands++;
	} /* while ( num_of_... */
	
	return SUCCESS;
}

int operator_or(sb_stack_t *stack, QueryNode *node)
{
    int num_of_operands = 0;
    int idx1=0, idx2=0, idx3=0;
/*    int i,j;*/
    index_list_t *this, *operand1, *operand2;
/*    int nhits1, nhits2;*/

	debug("operator or section");
	
    debug("&node:%p",node);
    num_of_operands = node->num_of_operands;
    debug("num_of_operands[%d]",num_of_operands);
    debug("&node:%p",node);

    while (num_of_operands >= 2 ) {
    	idx1=0; idx2=0; idx3=0;
        num_of_operands -= 2;
        debug("&node:%p",node);
        operand2 = stack_pop(stack);
        debug("&node:%p",node);
        operand1 = stack_pop(stack);
        debug("&node:%p",node);

        this = get_free_node();
		this->list_type = NORMAL;
	    
		debug("operand1->list_type [%d]",operand1->list_type);
	    debug("operand2->list_type [%d]",operand2->list_type);
		
		
        if ( this == NULL || operand1 == NULL || operand2 == NULL ) {
            error("no free node");
            release_node(this);
            release_node(operand1);
            release_node(operand2);
            return FAIL;
        }

		// list type check !
		
		if (operand1->list_type == EXCLUDE || operand1->list_type == EXCLUDE) {
			warn("OR opertor ignore not operator");
		}

		if (operand1->list_type == FAKE_OP) {
			this->list_type = operand2->list_type;
		}
		if (operand2->list_type == FAKE_OP) {
			this->list_type = operand1->list_type;
		}
		
        while ( (idx1 < operand1->ndocs) || (idx2 < operand2->ndocs) ) {
		
			debug("operand1->doc_hits[%d].id(%d) "
				  "operand2->doc_hits[%d].id(%d), idx3(%d)",
			idx1,operand1->doc_hits[idx1].id,
			idx2,operand2->doc_hits[idx2].id,idx3);
			
			if (idx1 == operand1->ndocs) {
			
                    this->doc_hits[idx3] = operand2->doc_hits[idx2];
                    this->relevancy[idx3] = operand2->relevancy[idx2];
                    idx2++;	
				
			}else if (idx2 == operand2->ndocs) {
			
                    this->doc_hits[idx3] = operand1->doc_hits[idx1];
                    this->relevancy[idx3] = operand1->relevancy[idx1];
                    idx1++;	
				
			} else {
				if ( operand1->doc_hits[idx1].id
						< operand2->doc_hits[idx2].id ) {
					debug("operand1->doc_hits[%d].id(%d) < "
						  "operand2->doc_hits[%d].id(%d), idx3(%d)",
					idx1,operand1->doc_hits[idx1].id,
					idx2,operand2->doc_hits[idx2].id,idx3);

					this->doc_hits[idx3] = operand1->doc_hits[idx1];
					this->relevancy[idx3] = operand1->relevancy[idx1];
					idx1++;
				} else if ( operand1->doc_hits[idx1].id
						> operand2->doc_hits[idx2].id ) {
					debug("operand1->doc_hits[%d].id(%d) > "
						  "operand2->doc_hits[%d].id(%d), idx3(%d)",
					idx1,operand1->doc_hits[idx1].id,
					idx2,operand2->doc_hits[idx2].id,idx3);

					this->doc_hits[idx3] = operand2->doc_hits[idx2];
					this->relevancy[idx3] = operand2->relevancy[idx2];
					idx2++;
				} else {
					debug("operand1->doc_hits[%d].id(%d) == "
						  "operand2->doc_hits[%d].id(%d), idx3(%d)",
						  idx1,operand1->doc_hits[idx1].id,
						  idx2,operand2->doc_hits[idx2].id,idx3);

					//XXX ranking for intersection  just add tow relevancy
					this->doc_hits[idx3] = operand2->doc_hits[idx2];
					this->relevancy[idx3] = operand1->relevancy[idx1]
										  + operand2->relevancy[idx2];

					idx1++; idx2++;
				}

				debug("operand1->doc_hits[%d].id(%d) "
					  "operand2->doc_hits[%d].id(%d), idx3(%d)",
				idx1,operand1->doc_hits[idx1].id,
				idx2,operand2->doc_hits[idx2].id,idx3);
			}
          
		    this->ndocs++;
            idx3++;
		}
		
        release_node(operand1);
        release_node(operand2);
        stack_push(stack, this);
        num_of_operands++;
    } /*while ( num_of_ */
    return SUCCESS;
}


/* ranking point not change in operator not */
/*int operator_and_not(stack_t *stack, QueryNode *node)*/
int operator_not(sb_stack_t *stack, QueryNode *node)
{
	index_list_t *operand = stack_pop(stack);

	INFO("making EXCLUDE list type");
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
			return operator_and(stack, node);
		case QPP_OP_OR:
			return operator_or(stack, node);
		case QPP_OP_NOT:
			return operator_not(stack, node);
		case QPP_OP_WITHIN:
			return operator_within(stack, node);
		case QPP_OP_MORP:
			node->opParam = 1;
			return operator_within(stack, node);
		case QPP_OP_PHRASE:
			return operator_phrase(stack, node);

		//XXX : OP_NEAR 를 OP_PARA 대신 사용한다. QPP 에서 지원해주게 고치고 바꿈..
		//XXX : obsolete
		case QPP_OP_PARA:
			return operator_and(stack, node);
			
		default:
			warn("invalid operator[%d]", node->operator);
	}
	return FAIL;
}

int abstract_info (request_t *r)
{
	return 0;
}

int full_info (request_t *r)
{
	return 0;
}

static int get_abstract (int			num_retrieved_doc,
						RetrievedDoc	aDoc[],
						pdom_t			*docs[])
{
	int i;
	if (num_retrieved_doc > MAX_NUM_RETRIEVED_DOC) {
		error("too big number of argument 1; max number of retrieved document is %d",
				MAX_NUM_RETRIEVED_DOC);
	}

	for (i=0; i<num_retrieved_doc; i++) {
		if ((docs[i] = sb_run_cdm_retrieve_internal(qpcdmdb, aDoc[i].docId)) == NULL) {
			error("error while doc_get document of internal key[%lu]",aDoc[i].docId);
			return FAIL;
		}
	}

	return SUCCESS;
}

int cdm_get_field(parser_t 		*parser,
		          char			*fieldname,
				  void         **buffer,
				  int           *bufferlen) {
	char path[STRING_SIZE];
	int len;
	field_t *field;

	snprintf(path, STRING_SIZE, "/%s/%s", rootelement, fieldname);
	path[STRING_SIZE-1] = '\0';
	field = sb_run_xmlparser_retrieve_field(parser, path);
	if (field == NULL) {
		warn("doc_get_field error"); 
		return FAIL;
	}
	len = field->size > DOCUMENT_SIZE ? DOCUMENT_SIZE : field->size;

	*buffer = (char *)sb_calloc(1, len + 1);
	if (*buffer == NULL) {
		warn("doc_get_field error"); 
		return FAIL;
	}
	strncpy(*buffer, field->value, len);
	((char *)*buffer)[len] = '\0';
	*bufferlen = len;

	return SUCCESS;
}

#define COMMENT_SIZE 10 /* words */
static void fill_title_and_comment(request_t *req)
{
	int i=0,j=0,k=0,ret=0, last=0;
	uint32_t docid=0;
	int searched_list_size=0,tmp=0;
	char *tmpstr=NULL;
	int buflen;

	RetrievedDoc rdoc[MAX_NUM_RETRIEVED_DOC];
	pdom_t *doc[MAX_NUM_RETRIEVED_DOC] = {NULL};
	parser_t *parser = NULL;

	if (req->result_list->ndocs == 0) { info("no searched result"); return; }

	debug("first_result:%d",req->first_result);
	debug("list size:%d",req->list_size);

	if ( req->result_list->ndocs <  req->first_result ) {
		searched_list_size = 0;
	} else {
		tmp = req->result_list->ndocs - req->first_result;

		searched_list_size = (tmp > req->list_size) ? req->list_size:tmp;
	}

	if (searched_list_size > COMMENT_LIST_SIZE) {
		error("searched_list_size[%d] > COMMENT_LIST_SIZE[%d]",
					searched_list_size, COMMENT_LIST_SIZE);
		searched_list_size = COMMENT_LIST_SIZE;
	}

	// make up request of abstracted document
	last = req->first_result + searched_list_size;
	
	//XXX: rdoc index(i) and result_list->doc_hit index(j) differs.
	for (i = req->first_result,j=0; i < last; i++,j++) {
		docid = req->result_list->doc_hits[i].id;

		rdoc[j].docId = docid;
		rdoc[j].rank = 0;
		rdoc[j].rsv = 0;

		rdoc[j].numAbstractInfo = mCommentFieldNum;

		for (k=0; k<mCommentFieldNum; k++) {
			strncpy(rdoc[j].cdAbstractInfo[k].field,mCommentField[k],
					SHORT_STRING_SIZE);
#ifdef PARAGRAPH_POSITION 
			/* FIXME: should be made to be modifiable per each site */
			rdoc[j].cdAbstractInfo[k].paragraph_position = 0;
#endif
			rdoc[j].cdAbstractInfo[k].position = 0;
			rdoc[j].cdAbstractInfo[k].size = -1; 
			/* -1 for whole text retrival */
		}
	}

	debug("req->first_result:%d, last:%d",req->first_result,last);

/*	ret = sb_run_doc_get_abstract((int)searched_list_size, rdoc, doc);*/
/*	ret = sb_run_docapi_get_abstract((int)searched_list_size, rdoc, doc);*/
	ret = get_abstract((int)searched_list_size, rdoc, doc);
	if (ret < 0) {
		error("canneddoc get abstract error.");
		release_node(req->result_list);
		req->result_list = NULL;
		goto DONE_COMMENTING;
	}

	//XXX: result_list->doc_hit index and other index differs.
	CRIT("## req->first_result:%u",req->first_result);
	for (i = req->first_result,j=0; i < last; i++,j++) {
		int sizeleft;
		docid = req->result_list->doc_hits[i].id; //XXX -- for debug
		if (docid == 0) {
			crit("docid(%u) < 0",(uint32_t)docid);
			continue;
		}

		req->comments[j][0]='\0'; /* ready for strcat */
		sizeleft = LONG_LONG_STRING_SIZE-1;
		parser = sb_run_cdm_get_parser(doc[j]);
		for (k=0; k<mCommentFieldNum; k++) {
			tmpstr = NULL;
/*			ret = sb_run_doc_get_field(doc[j], NULL, mCommentField[k], &tmpstr);*/
/*			ret = sb_run_docapi_get_field(doc[j], mCommentField[k], &tmpstr, &buflen);*/
			ret = cdm_get_field(parser, mCommentField[k], (void *)&tmpstr, &buflen);
			if (ret < 0) {
				error("doc_get_field error for doc[%d], field[%s]", 
												docid, mCommentField[k]);
				continue;
			}

			strncat(req->comments[j],mCommentField[k],sizeleft);
			sizeleft -= strlen(mCommentField[k]);
			sizeleft = (sizeleft < 0) ? 0:sizeleft;

			strncat(req->comments[j],":",sizeleft);
			sizeleft -= 1;
			sizeleft = (sizeleft < 0) ? 0:sizeleft;

			strncat(req->comments[j],tmpstr,sizeleft);
			sizeleft -= strlen(tmpstr);
			sizeleft = (sizeleft < 0) ? 0:sizeleft;
			sb_free(tmpstr);

			strncat(req->comments[j],";",sizeleft);
			sizeleft -= 1;
			sizeleft = (sizeleft < 0) ? 0:sizeleft;

			if (sizeleft <= 0) {
				crit("req->comments size lack while pushing comment(field:%s)",
															mCommentField[k]);
				break;
			}
		}

		debug("doc[%d] (title: %s)",docid,req->titles[j]);
		debug("comments> \n   %s",req->comments[j]);
	}

	//XXX: ask jiwon or dong-hun
	for (i=req->first_result,j=0; i<last; i++,j++) {
		req->result_list->doc_hits[j].id = req->result_list->doc_hits[i].id;
		req->result_list->relevancy[j] = req->result_list->relevancy[i];
	}

DONE_COMMENTING:
	for (i = 0; i < searched_list_size; i++) {
/*		sb_run_doc_free(doc[i]);*/
		sb_run_cdm_release(doc[i]);
	}
}

int full_search (request_t *req)
{
	int ret=FAIL;

	INFO("req.list_size:%d",req->list_size);
	INFO("req.first_result:%d",req->first_result);

	ret = light_search(req);
	if (ret < 0) {
		error("light search error");
		req->result_list = NULL;
		return FAIL;
	}
	debug("light_search done");

	fill_title_and_comment(req);

	return SUCCESS;
}

index_list_t *docattr_filtering(index_list_t *src, char *attrquery);
static int docattr_sorting(index_list_t *list, char *sortquery);
#define MAX_QUERY_NODES 60
int light_search (request_t *req)
{
	int i,num_of_node;
	QueryNode qnodes[MAX_QUERY_NODES];
	index_list_t *new_node=NULL;
	DocId total_doc_num=0;
	sb_stack_t stack = { 0, NULL, NULL }; /* stack = { size, first, last } */

	req->word_list[0] = '\0';

	debug("req.list_size:%d",req->list_size);
	debug("req.first_result:%d",req->first_result);
	debug("req.attr_string[%s]",req->attr_string);
	debug("before preprocessing, query str: [%s]",req->query_string);

	num_of_node = 
		sb_run_preprocess(req->query_string, MAX_QUERY_STRING_SIZE, qnodes, MAX_QUERY_NODES);
/*		sb_run_preprocess(req->query_string,FALSE,MAX_QUERY_NODES,qnodes);*/

#ifdef DEBUG_SOFTBOTD
	debug("after preprocessed query number %d",num_of_node);
	sb_run_print_querynode(stderr,qnodes,num_of_node);
#endif

	// QPP 를 고친다면 이 부분이 나오는 일은 없도록 만들어야 한다.
	if (num_of_node <= 0 || num_of_node > MAX_INDEX_LIST_POOL) {
		warn("Query [%s] num_of_node(%d) ",req->query_string, num_of_node);
		strcat(req->word_list, req->query_string);
		strcat(req->word_list, "^"); /* FIXME buffer overflow check */
		return FAIL;		
	}
	
	total_doc_num = sb_run_last_indexed_did();
	debug("last indexed did[%ld]",total_doc_num);
	
	for ( i = 0; i<num_of_node ; i++ ) {
		if ( qnodes[i].type == OPERAND ) {
			new_node = get_free_node();
			if ( new_node == NULL ) {
				warn("no free node");
				break; /* for ( i = 0 ... */
			}
			
			/* set index_list_type */
			if (qnodes[i].opParam == -10) { // XXX -10 is proper?
				new_node->list_type = FAKE_OP;
				new_node->ndocs = 0;
			} else if (qnodes[i].word_st.id == 0) {
				new_node->list_type = NOT_EXIST_WORD;
				new_node->ndocs = 0;
			} else {
				readin_inv_idx(qnodes[i], new_node);
				debug("qnodes[i],opParam =(%d)",qnodes[i].opParam); 
				new_node->list_type = NORMAL;
				get_weight(&(qnodes[i]), total_doc_num, new_node);
			}

			debug("stack pushing word[%s](wordid[%d])(word.df[%d])",
				   qnodes[i].word_st.string, 
				   qnodes[i].word_st.id,
				   qnodes[i].word_st.word_attr.df);
			
			stack_push(&stack, new_node);
			strcat(req->word_list, qnodes[i].word_st.string);
			strcat(req->word_list, "^"); /* FIXME buffer overflow check */
		} else if ( qnodes[i].type == OPERATOR ) {
			debug("operator[%d]. ",qnodes[i].operator);
			operate(&stack, &(qnodes[i]));
		} else {
			error("node type is neither OPERATOR[%d] nor OPERAND[%d] :[%d]",
											OPERATOR,OPERAND,qnodes[i].type);
		}
	}
	
	sb_run_print_querynode(stderr,qnodes,num_of_node);

	if ( stack.size == 0 ) {
		// FIXME: "no document found 라고 나와야 함"
		error("stack is empty after query processed");
		return FAIL;
	} else if ( stack.size > 1 ) {
		// FIXME: 이 경우에도 stack에 남아있는 내용중 operand인 것중의 
		// 		  하나를 보내야 한다. 에러는 내 보내고..
		error("invalid stack size [%d] after query [%s] processed.", 
				stack.size, req->query_string);
		
		while ( stack.last != NULL ) {
			index_list_t *tmp;
			tmp = stack.last->prev;
			release_node(stack.last);
			stack.last = tmp;
		}
		return FAIL;
	}

    if (stack.first->ndocs == 0) {
        req->result_list = stack.first;
        return SUCCESS;
    }
/*	if ((new_node = docattr_filtering(stack.first, req->attr_string)) */
/*			== NULL) {*/
/*		return FAIL;*/
/*	}*/

/*	if (new_node->ndocs == 0) {*/
/*		req->result_list = new_node;*/
/*		return SUCCESS;*/
/*	}*/
/*	if (sb_run_docattr_modify_index_list(0, new_node) == -1) {*/
/*		return FAIL;*/
/*	}*/

/*	if (docattr_sorting(new_node, req->sort_string) == FAIL) {*/
/*		return FAIL;*/
/*	}*/

	req->result_list = stack.first;
	// XXX: should be freed by caller

	return SUCCESS;
}

static int docattr_sorting(index_list_t *list, char *sortquery)
{
	docattr_sort_t sc;
	char *cur, *d1, *d2;
	int i, n = atoi(sortquery);
	char sortingquerydup[MAX_SORT_STRING_SIZE];

	if (sortingorder[n] == NULL || sortingorder[n][0] == '\0') {
		return SUCCESS;
	}
	strcpy(sortingquerydup, sortingorder[n]);
	sortquery = sortingquerydup;
//	CRIT("sortquery:%s",sortquery);
//	CRIT("1.list:%p",list);

	cur = sortquery; i = 0;
	while ((d1 = strchr(cur, ':')) != NULL) {
		*d1 = '\0';
		strncpy(sc.keys[i].key, cur, MAX_SORT_STRING_SIZE);
		sc.keys[i].key[MAX_SORT_STRING_SIZE-1] = '\0';
		cur = d1 + 1;

//		CRIT("2[%d]-1.list:%p",i, list);
		if ((d2 = strchr(cur, ';')) == NULL) {
			warn("wrong sorting query: no ordering rule");
			return SUCCESS;
		}
//		CRIT("2[%d]-2.list:%p",i, list);

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

//	CRIT("3.list:%p",list);
//	CRIT("before sorting");
	if (sb_run_docattr_index_list_sortby(list, &sc, SC_SORT) == -1) {
		error("error in process of sorting by docattr module");
		return FAIL;
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
	sort_list_recursive(list->relevancy,list->doc_hits,0,list->ndocs-1);
}

static int private_init(void)
{
	int ret=0;
	ret=pthread_mutex_init(&listmutex, NULL);
	if (ret != 0) {
		error("error initializing list mutex: %s",strerror(errno));
	}

	init_free_list();
	return SUCCESS;
}

static int module_init(void)
{
	registry_t *reg=NULL;
	int ret=0;

	reg = registry_get("VRFLock");
	rVrfSemid = *(int*)(reg->data);

	debug("vrf semid: %d",rVrfSemid);

	debug("calling vrf_reopen");

	ret = sb_run_vrfi_alloc(&mVRFI);
	if (ret == FAIL) {
		error("failed to allocate vrfi");
		return FAIL;
	}
	if (mVRFI == NULL) {
		error("error allocating memory for vrf object");
		return FAIL;
	}

	acquire_lock(rVrfSemid);
		ret = sb_run_vrfi_open(mVRFI,mIdxFilePath,sizeof(inv_idx_header_t), sizeof(doc_hit_t));
		if (ret != SUCCESS) {
			crit("vrf_reopen failed.");
			return FAIL;
		}
	release_lock(rVrfSemid);

	qpcdmdb = sb_run_cdm_db_open(cdmdbname, cdmdbpath, CDM_SHARED);
	if (qpcdmdb == NULL) {
		error("cannot open cdmdb[%s, %s]", cdmdbname, cdmdbpath);
		return FAIL;
	}

	return SUCCESS;
}

/*****************************************************************************/
static void setIndexDbPath(configValue v)
{
	strncpy(mIdxFilePath,v.argument[0],MAX_PATH_LEN);
	mIdxFilePath[MAX_PATH_LEN-1] = '\0';
	debug("indexer db path: %s",mIdxFilePath);
}

static void set_cdmdb(configValue v)
{
	strncpy(cdmdbname, v.argument[0], MAX_DBNAME_LEN);
	cdmdbname[MAX_DBNAME_LEN-1] = '\0';

	snprintf(cdmdbpath, MAX_DBPATH_LEN, "%s/%s",
			gSoftBotRoot, v.argument[1]);
	cdmdbpath[MAX_DBPATH_LEN-1] = '\0';

	info("configure: cdmdb[%s, %s]", cdmdbname, cdmdbpath);
}

static void get_commentfield(configValue v)
{
	int idx=0;

	if (v.argNum < 7) return;

	if (mCommentFieldNum >= MAX_EXT_FIELD) {
		error("mCommentFieldNum(%d) >= MAX_EXT_FIELD(%d).",
				mCommentFieldNum,MAX_EXT_FIELD);
		error("Increase MAX_EXT_FIELD and recompile");
		return;
	}

	if (strncasecmp("RETURN",v.argument[6],SHORT_STRING_SIZE) != 0) {
		error("Field: %s %s, 5th column should RETURN or blank.. not [%s]",
				v.argument[0],v.argument[1],v.argument[5]);
		return;
	}

	idx = mCommentFieldNum;
	strncpy(mCommentField[idx], v.argument[1], SHORT_STRING_SIZE);

	INFO("commentfield:[%s] set",mCommentField[idx]);

	mCommentFieldNum++;
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

static void get_defaultsearchfield(configValue v)
{
	int i=0;
	uint32_t fieldid=0;

	mDefaultSearchFilter=0L;

	for (i=0; i<v.argNum; i++) {
		fieldid = atoi(v.argument[i]);
		mDefaultSearchFilter |= (1L << fieldid);
	}

	WARN("mDefaultSearchFilter:%x",mDefaultSearchFilter);
}

static config_t config[] = {
	CONFIG_GET("IndexDbPath",setIndexDbPath,1,\
			"inv indexer db path (e.g: IndexDbPath /home/)"),
	CONFIG_GET("Field",get_commentfield,VAR_ARG,\
			"Field which needs to be shown in result"),
	CONFIG_GET("FieldSortingOrder",get_FieldSortingOrder,2,\
			"Field sorting order"),
	CONFIG_GET("CdmDatabase", set_cdmdb, 2,\
			"cdm database db name, path: CdmDatabase [dbname dbpath]"),
	{NULL}
};

static void register_hooks(void)
{
	/* XXX: module which uses qp should call sb_run_qp_init once after fork. */
	sb_hook_qp_init(private_init,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_light_search(light_search,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_full_search(full_search,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_abstract_info(abstract_info,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_full_info(full_info,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_finalize_search(finalize_search,NULL,NULL,HOOK_MIDDLE);
}

module qp_cdmi_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	module_init,			/* initialize */
	//module_main,			/* child_main */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
