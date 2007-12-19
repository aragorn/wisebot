/* $Id$ */
#include "common_core.h"
#include "memory.h"
#include "util.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/lexicon.h"
#include "mod_api/qp.h"

#include "stack.h"
#include "tokenizer.h"
#include "mod_qpp.h"
#include "daum_qpp.h"

#include <string.h>
#include <stdlib.h>

#define BIGRAM_TRUNCATION_SEARCH
//#undef BIGRAM_TRUNCATION_SEARCH

#define TURN_BINARY_OPERATOR	(1)
#define TURN_OPERAND			(2)

/* * 되었을 때/SYNONYM/MORPHEME 확장하는 단어 수 */
/* int8_t 내에 있는 수이어야 한다 */
#define STARRED_WORD_NUM		(10)	/* *로 확장하는 단어 갯수 */

#define MAX_QPP_INDEXED_WORDS MAX_WORD_LEN

static uint32_t mDefaultSearchField=0xffffffff;
static uint32_t mDefaultPhraseField=0xffffffff;
static int mMorpIdForPhrase = 20;

static char mPrecedence[32];		// operator precedence

static QueryNode m_defaultOperatorNode = {
	QPP_OP_FUZZY,
	OPERATOR,
	2, /* num of operands */
	0, /* opParam */
	0, /* field */
	0.0,
	{"",0}	
}; // operator가 생략되었을 때 넣을 operator용
static QueryNode m_zeroWordidNode = {
	QPP_OP_OPERAND,
	OPERAND,
	0,  /* num_of_operands */
	0, 	/* opParam */ //FIXME fake operand 구별방법. 
	0, /* field */
	0.0,
	{"",0}	
};// wordid가 0인(없는 id) 인 query node
static QueryNode m_fakeQueryNode = {
	QPP_OP_OPERAND,
	OPERAND,
	0,  /* num_of_operands */
	-10, /* opParam */ //FIXME!!!!!! fake operand 구별방법. 
	0, /* field */
	0.0,
	{"",0}	
};// wordid가 0인(없는 id) 인 query node

static void setFakeOperand();

static void initPrecedence();
static int pushDefaultOperator(StateObj *pStObj);
static int pushZeroWordid(StateObj *pStObj);
static int pushFakeOperand(StateObj *pStObj);
static int pushOperator(StateObj *pStObj,QueryNode *pQuNode);
static int pushNumeric(StateObj *pStObj,QueryNode *pQuNode);
static int pushPhrase(StateObj *pStObj);
static int pushExtendedOperand(void* word_db, StateObj *pStObj,QueryNode *pQuNode);
static int pushGenericOperator(StateObj *pStObj,QueryNode *pQuNode);
static int pushStarredWord(void* word_db, StateObj *pStObj,QueryNode *pQuNode);

static int pushLeftEndBigram(void* word_db, StateObj *state, QueryNode *input_qnode);
static int pushRightEndBigram(void* word_db, StateObj *state, QueryNode *input_qnode);
static int pushBothEndBigram(void* word_db, StateObj *state, QueryNode *input_qnode);

static void makeUpperLetter(QueryNode *pQuNode);
/*static int processSyn(StateObj *pStObj,QueryNode *pQuNode);*/

static char operatorName[12][16] = 
{"QPP_OP_AND","QPP_OP_OR","QPP_OP_NOT","QPP_OP_PARA",
 "QPP_OP_WITHIN","QPP_OP_FUZZY","QPP_OP_OPERAND","QPP_OP_MORP",
 "QPP_OP_SYN","QPP_OP_FILTER","QPP_OP_PHRASE","QPP_OP_NUMERIC"};

int QPP_postfixPrint(QueryNode postfix[],int numNode) {
	int i = 0;
	char opNum = 0;

	for ( i=0; i<numNode; i++) {
		if (postfix[i].type == OPERAND) {
			info("word: %s(field:%s)(wordid:%u)(opParam:%d)",
						postfix[i].word_st.string,
						sb_strbin(postfix[i].field,sizeof(postfix[i].field)),
						postfix[i].word_st.id,
						postfix[i].opParam);
		}
		else {
			opNum = postfix[i].operator;
			info("operator: %s (num_operands:%d,param:%d)",operatorName[opNum-1],\
						postfix[i].num_of_operands,postfix[i].opParam);
		}
	}
	return SUCCESS;
}

int QPP_postfixBuffer(char *result, QueryNode postfix[],int numNode) {
	int i = 0;
	char tmpbuf[1024];
	char opNum = 0;

	memset(tmpbuf, 0x00, 1024);
	
	for ( i=0; i<numNode; i++) {
		if (postfix[i].type == OPERAND) {
			sprintf(tmpbuf,"word: %s(field:%s)(wordid:%u)(opParam:%d)\n",
						postfix[i].word_st.string,
						sb_strbin(postfix[i].field,sizeof(postfix[i].field)),
						postfix[i].word_st.id,
						postfix[i].opParam);
			strcat(result, tmpbuf);
		}
		else {
			opNum = postfix[i].operator;
			sprintf(tmpbuf,"operator: %s (num_operands:%d,param:%d)\n",operatorName[opNum-1],\
						postfix[i].num_of_operands,postfix[i].opParam);
			strcat(result, tmpbuf);
		}
	}
	return SUCCESS;
}

/*
 * for infix print
 */
struct qpp_tree_node {
	QueryNode* expr;

	struct qpp_tree_node** child;
	int child_count;
};

static struct qpp_tree_node* qpp_make_tree_node()
{
	struct qpp_tree_node* node =
		(struct qpp_tree_node*) sb_calloc(1, sizeof(struct qpp_tree_node));
	return node;
}

static void qpp_print_tree(struct qpp_tree_node* node, char* buf)
{
	if ( node->child_count == 1 ) { // unary operator
		strcat(buf, qpp_op_to_string(node->expr->operator, node->expr->opParam));
		qpp_print_tree(node->child[0], buf);
	}
	else if ( node->child_count > 1 ) { // operator
		int i;
		strcat(buf, "(");

		for ( i = 0; i < node->child_count; i++ ) {
			qpp_print_tree(node->child[i], buf);

			if ( i != node->child_count-1 ) {
				strcat(buf, " ");
				strcat(buf, qpp_op_to_string(node->expr->operator, node->expr->opParam));
				strcat(buf, " ");
			}
		}

		strcat(buf, ")");
	}
	else { // operand
		strcat(buf, node->expr->word_st.string);
	}
}

static void qpp_destroy_tree(struct qpp_tree_node* node)
{
	int i;

    if(node == NULL) return;

	for ( i = 0; i < node->child_count; i++ ) {
		qpp_destroy_tree(node->child[i]);
	}

	if ( node->child_count ) sb_free(node->child);
	sb_free(node);
}

