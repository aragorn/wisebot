/* $Id$ */
#include <ctype.h> /* isspace(3) */
#include <string.h> /* memset(3) */
#include <stdlib.h> /* atoi(3) */
#include "common_core.h"
#include "memory.h"
#include "common_util.h" /* hangul_strncmp() */
#include "util.h" /* sb_trim() */
#include "util.h" /* sb_trim() */
#include "mod_qp_general2.h"
#include "mod_api/qp2.h"

// operand pool
static docattr_operand_t* operands = NULL;
static int max_operand_count = 100;
static int current_operand_index = 0;

// list pool
static docattr_list_t* lists = NULL;
static int max_list_count = 10;
static int current_list_index = 0;

general_at_t parser_result; // yacc의 결과는 여기에 들어간다.
general_at_t where;

static void init_operands()
{
	if ( operands == NULL && max_operand_count > 0 ) {
		/* 처음 한 번만 실행되고 free 하지 않는다 
		 * mod_softbot4.c process 에서만 alloc 하게 될 것이다 */
		operands = (docattr_operand_t*)
			sb_malloc(sizeof(docattr_operand_t)*max_operand_count);
	}

	current_operand_index = 0;
	memset( &parser_result, 0, sizeof(general_at_t) );
	memset( operands, 0, sizeof(docattr_operand_t)*max_operand_count );
}

docattr_operand_t* get_new_operand()
{
	if ( current_operand_index >= max_operand_count ) {
		error("too many operands. MAX is %d. "
			  "modify config: <mod_qp_general.c> - MaxOperandsCount", max_operand_count);
		return NULL;
	}
	return &operands[current_operand_index++];
}

static char* get_value_type_name(docattr_value_type_t type)
{
	static char name[][20] = { "UNKNOWN", "INTEGER", "STRING", "MD5", "BOOLEAN", "LIST" };

	switch( type ) {
		case VALUE_INTEGER:
			return name[1];
		case VALUE_STRING:
			return name[2];
		case VALUE_MD5:
			return name[3];
		case VALUE_BOOLEAN:
			return name[4];
		case VALUE_LIST:
			return name[5];
	}

	return name[0];
}

static char* get_operand_type_name(docattr_operand_type_t type)
{
	static char name[][20] = { "UNKNOWN", "VALUE", "FIELD", "EXPR", "LIST" };

	switch( type ) {
		case OPERAND_VALUE:
			return name[1];
		case OPERAND_FIELD:
			return name[2];
		case OPERAND_EXPR:
			return name[3];
		case OPERAND_LIST:
			return name[4];
	}

	return name[0];
}

static void print_operand(docattr_operand_t* operand)
{
	char* value_type;
	char* operand_type;
	char buf[STRING_SIZE];
	int i, remain_buf;

	switch( operand->operand_type ) {
		case OPERAND_VALUE:
			info("OPERAND[%s]: value[%s]",
					get_value_type_name( operand->value_type ), operand->o.value.my_string);
			break;
		case OPERAND_FIELD:
			value_type = get_value_type_name( operand->o.field->value_type );
			info("OPERAND[%s]: field[%s], type[%s]",
					get_value_type_name( operand->value_type ), operand->o.field->name, value_type);
			break;
		case OPERAND_EXPR:
			info("OPERAND[%s]: expr[%s]",
					get_value_type_name( operand->value_type ), operand->o.expr.operator_string);
			break;
		case OPERAND_LIST:
			operand_type = get_operand_type_name( operand->o.list->operands[0]->operand_type );
			snprintf(buf, sizeof(buf), "OPERAND[%s]: list[%s",
					get_value_type_name( operand->value_type ), operand_type);
			remain_buf = (int)sizeof(buf) - strlen(buf);


			for ( i = 1; i < operand->o.list->count; i++ ) {
				operand_type = get_operand_type_name( operand->o.list->operands[i]->operand_type );

				strncat( buf, ",", remain_buf-1 );
				remain_buf = (int)sizeof(buf) - strlen(buf);
				if ( remain_buf <= 1 ) break;

				strncat( buf, operand_type, remain_buf-1 );
				remain_buf = (int)sizeof(buf) - strlen(buf);
				if ( remain_buf <= 1 ) break;
			}
			if ( remain_buf > 1 ) strncat( buf, "]", remain_buf-1 );
			info( buf );
			break;
		default:
			info("OPERAND[%s]: unknown", get_value_type_name( operand->value_type ));
			break;
	}
}

