/* $Id$ */
#include "stack.h"
#include "mod_qpp.h"

int stk_init(Stack *pStkObj) {

	pStkObj->index = 0;
	return SUCCESS;
}

int stk_push(Stack *pStkObj,QueryNode *pQueryNode) {
	int16_t index;

	if (pStkObj->index >= MAX_STACK_LEN)
		return STACK_OVERFLOW;

	index = pStkObj->index;	
	memcpy( &(pStkObj->queryNodes[index]),pQueryNode, sizeof(QueryNode) );

	pStkObj->index++;

	return SUCCESS;
}

int stk_pop(Stack *pStkObj,QueryNode *pQueryNode) {
	int16_t index;

	if (pStkObj->index == 0)
		return STACK_UNDERFLOW;

	pStkObj->index--;
	index = pStkObj->index;

	memcpy(pQueryNode, &(pStkObj->queryNodes[index]), sizeof(QueryNode));
	return SUCCESS;
}

int stk_peek(Stack *pStkObj,QueryNode *pQueryNode) {
	int16_t index;

	index = pStkObj->index;
	if (index == 0)
		return STACK_UNDERFLOW;

	memcpy(pQueryNode, &(pStkObj->queryNodes[index-1]), sizeof(QueryNode));
	return SUCCESS;
}

int stk_removeLast(Stack *pStkObj) {
	int16_t index;

	index = pStkObj->index;
	if (index == 0)
		return STACK_UNDERFLOW;
	pStkObj->index = pStkObj->index - 1;	

	return SUCCESS;
}

int stk_moveTillParen(Stack *pStkFrom, Stack *pStkTo) {
	QueryNode qn;
	int nRet,flag;

	flag=0;
	while (1) {
		nRet = stk_pop(pStkFrom,&qn);
		
		if (nRet < 0)
			return FAIL;
		else if (qn.operator == QPP_OP_BEG_PAREN) 
			break;
		else if (qn.operator == QPP_OP_FIELD || qn.operator == QPP_OP_VIRTUAL_FIELD) {
			flag=1;
			continue;	
		}
		else 
			stk_push(pStkTo,&qn);
	}

	if (flag) {
		return FIELD_CLEAR;
	}

	return SUCCESS;
}

int stk_moveTillBottom(Stack *pStkFrom, Stack *pStkTo) {
	QueryNode qn;
	int nRet;

	while (1) {
		nRet = stk_pop(pStkFrom,&qn);
		
		if (nRet == STACK_UNDERFLOW)
			break;
		else if (qn.operator == QPP_OP_FIELD || qn.operator == QPP_OP_VIRTUAL_FIELD)
			continue;	
		else 
			stk_push(pStkTo,&qn);
	}

	return SUCCESS;
}

int stk_move(Stack *pStkFrom,Stack *pStkTo) {
	QueryNode qn;
	int nRet;

	stk_pop(pStkFrom,&qn);
	nRet = stk_push(pStkTo,&qn);
	return nRet;
}
int stk_copyQueryNodes(Stack *pStkObj,int maxNode,QueryNode *pQueryNode) {
	int i;

	for (i = 0; ; i++) {
		if (i == pStkObj->index) {
			return i;
		}
		else if (i == maxNode){
			return QPP_QUERY_NODE_OVERFLOW;
		}

		memcpy( &(pQueryNode[i]), &(pStkObj->queryNodes[i]), sizeof(QueryNode));
	}
	return SUCCESS;
}

QueryNode* stk_getArray(Stack *pStkObj,int16_t *pArrayLen) {
	*pArrayLen = pStkObj->index;
	return pStkObj->queryNodes;
}

void printStkBottomUp(Stack *pStkObj) {
	int16_t index = 0;
/*	int16_t stkLen = pStkObj->index;*/
	QueryNode tmp;
	char tmpStr[14][32] = {"","QPP_OP_AND","QPP_OP_OR","QPP_OP_NOT",\
						"QPP_OP_NEAR","QPP_OP_WITHIN","QPP_OP_FUZZY","QPP_OP_FIELD",\
						"QPP_OP_TRUNC","QPP_OP_BEG_PHRASE","QPP_OP_END_PHRASE",\
						"QPP_OP_BEG_PAREN","QPP_OP_END_PAREN"};
	
	for (index = 0; index<pStkObj->index; index++) {
		tmp = pStkObj->queryNodes[index];

		if (tmp.operator == QPP_OP_FIELD){
			printf("int16_t: %u\n",tmp.word_st.id);
			continue;
		}
		
		printf("Operator: %s\n",tmpStr[(uint16_t)tmp.operator]);

	}
}