#define ENOUGH_QUERY_STACK_SIZE 128
int QPP_infixBuffer(char* result, QueryNode postfix[], int numNode)
{
	struct qpp_tree_node* operand_stack[ENOUGH_QUERY_STACK_SIZE];
	int operand_stack_pos = -1;
	int i, j, k;

	result[0] = '\0';

	for ( i = 0; i < numNode; i++ ) {
		struct qpp_tree_node* node = qpp_make_tree_node();
		node->expr = &postfix[i];

		if ( postfix[i].type == OPERAND ) {
			if ( operand_stack_pos >= ENOUGH_QUERY_STACK_SIZE-1 ) {
				error("not enough operand stack size");
				goto return_error;
			}

			operand_stack[++operand_stack_pos] = node;
			continue;
		}
		else { // operator
			if ( operand_stack_pos < postfix[i].num_of_operands-1 ) {
				error("invalid postfix expression");
				goto return_error;
			}

			node->child = (struct qpp_tree_node**)
				sb_malloc( sizeof(struct qpp_tree_node*) * postfix[i].num_of_operands );
			node->child_count = postfix[i].num_of_operands;

			for ( j = operand_stack_pos - postfix[i].num_of_operands + 1, k=0;
					j <= operand_stack_pos; j++ ) {

				node->child[k++] = operand_stack[j];
			}

			operand_stack_pos -= (postfix[i].num_of_operands-1);
			operand_stack[operand_stack_pos] = node;
		}
	}

	if ( operand_stack_pos != 0 ) {
		error("invalid postfix expression");
		goto return_error;
	}

	qpp_print_tree( operand_stack[0], result );
	qpp_destroy_tree( operand_stack[0] );

	return SUCCESS;

return_error:
	QPP_postfixPrint(postfix, numNode);
	for ( i = 0; i <= operand_stack_pos; i++ ) {
		qpp_destroy_tree( operand_stack[i] );
	}
	return FAIL;
}

static int init() {

	initPrecedence();
	setFakeOperand();

	return SUCCESS;
}

int QPP_infixPrint(QueryNode postfix[], int numNode)
{
	char buf[LONG_STRING_SIZE];

	if ( QPP_infixBuffer(buf, postfix, numNode) != SUCCESS ) return FAIL;

	info("QPP(infix expression) : [%s]", buf);
	QPP_postfixPrint(postfix, numNode); // 둘 다 보여주는게 좋을 것 같다.
	return SUCCESS;
}

// susia 
// handle query overflow function
/* static int handle_query_overflow(int maxNode, QueryNode postfix_query[])
{
	int stack_size=0, i=0, safety_stack_size=-1;
#ifdef DEBUG_SOFTBOTD	
	QPP_postfixPrint(stderr, postfix_query , maxNode);
#endif	
	for (i=0; i<maxNode ; i++){
		if (postfix_query[i].type == OPERAND) { 
			stack_size++;
		} else if (postfix_query[i].type == OPERATOR) {
			stack_size-= (postfix_query[i].num_of_operands-1);	
		} else {
		 	error("node type is neither OPERATOR[%d] nor OPERAND[%d] :[%d]",
						            OPERATOR,OPERAND,postfix_query[i].type);
		}
		
		if ( stack_size==1 ){
			safety_stack_size = i;
		} else if ( stack_size<1 ) {
			error("stack_size(%d) hit zero",stack_size);
			break;
		}

		DEBUG(" for loop (%d) stack_size(%d) safety_stack_size(%d)",
				i , stack_size, safety_stack_size);
	}

	return safety_stack_size+1;
} */

static int 
QPP_parse(void* word_db, char infix[],int max_infix_size, QueryNode postfix[], int maxNode)
{
	QueryNode quNode;
	char tmpQueryStr[STRING_SIZE];
	int nRet = 0;
	TokenObj tkObj;
	StateObj stObj;

	DEBUG("infix query: [%s]",infix);

	strncpy(tmpQueryStr,infix,STRING_SIZE-1);
	tmpQueryStr[STRING_SIZE-1] = '\0';

	DEBUG("before tokenizer set string");
	tk_setString(&tkObj,tmpQueryStr);

	stk_init(&(stObj.postfixStack));
	stk_init(&(stObj.operatorStack));
	stObj.searchField = -1;		// search all field
	stObj.nextTurn = TURN_OPERAND;
	stObj.posWithinPhrase = FALSE;
	stObj.numPhraseOperand = 0;
	stObj.truncated = 0;
	stObj.virtualfield = 0;
	stObj.virtualfield_morpid = 20;
	stObj.natural_search = 0;

	quNode.original_word[0] = '\0'; // 초기화

	DEBUG("before entering while loop");
	while (1) {
		nRet = tk_getNextToken(&tkObj,&quNode,MAX_ORIGINAL_WORD_LEN);
		DEBUG("got next token nRet [%d], token_string[%s]",nRet, quNode.original_word);
		
		// error
		if (nRet == END_OF_STRING)
			break;
		else if (nRet == TOKEN_OVERFLOW)
			continue;		// FIXME warn and push the token to stack..
		else if (nRet == 0) 
			continue;

		// right token
		switch (nRet) {
			case TOKEN_STAR:
					nRet = pushStarredWord(word_db, &stObj,&quNode);
					break;
			case TOKEN_NUMERIC:
					nRet = pushNumeric(&stObj,&quNode);
					break;
			case TOKEN_OPERATOR:
					nRet = pushOperator(&stObj,&quNode);				
					break;
			default:	// case nRet < 0
					nRet = pushExtendedOperand(word_db, &stObj,&quNode);
					break;
		}
		
		if (nRet < 0)
			break;
	}

	DEBUG("stack postfix index is [%d]",stObj.postfixStack.index);
	
	// parse된 데이터가 있고, 
	// operator로 끝나서 다음이 operand차례이면
	// fake operand를 집어넣는다.
	if (stObj.postfixStack.index > 0 && stObj.nextTurn ==TURN_OPERAND) {
		DEBUG("pushing fake operand because stack ended with operator");
		pushFakeOperand(&stObj);
	}

	nRet = stk_moveTillBottom(&(stObj.operatorStack),&(stObj.postfixStack));
	DEBUG("stk_moveTillBottom nRet is (%d)",nRet);

	nRet = stk_copyQueryNodes(&(stObj.postfixStack),maxNode,postfix);
	DEBUG("stk_copyQueryNodes nRet is (%d)",nRet);

	if (nRet == QPP_QUERY_NODE_OVERFLOW) {
		error("query node overflow. maxNode:%d", maxNode);
		return QPP_QUERY_NODE_OVERFLOW;
	}	

	/*
	if (nRet == QPP_QUERY_NODE_OVERFLOW) {
		DEBUG("OVERFLOW handling");
		return handle_query_overflow(maxNode, postfix);
	}*/

	return stObj.postfixStack.index;
}