static void init_lists()
{
	if ( lists == NULL && max_list_count > 0 ) {
		/* 처음 한 번만 실행되고 free 하지 않는다 
		 * mod_softbot4.c process 에서만 alloc 하게 될 것이다 */
		lists = (docattr_list_t*)
			sb_calloc(1, sizeof(docattr_list_t)*max_list_count);
	}

	current_list_index = 0;
}

/*
 * list.operands[] 는 재활용되기 때문에 memset 으로 초기화하면 안된다
 * list.size 도 건드리면 안된다.
 * list.count 만 0으로 만들어주자.
 */
docattr_list_t* get_new_list()
{
	if ( current_list_index >= max_list_count ) {
		error("too many lists. MAX is %d. "
			  "modify config: <mod_qp_general.c> - MaxListCount", max_list_count);
		return NULL;
	}

	lists[current_list_index].count = 0;
	return &lists[current_list_index++];
}

#define LIST_EXPAND_SIZE 10

void append_to_list(docattr_list_t* list, docattr_operand_t* operand)
{
	if ( list->count == list->size ) {
		/*
		 * list->operands 는 allocation 만 되고 free 하지 않는다.
		 * 한번 allocation 된 것을 계속 재활용한다
		 */
		list->size = list->size + LIST_EXPAND_SIZE;
		list->operands = (docattr_operand_t**)
			sb_realloc(list->operands, sizeof(docattr_operand_t*) * (list->size));
		// realloc 이 실패하지 않겠지?
	}

	list->operands[list->count] = operand;
	list->count++;
}

// parser_result 에 query에 나타난 field list를 저장해둔다
void add_field_to_cond(docattr_field_t* field)
{
	int i;

	for ( i = 0; i < parser_result.field_list_count; i++ ) {
		if ( field == parser_result.field_list[i] ) return;
	}

	parser_result.field_list[i] = field;
	parser_result.field_list_count++;
}

// parser_result 에서 field를 찾아보고 없으면 NULL
static docattr_field_t* find_field_from_cond(const char* name)
{
	docattr_field_t* field;
	int i;

	return_docattr_field( name, &field );
	if ( field == NULL ) return NULL;

	for ( i = 0; i < parser_result.field_list_count; i++ ) {
		if ( field == parser_result.field_list[i] ) return field;
	}

	return NULL;
}

/*************************************************************
 *                     operator 구현
 *
 * int vs int, string vs string 만 구현하면 된다..
 *************************************************************/

#define OPERAND1 (operand->o.expr.operand1)
#define OPERAND2 (operand->o.expr.operand2)

/*
 * operand 가 expr 인 경우 계산해서 결과를 저장한다.
 * value, field 인 경우라면 이미 result 에 값을 가지고 있다.
 */
#define GET_RESULT(operand) \
	if ( operand->operand_type == OPERAND_EXPR ) \
		operand->o.expr.exec_func( &operand->o.expr );
#define GET_RESULT1 GET_RESULT(expr->operand1)
#define GET_RESULT2 GET_RESULT(expr->operand2)

// 이 RESULT는 위의 GET_RESULT() 와 전혀 상관없음.
#define RESULT  (expr->result.v)
#define RESULT1 (expr->operand1->result->v)
#define RESULT2 (expr->operand2->result->v)

/******************************************************************/

int expr_eq_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean = ( RESULT1.integer == RESULT2.integer );

	return SUCCESS;
}

int expr_eq_string_string(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean =
		( hangul_strncmp( RESULT1.string, RESULT2.string, SHORT_STRING_SIZE ) == 0 );

	return SUCCESS;
}

int expr_eq_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_eq_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->o.expr.exec_func = expr_eq_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator EQ doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_BOOLEAN;
	return SUCCESS;
}

int expr_neq_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean = ( RESULT1.integer != RESULT2.integer );

	return SUCCESS;
}

int expr_neq_string_string(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean =
		( hangul_strncmp( RESULT1.string, RESULT2.string, SHORT_STRING_SIZE ) != 0 );

	return SUCCESS;
}

