/* $Id$ */
#include "common_core.h"
#include "qpp.h"

HOOK_STRUCT(
  HOOK_LINK(preprocess)
  HOOK_LINK(print_querynode)
  HOOK_LINK(buffer_querynode)
  HOOK_LINK(buffer_querynode_LG)
)

char gOperatorNames[][21] = {
 "","QPP_OP_AND","QPP_OP_OR","QPP_OP_NOT","QPP_OP_PARA",
 "QPP_OP_WITHIN","QPP_OP_FUZZY","QPP_OP_FIELD","QPP_OP_MORP","QPP_OP_SYN",
 "QPP_OP_FILTER","QPP_OP_PHRASE","QPP_OP_NUMERIC","QPP_OP_VIRTUAL_FIELD"
};

SB_IMPLEMENT_HOOK_RUN_FIRST(int,preprocess, \
		(void* word_db, char infix[],int max_infix_size,QueryNode postfix[],int max_postfix_size), \
		(word_db, infix, max_infix_size, postfix, max_postfix_size), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,print_querynode, \
		(QueryNode aPostfixQuery[],int numNode), \
		(aPostfixQuery,numNode),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_querynode, \
		(char *result, QueryNode aPostfixQuery[],int numNode), \
		(result,aPostfixQuery,numNode),DECLINE)		
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_querynode_LG, \
		(char *result, QueryNode aPostfixQuery[],int numNode), \
		(result,aPostfixQuery,numNode),DECLINE)