static void set_default_querynode(configValue v)
{
	if (strcmp(v.argument[0],"QPP_OP_AND") == 0) {
		m_defaultOperatorNode.operator = QPP_OP_AND;
	}
	else if (strcmp(v.argument[0],"QPP_OP_OR") == 0) {
		m_defaultOperatorNode.operator = QPP_OP_OR;
	}
	else{
		warn("default operator is not set properly.");
		warn("setting it QPP_OP_FUZZY.");
		m_defaultOperatorNode.operator = QPP_OP_FUZZY;
	}
}

static void setFakeOperand() {
	m_fakeQueryNode.operator = QPP_OP_OPERAND;
	m_fakeQueryNode.word_st.id= (uint32_t)0;
	m_fakeQueryNode.weight = (float)0;
	m_fakeQueryNode.word_st.string[0] = '\0';
}

static void initPrecedence() {
	// everything after parenthesis must be pushed
	mPrecedence[QPP_OP_BEG_PAREN] = 0;	

	mPrecedence[QPP_OP_FIELD] = 11;
	mPrecedence[QPP_OP_VIRTUAL_FIELD] = 11;
	mPrecedence[QPP_OP_NOT] = 10;
		
	mPrecedence[QPP_OP_WITHIN] = 8;
	mPrecedence[QPP_OP_PARA] = 8;

	mPrecedence[QPP_OP_AND] = 6;

	mPrecedence[QPP_OP_FUZZY] = 4;

	mPrecedence[QPP_OP_OR] = 2;
}
static int pushZeroWordid(StateObj *pStObj) {
	if (pStObj->nextTurn == TURN_OPERAND &&
			pStObj->posWithinPhrase == FALSE) {
		pStObj->nextTurn = TURN_BINARY_OPERATOR;
		return stk_push(&(pStObj->postfixStack),&m_zeroWordidNode);
	}
	return SUCCESS;		
}
static int pushFakeOperand(StateObj *pStObj) {
	if (pStObj->nextTurn == TURN_OPERAND &&
			pStObj->posWithinPhrase == FALSE) {
		pStObj->nextTurn = TURN_BINARY_OPERATOR;
		DEBUG("pushing fake operand to stack");
		return stk_push(&(pStObj->postfixStack),&m_fakeQueryNode);
	}
	return SUCCESS;		
}
static int pushDefaultOperator(StateObj *pStObj) {
	if(pStObj->nextTurn == TURN_BINARY_OPERATOR){
		DEBUG(" nextTurn == TURN_BINARY_OPERATOR");
	}
	if(pStObj->posWithinPhrase == FALSE){
		DEBUG( " posWithinPhrase == FALSE");
	}
	
	if (pStObj->nextTurn == TURN_BINARY_OPERATOR && 
		pStObj->posWithinPhrase == FALSE)
			return pushOperator(pStObj,&m_defaultOperatorNode);
	return SUCCESS;
}

#define DEFAULT_OPERATOR_INSERTION()    {\
                    nRet = pushDefaultOperator(pStObj);\
                    if (nRet < 0)\
                        return nRet;    \
                    }
#define FAKE_OPERAND_INSERTION()		{\
					nRet = pushFakeOperand(pStObj);\
					if (nRet < 0)\
						return nRet;	\
					}

static int pushOperator(StateObj *pStObj,QueryNode *pQuNode){
	int nRet = 0;
	pQuNode->type = OPERATOR;
	
	switch (pQuNode->operator) {
		case QPP_OP_NATURAL_BEGIN:
				pStObj->natural_search = 1;
				DEFAULT_OPERATOR_INSERTION();

				pStObj->nextTurn = TURN_OPERAND;
				break;

		case QPP_OP_NATURAL_END:
				pStObj->natural_search = 0;
				pStObj->nextTurn = TURN_BINARY_OPERATOR;
				break;
				
		case QPP_OP_BEG_PHRASE:		
				pStObj->numPhraseOperand = 0;
				DEFAULT_OPERATOR_INSERTION();

				pStObj->nextTurn = TURN_OPERAND;
				pStObj->posWithinPhrase = TRUE;
				break;
		case QPP_OP_END_PHRASE:		

				//susia insert : Empty "" fill fake operand
				if (pStObj->numPhraseOperand == 0) {
					stk_push(&(pStObj->postfixStack),&m_fakeQueryNode);
				}
				
				DEBUG("num operand %d",pStObj->numPhraseOperand);
				
				pStObj->nextTurn = TURN_BINARY_OPERATOR;
				pStObj->posWithinPhrase = FALSE;
				nRet = pushPhrase(pStObj);
				break;
		case QPP_OP_BEG_PAREN:		
				DEFAULT_OPERATOR_INSERTION();

				pStObj->nextTurn = TURN_OPERAND;
				nRet = stk_push(&(pStObj->operatorStack),pQuNode);
				break;
		case QPP_OP_END_PAREN:		
				FAKE_OPERAND_INSERTION();
				pStObj->nextTurn = TURN_BINARY_OPERATOR;
				nRet = stk_moveTillParen(&(pStObj->operatorStack),&(pStObj->postfixStack));
				if (nRet == FIELD_CLEAR) {
					pStObj->searchField = -1;
					pStObj->virtualfield = 0;
				}
				break;
		case QPP_OP_FIELD:
				DEFAULT_OPERATOR_INSERTION();

				pStObj->virtualfield = 0;
				pStObj->searchField = pQuNode->opParam;
				nRet = pushGenericOperator(pStObj,pQuNode);
				pStObj->nextTurn = TURN_OPERAND;	
				break;
		case QPP_OP_VIRTUAL_FIELD:
				DEFAULT_OPERATOR_INSERTION();

				pStObj->virtualfield = pQuNode->opParam;
				nRet = pushGenericOperator(pStObj,pQuNode);
				pStObj->nextTurn = TURN_OPERAND;	
				break;
		case QPP_OP_NOT:
				DEFAULT_OPERATOR_INSERTION();

				pStObj->nextTurn = TURN_OPERAND;
				nRet = pushGenericOperator(pStObj,pQuNode);
		default:				
///		case QPP_OP_AND:		// falls through
///		case QPP_OP_OR:			//  ""     ""
///		case QPP_OP_NOT: 		//  ""     ""
///		case QPP_OP_PARA:		//  ""     ""
///		case QPP_OP_WITHIN:		//  ""     ""
///		case QPP_OP_FUZZY:		//  ""     ""
				
				// get rid of successive binary operator
				// like "man AND & AND OR human" case
				if (pStObj->nextTurn == TURN_OPERAND){
					return SUCCESS;
				}

				pStObj->nextTurn = TURN_OPERAND;
				nRet = pushGenericOperator(pStObj,pQuNode);
			break;
	}
	
	DEBUG("pushOperator [%d] ret is [%d]",pQuNode->operator ,nRet);
	
	return nRet;
}
static int is_left_end_bigram_truncation(char word[])
{
	if (word[0] == '<') return TRUE;

	return FALSE;
}