int expr_neq_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_neq_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->o.expr.exec_func = expr_neq_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator NEQ doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_BOOLEAN;
	return SUCCESS;
}

int expr_gt_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean = ( RESULT1.integer > RESULT2.integer );

	return SUCCESS;
}

int expr_gt_string_string(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean =
		( hangul_strncmp( RESULT1.string, RESULT2.string, SHORT_STRING_SIZE ) > 0 );

	return SUCCESS;
}

int expr_gt_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_gt_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->o.expr.exec_func = expr_gt_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator GT doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_BOOLEAN;
	return SUCCESS;
}

int expr_gteq_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean = ( RESULT1.integer >= RESULT2.integer );

	return SUCCESS;
}

int expr_gteq_string_string(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean =
		( hangul_strncmp( RESULT1.string, RESULT2.string, SHORT_STRING_SIZE ) >= 0 );

	return SUCCESS;
}

int expr_gteq_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_gteq_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->o.expr.exec_func = expr_gteq_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator GT_EQ doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_BOOLEAN;
	return SUCCESS;
}

int expr_lt_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean = ( RESULT1.integer < RESULT2.integer );

	return SUCCESS;
}

int expr_lt_string_string(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean =
		( hangul_strncmp( RESULT1.string, RESULT2.string, SHORT_STRING_SIZE ) < 0 );

	return SUCCESS;
}

int expr_lt_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_lt_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->o.expr.exec_func = expr_lt_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator LT doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_BOOLEAN;
	return SUCCESS;
}

int expr_lteq_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean = ( RESULT1.integer <= RESULT2.integer );

	return SUCCESS;
}

int expr_lteq_string_string(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.boolean =
		( hangul_strncmp( RESULT1.string, RESULT2.string, SHORT_STRING_SIZE ) <= 0 );

	return SUCCESS;
}

int expr_lteq_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_lteq_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->o.expr.exec_func = expr_lteq_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator LT_EQ doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_BOOLEAN;
	return SUCCESS;
}

int expr_bitand_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.integer = ( RESULT1.integer & RESULT2.integer );

	return SUCCESS;
}

int expr_bitand_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_bitand_int_int;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator BIT_AND doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_bitor_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.integer = ( RESULT1.integer | RESULT2.integer );

	return SUCCESS;
}

int expr_bitor_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_bitor_int_int;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator BIT_OR doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_bitnot_int(docattr_expr_t* expr)
{
	GET_RESULT1;

	RESULT.integer = ~RESULT1.integer;

	return SUCCESS;
}

int expr_bitnot_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_bitnot_int;
	}
	else {
		print_operand(OPERAND1);
		error("operator BIT_NOT doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_plus_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.integer = ( RESULT1.integer + RESULT2.integer );

	return SUCCESS;
}

int expr_plus_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_plus_int_int;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator PLUS doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_minus_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.integer = ( RESULT1.integer - RESULT2.integer );

	return SUCCESS;
}

int expr_minus_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_minus_int_int;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator MINUS doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_multiply_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	RESULT.integer = ( RESULT1.integer * RESULT2.integer );

	return SUCCESS;
}

int expr_multiply_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_multiply_int_int;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator MULTIPLY doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_divide_int_int(docattr_expr_t* expr)
{
	GET_RESULT1; GET_RESULT2;

	// 0 으로 나눌 경우는 그냥 답이 0이라고 하고 넘어간다.
	if ( RESULT2.integer == 0 ) {
		error("divide 0!! ignore and regard result as 0");
		RESULT.integer = 0;
	}
	else RESULT.integer = ( RESULT1.integer / RESULT2.integer );

	return SUCCESS;
}

int expr_divide_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_divide_int_int;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator DIVIDE doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_in_int(docattr_expr_t* expr)
{
	docattr_list_t *list1, *list2;
	docattr_operand_t *operand1, *operand2;
	int i, j;

	list1 = expr->operand1->o.list;
	list2 = expr->operand2->o.list;

	// loop 안에서 하면 중복 노가다가 되므로 미리 계산해둔다.
	for ( j = 0; j < list2->count; j++ ) {
		operand2 = list2->operands[j];
		GET_RESULT(operand2);
	}

	for ( i = 0; i < list1->count; i++ ) {
		operand1 = list1->operands[i];
		GET_RESULT(operand1);

		for ( j = 0; j < list2->count; j++ ) {
			operand2 = list2->operands[j];

			if ( operand1->result->v.integer == operand2->result->v.integer ) break;
		} // for j

		if ( j == list2->count ) break;
	} // for i

	// 없는 게 있었으면 i loop 중간에 break 당했다.
	RESULT.boolean = ( i == list1->count );
	return SUCCESS;
}

