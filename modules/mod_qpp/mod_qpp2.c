/* $Id$ */
#include "common_core.h"

static qpp_operant_t* operands = NULL;
static int max_operand_count = 100;
static int current_operand_index = 0;

static qpp_operator_t* operators = NULL;
static int max_operator_count = 100;
static int current_operator_index = 0;

qpp_tree_t *result_tree;

static void init_operands()
{
	if ( operands == NULL && max_operand_count > 0 ) {
		/* 각 process별로 처음 한번만 실행되고, free하지 않는다. */
		operands = (qpp2_operator_t*)
			sb_calloc(max_operand_count, sizeof(qpp2_operator_t));
	}
	current_operand_index = 0;
	memset( operands, 0, max_operand_count*sizeof(qpp2_operator_t));
	memset( &result_tree, 0, sizeof(qpp2_tree_t));
}

qpp_operand_t* get_new_