static int is_right_end_bigram_truncation(char word[])
{
	int len = strlen(word);

	DEBUG("word:[%s], len:%d", word, len);
	if (word[len-1] == '>') return TRUE;

	return FALSE;
}

static int is_both_end_bigram_truncation(char word[])
{
	if (is_left_end_bigram_truncation(word) == TRUE &&
			is_right_end_bigram_truncation(word) == TRUE)
		return TRUE;

	return FALSE;
}

/* XXX: reduce to MAX_QPP_INDEXED_WORDS */
#define FIRST_HALF (int)(MAX_QPP_INDEXED_WORDS/2)
#define LAST_HALF (int)(MAX_QPP_INDEXED_WORDS/2)
static int reduce_index_words(index_word_t indexwords[], int nelm)
{
	memmove(indexwords+FIRST_HALF, indexwords + nelm - LAST_HALF, 
			LAST_HALF * sizeof(index_word_t));
	return FIRST_HALF + LAST_HALF;
}

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

#define ENOUGH_INDEXWORD_NUM (MAX_QPP_INDEXED_WORDS * 2)
static int pushExtendedOperand(void* word_db, StateObj *pStObj,QueryNode *pQuNode) {
	QueryNode qnode;
	int nRet = 0;
	int nMorpheme = 0;
	int i = 0;

	index_word_extractor_t *extractor=NULL;
	index_word_t indexwords[ENOUGH_INDEXWORD_NUM];
	int morp_id=0, indexwordnum=0;

	nRet = pushDefaultOperator(pStObj);
	DEBUG(" nRet of pushDefaultOperator is [%d] ",nRet);
	if ( nRet < 0 ) return nRet;

	if (is_both_end_bigram_truncation(pQuNode->original_word) == TRUE) {
		int len = strlen(pQuNode->original_word);
		pQuNode->original_word[len-1] = '\0';
		/* XXX: moving including NULL */
		memmove(pQuNode->original_word, pQuNode->original_word+1, len-1);
		return pushBothEndBigram(word_db, pStObj, pQuNode);
	}
	else if (is_left_end_bigram_truncation(pQuNode->original_word) == TRUE) {
		int len = strlen(pQuNode->original_word);
		/* XXX: moving including NULL */
		memmove(pQuNode->original_word, pQuNode->original_word+1, len);
		return pushLeftEndBigram(word_db, pStObj, pQuNode);
	}
	else if (is_right_end_bigram_truncation(pQuNode->original_word) == TRUE) {
		int len = strlen(pQuNode->original_word);
		pQuNode->original_word[len-1] = '\0';
		DEBUG("original_word for word> : %s", pQuNode->original_word);
		return pushRightEndBigram(word_db, pStObj, pQuNode);
	}

	morp_id = mQppMorpAnalyzerId[pStObj->searchField]; 

	debug("morp_id[%d], mMorpIdForPhrase[%d], virtualfield_morpid[%d]",
	       morp_id, mMorpIdForPhrase, pStObj->virtualfield_morpid);

	if (pStObj->posWithinPhrase == TRUE) {
	    /* 구문검색시 무조건 바이그램을 쓰지 않도록 수정 */
		morp_id = mMorpIdForPhrase;
	}

	if (pStObj->virtualfield != 0) {
		morp_id = pStObj->virtualfield_morpid;
	}

	extractor = sb_run_new_index_word_extractor(morp_id);
	if (extractor == NULL || extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		error("cannot create index_word_extractor: %p", extractor);
		return FAIL;
	}

	//sb_run_index_word_extractor_set_text(extractor, pQuNode->word_st.string);
	info("original_word:%s", pQuNode->original_word);
	strncpy(pQuNode->word_st.string, pQuNode->original_word, MAX_WORD_LEN-1);
	pQuNode->word_st.string[MAX_WORD_LEN-1] = '\0';
    cut_string(pQuNode->word_st.string, MAX_WORD_LEN-2);

	sb_run_index_word_extractor_set_text(extractor, pQuNode->original_word);

	//XXX: get_index_words should be called in a loop
	//     (more than MAX_QPP_INDEXED_WORDS indexwords can be returned)
	nRet = sb_run_get_index_words(extractor, indexwords, ENOUGH_INDEXWORD_NUM);
	if (nRet < 0) {
		error("error while getting index_words. nRet[%d]", nRet);
		sb_run_delete_index_word_extractor(extractor);
		return FAIL;
	}
	indexwordnum = nRet;

	if (indexwordnum > MAX_QPP_INDEXED_WORDS) {
		indexwordnum = reduce_index_words(indexwords, indexwordnum);
	}

	nMorpheme = indexwordnum;
	sb_run_delete_index_word_extractor(extractor);

	
	// daum_koma를 사용하는 경우 특수처리
	// 이놈은 boolean query를 리턴한다
	if ( morp_id == 16 ) {
		char buf[STRING_SIZE];
		struct daum_tree_node* tree = parse_daum_query(indexwords, nMorpheme);
		if ( tree == NULL ) {
			error("failed parsing daum koma query: %s", pQuNode->original_word);
			return FAIL;
		}

		info("original query: [%s]", pQuNode->original_word);
		info("parsed daum query: [%s]", print_daum_tree(tree, buf, sizeof(buf)));

		if ( push_daum_tree(word_db, pStObj, tree) != SUCCESS ) {
			error("push_daum_tree failed");
			destroy_daum_tree(tree);
			return FAIL;
		}

		// 단어가 등장하면 최소한 { 가 } 와 같이 3개가 나온다
		if ( nMorpheme >= 3 && pStObj->posWithinPhrase == TRUE ) {
			pStObj->numPhraseOperand++;
		}

		destroy_daum_tree(tree);
		pStObj->nextTurn = TURN_BINARY_OPERATOR;

		return SUCCESS;
	}
	else if (nMorpheme > 0) {
		for (i=0; i<nMorpheme; i++) {
			strncpy(qnode.word_st.string, indexwords[i].word, MAX_WORD_LEN-1);
			qnode.word_st.string[MAX_WORD_LEN-1] = '\0';

            cut_string(qnode.word_st.string, MAX_WORD_LEN-2);
			INFO("qnode[%d] word:%s", i, qnode.word_st.string);

			nRet = pushOperand(word_db, pStObj, &qnode);
			if (nRet < 0) {
				error("failed to push operand for word[%s]",
										qnode.word_st.string);
				return FAIL;
			}
		}

		if (pStObj->posWithinPhrase == TRUE) {
			pStObj->numPhraseOperand++;
		}

		/* 하나의 토큰에서 두개이상의 색인어가 추출되는 경우 within 연산으로 처리 */
	    if (nMorpheme > 1) {
			qnode.type = OPERATOR;
			qnode.operator = QPP_OP_WITHIN;
			qnode.opParam = 0; /* within 0 */
			qnode.num_of_operands = nMorpheme;
			nRet = stk_push(&(pStObj->postfixStack),&qnode);
		    if (nRet < 0)
		    	return nRet;
		}
	} else {
		/* 색인어가 없는 경우 바로 리턴한다. */
		return SUCCESS;
	}

	/*
	 * 형태소분석 : 10 <= morp_id < 20
	 * 바이그램   : 20 <= morp_id < 30
     */
	/* 형태소분석 결과의 버그를 피해가는 코드는 사용하지 않는다.
	 * 2007-09-18 김정겸 */
#if 0
	/*
	if (nMorpheme == 0 && (morp_id >= 10 && morp_id < 20)) {
		// 형태소 분석 결과가 없으면 원래 단어로 검색
		nRet = pushOperand(pStObj, pQuNode);
		if (nRet < 0) {
			error("error while pushing original wordp[%s] into stack",
												pQuNode->word_st.string);
			return nRet;
		}
	} else
	*/
	if (nMorpheme == 1 && 
				(morp_id >= 10 && morp_id < 20)) {
		/* 형태소 분석 결과 단어와 입력단어가 다르면 입력단어를 OR 검색으로 추가 */
		if ( strncmp(pQuNode->original_word, indexwords[0].word, MAX_WORD_LEN) != 0) {
			// or with original word
			nRet = pushOperand(word_db, pStObj, pQuNode);
			if (nRet < 0) {
				error("error while pushing original wordp[%s] into stack",
													pQuNode->word_st.string);
				return nRet;
			}

			qnode.type = OPERATOR;
			qnode.operator = QPP_OP_OR;
			qnode.num_of_operands = 2;
			nRet = stk_push(&(pStObj->postfixStack), &qnode);
			if (nRet < 0) {
				error("error while pushing original word into stack");
				return nRet;
			}

		}
	}
	else if (nMorpheme >= 2 && 
				(morp_id >= 10 && morp_id < 20)) {
		/* 형태소 분석 결과 단어와 입력단어가 다르면 입력단어를 OR 검색으로 추가 */
		// or with original word
		nRet = pushOperand(word_db, pStObj, pQuNode);
		if (nRet < 0) {
			error("error while pushing original word[%s] into stack",
												pQuNode->word_st.string);
			return nRet;
		}

		qnode.type = OPERATOR;
		qnode.operator = QPP_OP_OR;
		qnode.num_of_operands = 2;
		nRet = stk_push(&(pStObj->postfixStack), &qnode);
		if (nRet < 0) {
			error("error while pushing original word into stack");
			return nRet;
		}
	}
#endif

	pStObj->nextTurn = TURN_BINARY_OPERATOR;

	return SUCCESS;
}