int expr_in_string(docattr_expr_t* expr)
{
	docattr_list_t *list1, *list2;
	docattr_operand_t *operand1, *operand2;
	int i, j;

	list1 = expr->operand1->o.list;
	list2 = expr->operand2->o.list;

	// loop 안에서 하면 중복 노가다가 되므로 미리 계산해둔다.
	for ( j = 0; j < list2->count; j++ ) {
		operand2 = list2->operands[j];
		GET_RESULT(operand2);
	}

	for ( i = 0; i < list1->count; i++ ) {
		operand1 = list1->operands[i];
		GET_RESULT(operand1);

		for ( j = 0; j < list2->count; j++ ) {
			operand2 = list2->operands[j];

			if ( hangul_strncmp( operand1->result->v.string,
						operand2->result->v.string, SHORT_STRING_SIZE ) == 0 ) break;
		} // for j

		if ( j == list2->count ) break;
	} // for i

	// 없는 게 있었으면 i loop 중간에 break 당했다.
	RESULT.boolean = ( i == list1->count );
	return SUCCESS;
}

int expr_in_set(docattr_operand_t* operand)
{
	docattr_value_type_t value_type;
	docattr_list_t *list1, *list2;
	int i;

	if ( OPERAND1->value_type != VALUE_LIST
			|| OPERAND2->value_type != VALUE_LIST ) {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator<in> should have list operands");
		return FAIL;
	}
	
	list1 = operand->o.expr.operand1->o.list;
	list2 = operand->o.expr.operand2->o.list;

	// 나머지 모든 operand 들도 같은 value_type 이어야 한다.
	value_type = list1->operands[0]->value_type;
	for ( i = 1; i < list1->count; i++ ) {
		if ( value_type == list1->operands[i]->value_type ) continue;

		print_operand(list1->operands[i]);
		error("that operand has diffent type. expected[%s]",
				get_value_type_name( value_type ));
		return FAIL;
	}
	for ( i = 0; i < list2->count; i++ ) {
		if ( value_type == list2->operands[i]->value_type ) continue;

		print_operand(list2->operands[i]);
		error("that operand has diffent type. expected[%s]",
				get_value_type_name( value_type ));
		return FAIL;
	}

	if ( value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_in_int;
	}
	else if ( value_type == VALUE_STRING ) {
		operand->o.expr.exec_func = expr_in_string;
	}
	else {
		error("operator <in> doesn't support %s type", get_value_type_name( value_type ));
		return FAIL;
	}

	operand->value_type = VALUE_BOOLEAN;
	return SUCCESS;
}

int expr_common_int(docattr_expr_t* expr)
{
	docattr_list_t *list1, *list2;
	docattr_operand_t *operand1, *operand2;
	int i, j;

	list1 = expr->operand1->o.list;
	list2 = expr->operand2->o.list;

	// loop 안에서 하면 중복 노가다가 되므로 미리 계산해둔다.
	for ( j = 0; j < list2->count; j++ ) {
		operand2 = list2->operands[j];
		GET_RESULT(operand2);
	}

	for ( i = 0; i < list1->count; i++ ) {
		operand1 = list1->operands[i];
		GET_RESULT(operand1);

		for ( j = 0; j < list2->count; j++ ) {
			operand2 = list2->operands[j];

			if ( operand1->result->v.integer == operand2->result->v.integer ) break;
		} // for j

		if ( j != list2->count ) break;
	} // for i

	// 하나라도 있었으면 i loop 중간에 break 당했다.
	RESULT.boolean = ( i != list1->count );
	return SUCCESS;
}

