/* $Id$ */
#include "softbot.h"
#include "mod_api/qpp.h"

HOOK_STRUCT(
  HOOK_LINK(preprocess)
  HOOK_LINK(print_querynode)
  HOOK_LINK(buffer_querynode)
  HOOK_LINK(buffer_querynode_LG)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,preprocess, \
		(char infix[],int max_infix_size,QueryNode postfix[],int max_postfix_size), \
		(infix, max_infix_size, postfix, max_postfix_size), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,print_querynode, \
		(FILE *fp, QueryNode aPostfixQuery[],int numNode), \
		(fp,aPostfixQuery,numNode),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_querynode, \
		(char *result, QueryNode aPostfixQuery[],int numNode), \
		(result,aPostfixQuery,numNode),DECLINE)		
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_querynode_LG, \
		(char *result, QueryNode aPostfixQuery[],int numNode), \
		(result,aPostfixQuery,numNode),DECLINE)