static void makeUpperLetter(QueryNode *pQuNode) {
	int i = 0;

	for (i = 0; pQuNode->word_st.string[i] != '\0'; i++) {
		if (pQuNode->word_st.string[i] >= 'a' && pQuNode->word_st.string[i] <= 'z' ){
			pQuNode->word_st.string[i] = pQuNode->word_st.string[i] - ('a' - 'A');
		}
	}
}

int pushOperand(void* word_db, StateObj *pStObj,QueryNode *pQuNode) {
	int ret=0;
/*	LWord retWord={"",0,-1,0};*/
/*	word_t word={"",{0,0,0}};*/
	
	makeUpperLetter(pQuNode);
	pQuNode->word_st.string[MAX_WORD_LEN-1]='\0';

	pQuNode->type = OPERAND;
	
	if (pStObj->searchField == -1) {
		pQuNode->field = mDefaultSearchField;
	} else
	if ( (pStObj->posWithinPhrase == TRUE)
		&& (mDefaultSearchField & (1L << pStObj->searchField)) )
	{
		pQuNode->field = (1L << (pStObj->searchField+1));
		if ((pQuNode->field & mDefaultPhraseField) == 0)
		{
			warn("PHRASE operator in an invalid field[%d]. Cannot find matching BIGRAM field.",
					pStObj->searchField);
			pQuNode->field = (1L << (pStObj->searchField));
		}
	}
	else {
		pQuNode->field = (1L << pStObj->searchField);
	}

	if (pStObj->searchField == -1 && pStObj->posWithinPhrase == TRUE) {
		pQuNode->field = mDefaultPhraseField;
	}
	else if (pStObj->searchField == -1 && pStObj->truncated == 1) {
//		pQuNode->field = 0xffffffff;
		pStObj->truncated = 0;
	}

	if (pStObj->virtualfield != 0) {
		pQuNode->field = pStObj->virtualfield;
	}

	pQuNode->opParam = pStObj->searchField; /* obsolete ? */
	pQuNode->weight = 1;

	ret = sb_run_get_word(word_db, &(pQuNode->word_st));
	DEBUG("ret(by sb_run_get_word): %d", ret);

	if (ret == WORD_NOT_REGISTERED) {
		INFO("user entered unknown word: [%s]",pQuNode->word_st.string);
		INFO("ret wordid is [%u]",pQuNode->word_st.id);
		pQuNode->word_st.id = 0;
	}
	else if (ret < 0) {
		error("can't get wordid. error[%d]",ret);
		pQuNode->word_st.string[0]= '\0';
		pQuNode->word_st.id= 0;
	}else{
/*		pQuNode->word_st = retWord;*/
		DEBUG("su_run_get_word ret:(%d) , word[%s], wordid:(%d)",
				ret, pQuNode->word_st.string, pQuNode->word_st.id);
	}

	if ( pStObj->natural_search == 1
			&& ret == WORD_NOT_REGISTERED) {
		pQuNode->opParam = -10;
	}

	return stk_push(&pStObj->postfixStack,pQuNode);
}
static int pushStarHackedOperand(void* word_db, StateObj *pStObj,QueryNode *pQuNode, char *original_word) {
	int ret=0;
/*	LWord retWord={"",0,-1,0};*/
/*	word_t word={"",{0,0,0}};*/
	
	makeUpperLetter(pQuNode);
	pQuNode->word_st.string[MAX_WORD_LEN-1]='\0';

	pQuNode->type = OPERAND;
	
	if (pStObj->searchField == -1) {
		pQuNode->field = mDefaultSearchField;
	}
	else {
		pQuNode->field = (1L << pStObj->searchField);
	}

	if (pStObj->searchField == -1 && pStObj->posWithinPhrase == TRUE) {
		pQuNode->field = mDefaultPhraseField;
	}
	else if (pStObj->searchField == -1 && pStObj->truncated == 1) {
//		pQuNode->field = 0xffffffff;
		pStObj->truncated = 0;
	}

	if (pStObj->virtualfield != 0) {
		pQuNode->field = pStObj->virtualfield;
	}

	pQuNode->opParam = pStObj->searchField; /* obsolete ? */
	pQuNode->weight = 1;

	ret = sb_run_get_word(word_db, &(pQuNode->word_st));

	if (ret == WORD_NOT_REGISTERED) {
		INFO("user entered unknown word: [%s]",pQuNode->word_st.string);
		INFO("ret wordid is [%u]",pQuNode->word_st.id);
		pQuNode->word_st.id = 0;
	}
	else if (ret < 0) {
		error("can't get wordid. error[%d]",ret);
		pQuNode->word_st.string[0]= '\0';
		pQuNode->word_st.id= 0;
	}else{
/*		pQuNode->word_st = retWord;*/
		DEBUG("su_run_get_word ret:(%d) , word[%s], wordid:(%d)",
				ret, pQuNode->word_st.string, pQuNode->word_st.id);
	}

	if ( pStObj->natural_search == 1 && 
			ret == WORD_NOT_REGISTERED) {
		pQuNode->opParam = -10;
	}

	// strncpy(pQuNode->word_st.string, original_word, MAX_WORD_LEN-1);
	// pQuNode->word_st.string[MAX_WORD_LEN-1]='\0';
	
	return stk_push(&pStObj->postfixStack,pQuNode);
}