int expr_common_string(docattr_expr_t* expr)
{
	docattr_list_t *list1, *list2;
	docattr_operand_t *operand1, *operand2;
	int i, j;

	list1 = expr->operand1->o.list;
	list2 = expr->operand2->o.list;

	// loop 안에서 하면 중복 노가다가 되므로 미리 계산해둔다.
	for ( j = 0; j < list2->count; j++ ) {
		operand2 = list2->operands[j];
		GET_RESULT(operand2);
	}

	for ( i = 0; i < list1->count; i++ ) {
		operand1 = list1->operands[i];
		GET_RESULT(operand1);

		for ( j = 0; j < list2->count; j++ ) {
			operand2 = list2->operands[j];

			if ( hangul_strncmp( operand1->result->v.string,
						operand2->result->v.string, SHORT_STRING_SIZE ) == 0 ) break;
		} // for j

		if ( j != list2->count ) break;
	} // for i

	// 하나라도 있었으면 i loop 중간에 break 당했다.
	RESULT.boolean = ( i != list1->count );
	return SUCCESS;
}

int expr_common_set(docattr_operand_t* operand)
{
	docattr_value_type_t value_type;
	docattr_list_t *list1, *list2;
	int i;

	if ( OPERAND1->value_type != VALUE_LIST
			|| OPERAND2->value_type != VALUE_LIST ) {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator<common> should have list operands");
		return FAIL;
	}
	
	list1 = operand->o.expr.operand1->o.list;
	list2 = operand->o.expr.operand2->o.list;

	// 나머지 모든 operand 들도 같은 value_type 이어야 한다.
	value_type = list1->operands[0]->value_type;
	for ( i = 1; i < list1->count; i++ ) {
		if ( value_type == list1->operands[i]->value_type ) continue;

		print_operand(list1->operands[i]);
		error("that operand has diffent type. expected[%s]",
				get_value_type_name( value_type ));
		return FAIL;
	}
	for ( i = 0; i < list2->count; i++ ) {
		if ( value_type == list2->operands[i]->value_type ) continue;

		print_operand(list2->operands[i]);
		error("that operand has diffent type. expected[%s]",
				get_value_type_name( value_type ));
		return FAIL;
	}

	if ( value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_common_int;
	}
	else if ( value_type == VALUE_STRING ) {
		operand->o.expr.exec_func = expr_common_string;
	}
	else {
		error("operator <common> doesn't support %s type", get_value_type_name( value_type ));
		return FAIL;
	}

	operand->value_type = VALUE_BOOLEAN;
	return SUCCESS;
}

int expr_logical_and(docattr_expr_t* expr)
{
	GET_RESULT1;
	if ( !RESULT1.boolean ) {
		RESULT.boolean = 0;
		return SUCCESS;
	}

	GET_RESULT2;
	if ( !RESULT2.boolean ) {
		RESULT.boolean = 0;
		return SUCCESS;
	}

	RESULT.boolean = 1;
	return SUCCESS;
}

int expr_logical_or(docattr_expr_t* expr)
{
	GET_RESULT1;
	if ( RESULT1.boolean ) {
		RESULT.boolean = 1;
		return SUCCESS;
	}

	GET_RESULT2;
	if ( RESULT2.boolean ) {
		RESULT.boolean = 1;
		return SUCCESS;
	}

	RESULT.boolean = 0;
	return SUCCESS;
}

int expr_logical_not(docattr_expr_t* expr)
{
	GET_RESULT1;

	RESULT.boolean = !RESULT1.boolean;
	return SUCCESS;
}

#undef OPERAND1
#undef OPERAND2

#undef GET_RESULT
#undef GET_RESULT1
#undef GET_RESULT2

#undef RESULT
#undef RESULT1
#undef RESULT2

/************************************************************
 *            계산식 tree로 뭔가 하는 작업들..
 ************************************************************/

// root operand 부터 depth first search 하면서 postfix 계산식 출력 
void print_tree(docattr_operand_t* operand)
{
	if ( operand == NULL ) return;

	if ( operand->operand_type == OPERAND_EXPR ) {
		print_tree( operand->o.expr.operand1 );
		print_tree( operand->o.expr.operand2 );
	}
	print_operand( operand );
}

// parser_result 의 field 에 있는 값들을 미리 가져온다.
void fetch_docattr_field(docattr_t* docattr, general_at_t* at)
{
	int i;
	docattr_field_t* field;

	for ( i = 0; i < at->field_list_count; i++ ) {
		field = at->field_list[i];
		field->get_func( docattr, field, &field->value );
	}
}

