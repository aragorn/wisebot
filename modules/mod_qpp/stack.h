/* $Id$ */
#ifndef _QPP_STACK_H_
#define _QPP_STACK_H_ 1

#include "softbot.h"
#include "mod_api/qpp.h" // XXX: qpp structure 를 쓰므로..(QueryNode)

#define STACK_OVERFLOW	(-1)
#define STACK_UNDERFLOW	(-2)

#define MAX_STACK_LEN	(512)
typedef struct _StackObj{
	QueryNode queryNodes[MAX_STACK_LEN];
	int16_t index;
} Stack;

int stk_init(Stack *pStkObj);
int stk_push(Stack *pStkObj,QueryNode *pQueryNode);
int stk_pop(Stack *pStkObj,QueryNode *pQueryNode);
int stk_peek(Stack *pStkObj,QueryNode *pQueryNode); 
int stk_removeLast(Stack *pStkObj);
int stk_move(Stack *pStkFrom,Stack *pStkTo);

int stk_moveTillParen(Stack *pStkFrom, Stack *pStkTo);
int stk_moveTillBottom(Stack *pStkFrom, Stack *pStkTo);
int stk_copyQueryNodes(Stack *pStkObj,int maxNode,QueryNode *pQueryNode);

QueryNode* stk_getArray(Stack *pStkObj,int16_t *pArrayLen);

// for debugging
void printStkBottomUp(Stack *pStkObj);

#endif