static int pushOperandEmptyWord(void* word_db, StateObj *pStObj,QueryNode *pQuNode) {
	int ret=0;
/*	LWord retWord={"",0,-1,0};*/
/*	word_t word={"",{0,0,0}};*/
	
	makeUpperLetter(pQuNode);
	pQuNode->word_st.string[MAX_WORD_LEN-1]='\0';

	pQuNode->type = OPERAND;
	
	if (pStObj->searchField == -1) {
		pQuNode->field = mDefaultSearchField;
	}
	else {
		pQuNode->field = (1L << pStObj->searchField);
	}

	if (pStObj->searchField == -1 && pStObj->posWithinPhrase == TRUE) {
		pQuNode->field = mDefaultPhraseField;
	}
	else if (pStObj->searchField == -1 && pStObj->truncated == 1) {
//		pQuNode->field = 0xffffffff;
		pStObj->truncated = 0;
	}

	if (pStObj->virtualfield != 0) {
		pQuNode->field = pStObj->virtualfield;
	}

	pQuNode->opParam = pStObj->searchField; /* obsolete ? */
	pQuNode->weight = 1;

	ret = sb_run_get_word(word_db, &(pQuNode->word_st));

	if (ret == WORD_NOT_REGISTERED) {
		INFO("user entered unknown word: [%s]",pQuNode->word_st.string);
		INFO("ret wordid is [%u]",pQuNode->word_st.id);
		pQuNode->word_st.id = 0;
	}
	else if (ret < 0) {
		error("can't get wordid. error[%d]",ret);
		pQuNode->word_st.string[0]= '\0';
		pQuNode->word_st.id= 0;
	}else{
/*		pQuNode->word_st = retWord;*/
		DEBUG("su_run_get_word ret:(%d) , word[%s], wordid:(%d)",
				ret, pQuNode->word_st.string, pQuNode->word_st.id);
	}

	if ( pStObj->natural_search == 1 &&
			ret == WORD_NOT_REGISTERED) {
		pQuNode->opParam = -10;
	}

	// pQuNode->word_st.string[0] = '\0';

	return stk_push(&pStObj->postfixStack,pQuNode);
}

static int pushNumeric(StateObj *pStObj,QueryNode *pQuNode) {
	pQuNode->operator = QPP_OP_NUMERIC;
	pQuNode->opParam = pStObj->searchField;
	pStObj->searchField = -1;
	stk_removeLast(&(pStObj->operatorStack));

	pStObj->nextTurn = TURN_BINARY_OPERATOR;

	return stk_push(&(pStObj->postfixStack),pQuNode);
}
static int pushPhrase(StateObj *pStObj){
    QueryNode quNode;
	
	quNode.type = OPERATOR;
    quNode.operator = QPP_OP_PHRASE;
	quNode.opParam = 1;
    quNode.num_of_operands = pStObj->numPhraseOperand;

    return stk_push(&(pStObj->postfixStack),&quNode);
}

static int pushGenericOperator(StateObj *pStObj,QueryNode *pQuNode){
	QueryNode peekNode;
	int nRet = SUCCESS;

	while (1) {
		nRet = stk_peek(&(pStObj->operatorStack),&peekNode);
		if (nRet == STACK_UNDERFLOW){
			nRet = stk_push(&(pStObj->operatorStack),pQuNode);
			return nRet;
		}
		else if (mPrecedence[(uint16_t)peekNode.operator] >=
				mPrecedence[(uint16_t)pQuNode->operator]) {
			if (peekNode.operator == QPP_OP_FIELD) {
				stk_removeLast(&(pStObj->operatorStack));
				pStObj->searchField = -1;
				pStObj->virtualfield = 0;
			}
			else if (peekNode.operator == QPP_OP_VIRTUAL_FIELD) {
				stk_removeLast(&(pStObj->operatorStack));
				pStObj->searchField = -1;
				pStObj->virtualfield = 0;
			}
			else	
				nRet = stk_move(&(pStObj->operatorStack),&(pStObj->postfixStack));
		}
		else if (mPrecedence[(uint16_t)peekNode.operator] <
				mPrecedence[(uint16_t)pQuNode->operator]) {
			nRet = stk_push(&(pStObj->operatorStack),pQuNode);
			return nRet;
		}

		if (nRet < 0)
			break;
	}

	return nRet;
}

#define IS_KOREAN(c)	((c >= 0xb0) && (c <= 0xc8) )
// XXX: pos가 0이면 전방, pos가 2이면 후방이다.
static void bigram_word_copy(char *dest, char *src, int max, int pos) 
{

	if(IS_KOREAN((unsigned char)src[0]))
	{
		dest[0]=src[pos];
		dest[1]=src[pos+1];
		dest[2]='\0';
	}
	else	
		strncpy(dest, src, max-1);

	dest[max-1]='\0';
}