/************************************************************
 *               module & api 관련 코드
 ************************************************************/

static int init()
{
/*	if ( sizeof(general_cond_t) > sizeof(docattr_cond_t) ) {
		error("sizeof(general_cond_t): %d, sizeof(docattr_cond_t): %d",
				(int) sizeof(general_cond_t), (int) sizeof(docattr_cond_t));
		return FAIL;
	}*/

	return SUCCESS;
}

#if 0
static int compare_function(void* dest, void* cond, uint32_t docid)
{
	docattr_t* docattr = (docattr_t*) dest;
	// cond는 무시한다

	fetch_docattr_field( docattr, &at );

	if ( at.root_operand->operand_type == OPERAND_EXPR )
		at.root_operand->o.expr.exec_func( &at.root_operand->o.expr );

	return (int) at.root_operand->result->v.boolean;
}

static int compare2_function(void* dest, void* cond, uint32_t docid)
{
	docattr_t* docattr = (docattr_t*) dest;
	// cond는 무시한다.
	int i;

	if ( at2.root_operand == NULL
			&& group_field_list[0] == NULL )
		return MINUS_DECLINE;

	// group field는 항상 integer이다.
	if ( group_field_list[0] == NULL ) {}
	else if ( group_field_list[1] == NULL ) { // group_field 가 하나라면 좀 더 빨리 하자.
		for ( i = 0; i < MAX_GROUP_FIELD && group_field_list[i]; i++ ) {
			docattr_field_t* field = group_field_list[i];
			docattr_integer field_value;

			field->get_func( docattr, field, &field->value );
			field_value = field->value.v.integer;

			if ( field_value <= 0 || field_value > MAX_GROUP_FIELD_VALUE ) {
				warn("invalid group field[%s] value[%ld], MAX(%d)",
						field->name, field_value, MAX_GROUP_FIELD_VALUE);
				continue;
			}

			group_field_count[i][0]++;
			group_field_count[i][field_value]++;
			if ( group_field_limit[i] > 0
					&& group_field_count[i][field_value] > group_field_limit[i] )
				return 0;
		}
	}
	else { // group_field가 두 개 이상이면 for loop를 두 번 돌아야 한다.
		for ( i = 0; i < MAX_GROUP_FIELD && group_field_list[i]; i++ ) {
			docattr_field_t* field = group_field_list[i];
			docattr_integer field_value;

			field->get_func( docattr, field, &field->value );
			field_value = field->value.v.integer;

			if ( field_value <= 0 || field_value > MAX_GROUP_FIELD_VALUE ) {
				warn("invalid group field[%s] value[%ld]", field->name, field_value);
				continue;
			}

			group_field_count[i][0]++;
			group_field_count[i][field_value]++;
		}

		for ( i = 0; i < MAX_GROUP_FIELD && group_field_list[i]; i++ ) {
			docattr_field_t* field = group_field_list[i];
			docattr_integer field_value;

			field->get_func( docattr, field, &field->value );
			field_value = field->value.v.integer;

			if ( field_value <= 0 || field_value > MAX_GROUP_FIELD_VALUE ) {
				warn("invalid group field[%s] value[%ld]", field->name, field_value);
				continue;
			}

			if ( group_field_limit[i] > 0
					&& group_field_count[i][field_value] > group_field_limit[i] )
				return 0;
		}
	}

	if ( at2.root_operand ) {
		fetch_docattr_field( docattr, &at2 );

		if ( at2.root_operand->operand_type == OPERAND_EXPR )
			at2.root_operand->o.expr.exec_func( &at2.root_operand->o.expr );

		return (int) at2.root_operand->result->v.boolean;
	}
	else return 1;
}

