/* $Id$ */
#ifndef _QPP_H_
#define _QPP_H_ 1

#include "softbot.h"
#include "lexicon.h"

#define QPP_QUERY_NODE_OVERFLOW 		(-10)

#define QPP_OP_AND			(1)
#define QPP_OP_OR			(2)
#define QPP_OP_NOT			(3)
#define QPP_OP_PARA			(4)
#define QPP_OP_WITHIN		(5)
#define QPP_OP_FUZZY		(6)
#define QPP_OP_FIELD		(7)
#define QPP_OP_MORP			(8)
#define QPP_OP_SYN			(9)
///#define QPP_OP_FILTER		(10)
#define QPP_OP_PHRASE		(11)
#define QPP_OP_NUMERIC		(12)

#define QPP_OP_VIRTUAL_FIELD (13)

enum node_type{
	OPERAND,
	OPERATOR
};

typedef struct _QueryNode{
	int8_t 	operator;
	enum 	node_type type;
	int8_t 	num_of_operands;
	int32_t opParam;
	uint32_t field;
	float 	weight;
	word_t 	word_st;
#define MAX_ORIGINAL_WORD_LEN 1024
	char original_word[MAX_ORIGINAL_WORD_LEN];
} QueryNode;

SB_DECLARE_HOOK(int,preprocess,\
		(void* word_db, char infix[],int max_infix_size,QueryNode postfix[], int max_postfix_size))
SB_DECLARE_HOOK(int,print_querynode,\
		(FILE *fp, QueryNode aPostfixQuery[], int numNode))
SB_DECLARE_HOOK(int,buffer_querynode,\
		(char *result, QueryNode aPostfixQuery[], int numNode))	
SB_DECLARE_HOOK(int,buffer_querynode_LG,\
		(char *result, QueryNode aPostfixQuery[], int numNode))			
			

#endif