static int pushRightEndBigram(void* word_db, StateObj *state, QueryNode *input_qnode)
{
	index_word_extractor_t *extractor=NULL;
	int num_of_words=0, i=0, rv=0;
	index_word_t indexwords[MAX_QPP_INDEXED_WORDS];
	QueryNode within, qnode;
	char tmp_string[MAX_WORD_LEN];

	strncpy(input_qnode->word_st.string, input_qnode->original_word, MAX_WORD_LEN-1);
	input_qnode->word_st.string[MAX_WORD_LEN-1] = '\0';
	extractor = sb_run_new_index_word_extractor(20);
	if (extractor == NULL || extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("error while allocating index word extractor: %p", extractor);
		return FAIL;
	}

	sb_run_index_word_extractor_set_text(extractor, input_qnode->original_word);

	num_of_words = sb_run_get_index_words(extractor, indexwords, MAX_QPP_INDEXED_WORDS);
	sb_run_delete_index_word_extractor(extractor);

	if (num_of_words <= 0) {
		INFO("num_of_words 0 for word:%s", input_qnode->original_word);
		pushZeroWordid(state);
		return SUCCESS;
	}

	bigram_word_copy(tmp_string, indexwords[num_of_words-1].word,
					MAX_WORD_LEN, 2);
	snprintf(indexwords[num_of_words].word, MAX_WORD_LEN, "%s%s", tmp_string, "\\>");
	indexwords[num_of_words].len = strlen(indexwords[num_of_words].word);

	for (i=0; i<num_of_words+1; i++) {
		strncpy(qnode.word_st.string, indexwords[i].word, MAX_WORD_LEN-1);
		qnode.word_st.string[MAX_WORD_LEN-1] = '\0';

		state->truncated = 1; // truncated is unset within pushOperand

		if (i==0)
			pushStarHackedOperand(word_db, state, &qnode, input_qnode->word_st.string);
		else
			pushOperandEmptyWord(word_db, state, &qnode);
	}

	within.type = OPERATOR;
	within.operator = QPP_OP_WITHIN;
	within.num_of_operands = num_of_words+1; // \> added
	within.opParam = 0;

	rv = stk_push(&(state->postfixStack), &within);
	if (rv < 0) {
		error("error while pushing within operator into stack");
		return FAIL;
	}

	if (state->posWithinPhrase == TRUE) {
		state->numPhraseOperand++;
	}

	state->nextTurn = TURN_BINARY_OPERATOR;
	return SUCCESS;
}

static int pushLeftEndBigram(void* word_db, StateObj *state, QueryNode *input_qnode)
{
	index_word_extractor_t *extractor=NULL;
	index_word_t indexwords[MAX_QPP_INDEXED_WORDS+1];/* dirty hack */
	QueryNode within, qnode;
	int num_of_words=0, i=0, rv=0;
	char tmp_string[MAX_WORD_LEN];

	strncpy(input_qnode->word_st.string, input_qnode->original_word, MAX_WORD_LEN-1);
	input_qnode->word_st.string[MAX_WORD_LEN-1] = '\0';

	extractor = sb_run_new_index_word_extractor(20);
	if (extractor == NULL || extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("error while allocating index word extractor");
		return FAIL;
	}

	sb_run_index_word_extractor_set_text(extractor, input_qnode->original_word);

	num_of_words = sb_run_get_index_words(extractor, indexwords, MAX_QPP_INDEXED_WORDS);
	sb_run_delete_index_word_extractor(extractor);

	if (num_of_words <= 0) {
		INFO("num_of_words 0 for word:%s", input_qnode->original_word);
		pushZeroWordid(state);
		return SUCCESS;
	}

	bigram_word_copy(tmp_string, indexwords[0].word, MAX_WORD_LEN, 0);
	snprintf(indexwords[num_of_words].word, MAX_WORD_LEN, "%s%s", "\\<", tmp_string);
	indexwords[num_of_words].len = strlen(indexwords[num_of_words].word);

	for (i=0; i<num_of_words+1; i++) {
		strncpy(qnode.word_st.string, indexwords[i].word, MAX_WORD_LEN-1);
		qnode.word_st.string[MAX_WORD_LEN-1] = '\0';

		state->truncated = 1; // truncated is unset within pushOperand
		
		if (i==0) 
			pushStarHackedOperand(word_db, state, &qnode,input_qnode->word_st.string);
		else
			pushOperandEmptyWord(word_db, state, &qnode);
	}

	within.type = OPERATOR;
	within.operator = QPP_OP_WITHIN;
	within.num_of_operands = num_of_words+1; // \< added
	within.opParam = 0;

	rv = stk_push(&(state->postfixStack), &within);
	if (rv < 0) {
		error("error while pushing within operator into stack");
		return FAIL;
	}

	if (state->posWithinPhrase == TRUE) {
		state->numPhraseOperand++;
	}

	state->nextTurn = TURN_BINARY_OPERATOR;
	return SUCCESS;
}

static int pushBothEndBigram(void* word_db, StateObj *state, QueryNode *input_qnode)
{
	index_word_extractor_t *extractor=NULL;
	index_word_t indexwords[MAX_QPP_INDEXED_WORDS+2];/* dirty hack */
	QueryNode within, qnode;
	int num_of_words=0, i=0, rv=0;
	char tmp_string[MAX_WORD_LEN];

	strncpy(input_qnode->word_st.string, input_qnode->original_word, MAX_WORD_LEN-1);
	input_qnode->word_st.string[MAX_WORD_LEN-1] = '\0';
	DEBUG("original_word:%s, MAX_QPP_INDEXED_WORDS:%d",
			input_qnode->original_word, MAX_QPP_INDEXED_WORDS);

	extractor = sb_run_new_index_word_extractor(20);
	if (extractor == NULL || extractor == (index_word_extractor_t*)MINUS_DECLINE) {
		crit("error while allocating index word extractor");
		return FAIL;
	}

	sb_run_index_word_extractor_set_text(extractor, input_qnode->original_word);

	num_of_words = sb_run_get_index_words(extractor, indexwords, MAX_QPP_INDEXED_WORDS);
	sb_run_delete_index_word_extractor(extractor);

	if (num_of_words <= 0) {
		INFO("num_of_words 0 for word:%s", input_qnode->original_word);
		pushZeroWordid(state);
		return SUCCESS;
	}

	DEBUG("num_of_words:%d", num_of_words);

	bigram_word_copy(tmp_string, indexwords[0].word, MAX_WORD_LEN, 0);
	snprintf(indexwords[num_of_words].word, MAX_WORD_LEN, "%s%s", "\\<", tmp_string);
	indexwords[num_of_words].len = strlen(indexwords[num_of_words].word);

	bigram_word_copy(tmp_string, indexwords[num_of_words-1].word,
					MAX_WORD_LEN, 2);
	snprintf(indexwords[num_of_words+1].word, MAX_WORD_LEN, "%s%s", tmp_string, "\\>");
	indexwords[num_of_words+1].len = strlen(indexwords[num_of_words+1].word);

	for (i=0; i<num_of_words+2; i++) {
		strncpy(qnode.word_st.string, indexwords[i].word, MAX_WORD_LEN-1);
		qnode.word_st.string[MAX_WORD_LEN-1] = '\0';

		state->truncated = 1; // truncated is unset within pushOperand
		
		if (i==0) 
			pushStarHackedOperand(word_db, state, &qnode,input_qnode->word_st.string);
		else
			pushOperandEmptyWord(word_db, state, &qnode);
	}

	within.type = OPERATOR;
	within.operator = QPP_OP_WITHIN;
	within.num_of_operands = num_of_words+2; // \< , \> added
	within.opParam = 0;

	rv = stk_push(&(state->postfixStack), &within);
	if (rv < 0) {
		error("error while pushing within operator into stack");
		return FAIL;
	}

	if (state->posWithinPhrase == TRUE) {
		state->numPhraseOperand++;
	}

	state->nextTurn = TURN_BINARY_OPERATOR;
	return SUCCESS;
}