static int set_group_result_function(void* cond, group_result_t* group_result, int* size)
{
	// cond는 무시한다.
	int i, j, curr = 0;
	char number[20], *enum_name;

	for ( i = 0; i < MAX_GROUP_FIELD && group_field_list[i]; i++ ) {
		docattr_field_t* field = group_field_list[i];

		for ( j = 1; j < MAX_GROUP_FIELD_VALUE && curr < *size; j++ ) {
			if ( group_field_count[i][j] == 0 ) continue;

			strcpy( group_result[curr].field, field->name );
			if ( is_enum_field( field->field_type ) ) {
				enum_name = return_enum_name(j);
				if ( enum_name != NULL ) strcpy( group_result[curr].value, enum_name );
				else {
					warn("unknown enum value: %d", j);
					snprintf( number, sizeof(number), "%d", j );
					strcpy( group_result[curr].value, number );
				}
			}
			else {
				snprintf( number, sizeof(number), "%d", j );
				strcpy( group_result[curr].value, number );
			}
			group_result[curr++].count = group_field_count[i][j];
		}

		if ( curr < *size ) {
			strcpy( group_result[curr].field, field->name );
			strcpy( group_result[curr].value, "#" );
			group_result[curr++].count = group_field_count[i][0];
		}
	}

	if ( curr >= *size ) {
		error("not enough group_result size[%d]", *size);
		return FAIL;
	}

	*size = curr;

	return SUCCESS;
}

static int string_trim(char* string)
{
	int len = strlen( string );
	while ( len > 0 && isspace((int) string[len-1]) )
		string[--len] = '\0';

	return len;
}

// AT=
static int docattr_qpp(docattr_cond_t *cond, char* attrquery)
{
	int ret, len;

	len = string_trim( attrquery );
	info("docattr query: [%s]", attrquery);
	if ( len == 0 ) attrquery = "Delete=0";

retry:
	init_operands();
	init_lists();
	__yy_scan_string( attrquery );

	ret = __yyparse();
	if ( ret != 0 ) {
		error("yyparse: %d", ret );
		return FAIL;
	}

	// Delete field가 없으면 강제로 넣어야 하는데... shit 이다.
	if ( find_field_from_cond( "Delete" ) == NULL ) {
		char attrquery2[LONG_STRING_SIZE];

		if ( sizeof(attrquery2) < len+20 ) {
			error("insufficient attrquery2 size. current attrquery len:[%d]", len);
			return FAIL;
		}

		warn("need Delete=0 condition in attrquery");

		snprintf( attrquery2, sizeof(attrquery2), "(%s) and Delete=0", attrquery );
		len = strlen( attrquery2 );
		attrquery = attrquery2;
		goto retry;
	}

	at = parser_result;
	print_tree( parser_result.root_operand );

	return SUCCESS;
}

// AT2=
static int docattr_qpp2(docattr_cond_t *cond, char* attr2query)
{
	int ret, len;

	len = string_trim( attr2query );
	if ( len == 0 ) {
		at2.root_operand = NULL;
		return SUCCESS;
	}

	info("docattr2 query: [%s]", attr2query);

	__yy_scan_string( attr2query );

	ret = __yyparse();
	if ( ret != 0 ) {
		error("yyparse: %d", ret );
		return FAIL;
	}

	at2 = parser_result;
	print_tree( parser_result.root_operand );

	return SUCCESS;
}

// GR=
static int docattr_qpp_group(docattr_cond_t *cond, char* groupquery)
{
	int len, i;
	char *group_field, *group_count;

	len = string_trim( groupquery );
	if ( len == 0 ) {
		group_field_list[0] = NULL;
		return SUCCESS;
	}

	memset( group_field_count, 0, sizeof(group_field_count) );
	memset( group_field_limit, 0, sizeof(group_field_limit) );

	i = 0;
	group_field = strtok( groupquery, "," );
	while ( group_field != NULL && i < MAX_GROUP_FIELD ) {
		group_count = group_field;
		while ( *group_count != ':' && *group_count != '\0' ) group_count++;

		if ( *group_count != '\0' ) {
			*group_count = '\0';
			group_count++;

			if ( isNumber( group_count, &group_field_limit[i] ) != SUCCESS ) {
				error("invalid group_count[%s] of group_field[%s]", group_count, group_field);
				return FAIL;
			}
		}
		else group_field_limit[i] = 0;

		return_docattr_field( group_field, &group_field_list[i] );
		if ( group_field_list[i] == NULL ) {
			error("invalid group_field[%s]", group_field);
			return FAIL;
		}

		if ( group_field_list[i]->value_type != VALUE_INTEGER ) {
			error("group field[%s]'s value type is not integer", group_field);
			return FAIL;
		}

		group_field = strtok( NULL, "," );
		i++;
	}

	if ( group_field != NULL ) {
		error("too many group_fields...");
		return FAIL;
	}

	return SUCCESS;
}
#endif

static int qp_set_where_expression(char* clause)
{
	int ret, len;

	clause = sb_trim( clause );
	len = strlen(clause);
	if (len == 0) {
	    clause = "Delete=0";
	}

	info("where clause : [%s]", clause);

retry:
	init_operands();
	init_lists();

	__yy_scan_string( clause );

	ret = __yyparse();
	if ( ret != 0 ) {
		error("yyparse: %d", ret );
		return FAIL;
	}

	// Delete field가 없으면 default로 입력.
	if ( find_field_from_cond( "Delete" ) == NULL ) {
		char clause2[LONG_STRING_SIZE];

		if ( sizeof(clause2) < len+20 ) {
			error("insufficient attrquery2 size. current attrquery len:[%d]", len);
			return FAIL;
		}

		warn("need Delete=0 condition in attrquery");

		snprintf( clause2, sizeof(clause2), "(%s) and Delete=0", clause );
		len = strlen( clause2 );
		clause = clause2;
		goto retry;
	}

	print_tree( parser_result.root_operand );

    where = parser_result;

	return SUCCESS;
}

static int qp_cb_where(const void* dest)
{
	docattr_t* docattr = (docattr_t*) dest;

	fetch_docattr_field( docattr, &where );

	if ( where.root_operand->operand_type == OPERAND_EXPR )
		where.root_operand->o.expr.exec_func( &where.root_operand->o.expr );

	return (int) where.root_operand->result->v.boolean;
}

static int qp_cb_orderby(const void* dest, const void* sour, void* userdata)
{
	int i, diff = 0;
	docattr_value_t value1, value2;
	docattr_field_t* field;
	orderby_rule_list_t *rules;
	virtual_document_t *dest_vd;
	virtual_document_t *sour_vd;

	rules = (orderby_rule_list_t*)userdata;

	if ( rules == NULL ) return 0;

	dest_vd = (virtual_document_t*)dest;
	sour_vd = (virtual_document_t*)sour;

	for ( i = 0; i < rules->cnt; i++ ) {
		orderby_rule_t* order = &rules->list[i];

	    return_docattr_field(order->rule.name, &field);
		if(field == NULL) {
			error("can not get docattr_field[%s]", order->rule.name);
			return 0;
		}

		if ( order->rule.type == RELEVANCY ) {
			diff = dest_vd->relevancy - sour_vd->relevancy;
		}
		else if ( order->rule.type == DID ) {
		    diff = ((virtual_document_t*) dest)->id - ((virtual_document_t*) sour)->id;
		}
		else { // 일반 docattr field
			field->get_func( dest_vd->docattr, field, &value1 );
			field->get_func( sour_vd->docattr, field, &value2 );

			diff = field->compare_func( &value1, &value2 );
		}

		if ( diff != 0 ) {
			diff = diff > 0 ? 1 : -1;
			return diff * order->type;
		}
	} // for (i)

	return 0;
}
////////////////////////////////////////////////////
static void set_max_operand_count(configValue v)
{
	max_operand_count = atoi( v.argument[0] );
}

static void set_max_list_count(configValue v)
{
	max_list_count = atoi( v.argument[0] );
}

static config_t config[] = {
	CONFIG_GET("MaxOperandCount", set_max_operand_count, 1,
			"maximum operand count in AT + AT2"),
	CONFIG_GET("MaxListCount", set_max_list_count, 1,
			"maximum list count (not list size)"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_qp_set_where_expression(qp_set_where_expression,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_cb_where_virtual_document(qp_cb_where,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_cb_orderby_virtual_document(qp_cb_orderby,NULL,NULL,HOOK_MIDDLE);
	/*
	sb_hook_docattr_compare_function(compare_function,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_compare2_function(compare2_function,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_set_group_result_function(set_group_result_function,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_docattr_query_process(docattr_qpp,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_docattr_query2_process(docattr_qpp2,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_docattr_group_query_process(docattr_qpp_group,NULL,NULL,HOOK_MIDDLE);
	*/
}

module qp_general2_module = {
	STANDARD_MODULE_STUFF,
	config,                 /* config */
	NULL,                   /* registry */
	init,                   /* initialize */
	NULL,                   /* child_main */
	NULL,                   /* scoreboard */
	register_hooks          /* register hook api */
};