#ifdef BIGRAM_TRUNCATION_SEARCH
static int pushStarredWord(void* word_db, StateObj *state, QueryNode *input_qnode)
{
	if (input_qnode->opParam == STAR_BEGIN) {
		return pushRightEndBigram(word_db, state, input_qnode);
	}
	else if (input_qnode->opParam == STAR_END) {
		return pushLeftEndBigram(word_db, state, input_qnode);
	}
	else {
		crit("opParam[%d] is not STAR_BEGIN[%d] and not STAR_END[%d] .. then what???",
				input_qnode->opParam, STAR_BEGIN, STAR_END);
		return FAIL;
	}
}
#else
/* XXX: lexicon supported version */
static int pushStarredWord(void* word_db, StateObj *pStObj, QueryNode *querynode)
{
	warn("truncation search is disabled from 2005/02/02");
	return FAIL;
}
#endif

void set_def_morp_analyzer(configValue v)
{
	char id=0;

	id = atoi(v.argument[0]);
	mQppMorpAnalyzerId[-1] = id;

	INFO("Setting default morpheme analyzer : %d",id);
}

static void get_defaultsearchfield(configValue v)
{
    int i=0;
    uint32_t fieldid=0;

    mDefaultSearchField=0L;

    for (i=0; i<v.argNum; i++) {
		WARN("arg %d: %s",i,v.argument[i]);
    }

    for (i=0; i<v.argNum; i++) {

        fieldid = atol(v.argument[i]);
        mDefaultSearchField |= (1L << fieldid);
    }

    WARN("mDefaultSearchField:%x",mDefaultSearchField);
}

static void get_defaultphrasefield(configValue v)
{
    int i=0;
    uint32_t fieldid=0;

    mDefaultPhraseField=0L;

    for (i=0; i<v.argNum; i++) {
		WARN("arg %d: %s",i,v.argument[i]);
    }

    for (i=0; i<v.argNum; i++) {

        fieldid = atol(v.argument[i]);
        mDefaultPhraseField |= (1L << fieldid);
    }

    WARN("mDefaultPhraseField:%x",mDefaultPhraseField);
}

// FIXME QPP_OP_PHRASE have setting BEGIN_OP_PHRASE and END_OP_PHRASE.. 
static config_t config[] = {
	CONFIG_GET("QPP_OP_DEFAULT",set_default_querynode,1,\
						"default operator which is used for blank"),
	CONFIG_GET("QPP_OP_AND",set_op_and,VAR_ARG,"string for AND operator"),
	CONFIG_GET("QPP_OP_OR",set_op_or,VAR_ARG,"string for OR operator"),
	CONFIG_GET("QPP_OP_NOT",set_op_not,VAR_ARG,"string for NOT operator"),
	CONFIG_GET("QPP_OP_PARA",set_op_paragraph,VAR_ARG,"string for NEAR operator"),
	CONFIG_GET("QPP_OP_WITHIN",set_op_within,VAR_ARG,"string for WITHIN operator"),
	CONFIG_GET("QPP_OP_FUZZY",set_op_fuzzy,VAR_ARG,"string for FUZZY operator"),
	CONFIG_GET("QPP_OP_FIELD",set_op_field,VAR_ARG,"string for FIELD operator"),
	CONFIG_GET("QPP_OP_STAR",set_op_star,VAR_ARG,"string for * operator"),
	CONFIG_GET("QPP_OP_PHRASE",set_op_phrase,VAR_ARG,"string for phrase operator"),
	CONFIG_GET("BEGIN_OP_PHRASE",set_begin_op_phrase,1,"one character for check begin op-phrase"),
	CONFIG_GET("END_OP_PHRASE",set_end_op_phrase,1,"one character for check end op-phrase"),

	CONFIG_GET("Field",set_fieldname,VAR_ARG, "string used for field search"),
	CONFIG_GET("VirtualField",set_virtual_fieldname,VAR_ARG, \
					"string used for virtualfield search"),
	CONFIG_GET("DefaultMorpAnalyzer",set_def_morp_analyzer, 1, "Default morpheme analyzer id"),

	CONFIG_GET("DefaultSearchField",get_defaultsearchfield,VAR_ARG,\
				"Fields which is searched when no field is specified"),

	CONFIG_GET("DefaultPhraseField",get_defaultphrasefield,VAR_ARG,\
				"Fields which is searched when phrase search "),
	{NULL}
};

int dummy_suffix_word(char *word, int pass, int nelm, word_t words[])
{
	strncpy(words[0].string, "suffix_word0", MAX_WORD_LEN-1);
	words[0].string[MAX_WORD_LEN-1]='\0';
	words[0].id = 10;

	strncpy(words[1].string, "suffix_word1", MAX_WORD_LEN-1);
	words[1].string[MAX_WORD_LEN-1]='\0';
	words[1].id = 11;

	return 2;
}

int dummy_prefix_word(char *word, int pass, int nelm, word_t words[])
{
	strncpy(words[0].string, "prefix_word0", MAX_WORD_LEN-1);
	words[0].string[MAX_WORD_LEN-1]='\0';
	words[0].id = 10;

	strncpy(words[1].string, "prefix_word1", MAX_WORD_LEN-1);
	words[1].string[MAX_WORD_LEN-1]='\0';
	words[1].id = 11;

	return 2;
}

static void register_hooks(void)
{
	sb_hook_preprocess(QPP_parse,NULL,NULL,HOOK_MIDDLE);
	//sb_hook_print_querynode(QPP_postfixPrint,NULL,NULL,HOOK_MIDDLE);
	//sb_hook_buffer_querynode(QPP_postfixBuffer,NULL,NULL,HOOK_MIDDLE);
	sb_hook_print_querynode(QPP_infixPrint,NULL,NULL,HOOK_MIDDLE);
	sb_hook_buffer_querynode(QPP_infixBuffer,NULL,NULL,HOOK_MIDDLE);
	

/*    sb_hook_get_suffix_word(dummy_suffix_word, NULL, NULL, HOOK_REALLY_LAST);*/
/*    sb_hook_get_prefix_word(dummy_prefix_word, NULL, NULL, HOOK_REALLY_LAST);*/
}

module qpp_module = {
    STANDARD_MODULE_STUFF,
    config,					/* config */
    NULL,                   /* registry */
    init,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks,			/* register hook api */
};
