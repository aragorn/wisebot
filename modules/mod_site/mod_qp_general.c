#include "mod_qp_general.h"
#include "mod_qp_general_yacc.h"
#include "mod_api/qp.h"
#include <ctype.h>
#include <stdio_ext.h>

static docattr_operand_t operands[MAX_OPERAND_NUM];
static int current_operand_index = 0;

// 계산식 tree의 가장 윗부분 node
general_cond_t general_cond;

void init_operands()
{
	current_operand_index = 0;
	memset( &general_cond, 0, sizeof(general_cond) );
	memset( operands, 0, sizeof(operands) );
}

docattr_operand_t* get_new_operand()
{
	if ( current_operand_index >= MAX_OPERAND_NUM ) {
		error("too many operands. MAX is %d", MAX_OPERAND_NUM);
		return NULL;
	}
	return &operands[current_operand_index++];
}

char* get_value_type_name(docattr_operand_type_t type)
{
	static char name[][20] = { "INTEGER", "STRING", "MD5", "UNKNOWN" };

	switch( type ) {
		case VALUE_INTEGER:
			return name[0];
		case VALUE_STRING:
			return name[1];
		case VALUE_MD5:
			return name[2];
	}

	return name[3];
}

void print_operand(docattr_operand_t* operand)
{
	char* value_type;

	switch( operand->operand_type ) {
		case OPERAND_VALUE:
			info("OPERAND[%s]: value[%s]",
					get_value_type_name( operand->value_type ), operand->value.my_string);
			break;
		case OPERAND_FIELD:
			value_type = get_value_type_name( operand->field->value_type );
			info("OPERAND[%s]: field[%s], type[%s]",
					get_value_type_name( operand->value_type ), operand->field->name, value_type);
			break;
		case OPERAND_EXPR:
			info("OPERAND[%s]: expr[%s]",
					get_value_type_name( operand->value_type ), operand->expr.operator_string);
			break;
	}
}

// general_cond 에 query에 나타난 field list를 저장해둔다
void add_field_to_cond(docattr_field_t* field)
{
	int i;

	for ( i = 0; i < general_cond.field_list_count; i++ ) {
		if ( field == general_cond.field_list[i] ) return;
	}

	general_cond.field_list[i] = field;
	general_cond.field_list_count++;
}

// general_cond 에서 field를 찾아보고 없으면 NULL
docattr_field_t* find_field_from_cond(const char* name)
{
	docattr_field_t* field;
	int i;

	return_docattr_field( name, &field );
	if ( field == NULL ) return NULL;

	for ( i = 0; i < general_cond.field_list_count; i++ ) {
		if ( field == general_cond.field_list[i] ) return field;
	}

	return NULL;
}

/*************************************************************
 *                     operator 구현
 *
 * int vs int, string vs string 만 구현하면 된다..
 *************************************************************/

#define OPERAND1 (operand->expr.operand1)
#define OPERAND2 (operand->expr.operand2)

#define RESULT  (expr->result)
#define RESULT1 (expr->operand1->result)
#define RESULT2 (expr->operand2->result)

int operand_expr_exec_func(docattr_operand_t* operand)
{
	if ( OPERAND1 && OPERAND1->exec_func ) OPERAND1->exec_func( OPERAND1 );
	if ( OPERAND2 && OPERAND2->exec_func ) OPERAND2->exec_func( OPERAND2 );
	operand->expr.exec_func( &operand->expr );

	return SUCCESS;
}

/******************************************************************/

int expr_eq_int_int(docattr_expr_t* expr)
{
	RESULT.integer = ( RESULT1->integer == RESULT2->integer );

	return SUCCESS;
}

int expr_eq_string_string(docattr_expr_t* expr)
{
	RESULT.integer =
		( hangul_strncmp( RESULT1->string, RESULT2->string, sizeof(RESULT1->my_string) ) == 0 );

	return SUCCESS;
}

int expr_eq_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->expr.exec_func = expr_eq_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->expr.exec_func = expr_eq_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator EQ doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_neq_int_int(docattr_expr_t* expr)
{
	RESULT.integer = ( RESULT1->integer != RESULT2->integer );

	return SUCCESS;
}

int expr_neq_string_string(docattr_expr_t* expr)
{
	RESULT.integer =
		( hangul_strncmp( RESULT1->string, RESULT2->string, sizeof(RESULT1->my_string) ) != 0 );

	return SUCCESS;
}

int expr_neq_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->expr.exec_func = expr_neq_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->expr.exec_func = expr_neq_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator NEQ doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_gt_int_int(docattr_expr_t* expr)
{
	RESULT.integer = ( RESULT1->integer > RESULT2->integer );

	return SUCCESS;
}

int expr_gt_string_string(docattr_expr_t* expr)
{
	RESULT.integer =
		( hangul_strncmp( RESULT1->string, RESULT2->string, sizeof(RESULT1->my_string) ) > 0 );

	return SUCCESS;
}

int expr_gt_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->expr.exec_func = expr_gt_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->expr.exec_func = expr_gt_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator GT doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_gteq_int_int(docattr_expr_t* expr)
{
	RESULT.integer = ( RESULT1->integer >= RESULT2->integer );

	return SUCCESS;
}

int expr_gteq_string_string(docattr_expr_t* expr)
{
	RESULT.integer =
		( hangul_strncmp( RESULT1->string, RESULT2->string, sizeof(RESULT1->my_string) ) >= 0 );

	return SUCCESS;
}

int expr_gteq_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->expr.exec_func = expr_gteq_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->expr.exec_func = expr_gteq_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator GT_EQ doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_lt_int_int(docattr_expr_t* expr)
{
	RESULT.integer = ( RESULT1->integer < RESULT2->integer );

	return SUCCESS;
}

int expr_lt_string_string(docattr_expr_t* expr)
{
	RESULT.integer =
		( hangul_strncmp( RESULT1->string, RESULT2->string, sizeof(RESULT1->my_string) ) < 0 );

	return SUCCESS;
}

int expr_lt_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->expr.exec_func = expr_lt_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->expr.exec_func = expr_lt_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator LT doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_lteq_int_int(docattr_expr_t* expr)
{
	RESULT.integer = ( RESULT1->integer <= RESULT2->integer );

	return SUCCESS;
}

int expr_lteq_string_string(docattr_expr_t* expr)
{
	RESULT.integer =
		( hangul_strncmp( RESULT1->string, RESULT2->string, sizeof(RESULT1->my_string) ) <= 0 );

	return SUCCESS;
}

int expr_lteq_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->expr.exec_func = expr_lteq_int_int;
	}
	else if ( OPERAND1->value_type == VALUE_STRING
			&& OPERAND2->value_type == VALUE_STRING ) {
		operand->expr.exec_func = expr_lteq_string_string;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator LT_EQ doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
	return SUCCESS;
}

int expr_bitand_int_int(docattr_expr_t* expr)
{
	RESULT.integer = ( RESULT1->integer & RESULT2->integer );

	return SUCCESS;
}

int expr_bitand_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->expr.exec_func = expr_bitand_int_int;
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
	RESULT.integer = ( RESULT1->integer | RESULT2->integer );

	return SUCCESS;
}

int expr_bitor_set(docattr_operand_t* operand)
{
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->expr.exec_func = expr_bitor_int_int;
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

int expr_logical_and(docattr_expr_t* expr)
{
	RESULT.integer = RESULT1->integer && RESULT2->integer;
	return SUCCESS;
}

int expr_logical_or(docattr_expr_t* expr)
{
	RESULT.integer = RESULT1->integer || RESULT2->integer;
	return SUCCESS;
}

int expr_logical_not(docattr_expr_t* expr)
{
	RESULT.integer = !RESULT1->integer;
	return SUCCESS;
}

#undef OPERAND1
#undef OPERAND2

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
		print_tree( operand->expr.operand1 );
		print_tree( operand->expr.operand2 );
	}
	print_operand( operand );
}

// general_cond 의 field 에 있는 값들을 미리 가져온다.
void fetch_docattr_field(general_docattr_t* docattr, general_cond_t* cond)
{
	int i;
	docattr_field_t* field;

	for ( i = 0; i < cond->field_list_count; i++ ) {
		field = cond->field_list[i];
		field->get_func( docattr, field, &field->value );
	}
}

/************************************************************
 *               module & api 관련 코드
 ************************************************************/

static int init()
{
	if ( sizeof(general_cond_t) > sizeof(docattr_cond_t) ) {
		error("sizeof(general_cond_t): %d, sizeof(docattr_cond_t): %d",
				(int) sizeof(general_cond_t), (int) sizeof(docattr_cond_t));
		return FAIL;
	}

	return SUCCESS;
}

static int compare_function(void* dest, void* cond, uint32_t docid)
{
	general_docattr_t* docattr = (general_docattr_t*) dest;
	general_cond_t* doccond = (general_cond_t*) cond;

	fetch_docattr_field( docattr, doccond );
	doccond->root_operand->exec_func( doccond->root_operand );

	return (int) doccond->root_operand->result->integer;
}

/*
static int compare2_function(void* dest, void* cond, uint32_t docid)
{
	return 1;
}
*/

static int docattr_qpp(docattr_cond_t *cond, char* attrquery)
{
	int ret, len;

	len = strlen( attrquery );
	while ( len > 0 && isspace(attrquery[len-1]) )
		attrquery[--len] = '\0';

	info("docattr query: [%s]", attrquery);
	if ( len == 0 ) attrquery = "Delete=0";

retry:
	init_operands();
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

	*((general_cond_t*) cond) = general_cond;
	print_tree( general_cond.root_operand );

	return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_docattr_compare_function(compare_function,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_docattr_compare2_function(compare2_function,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_docattr_query_process(docattr_qpp,NULL,NULL,HOOK_MIDDLE);
}

module qp_general_module = {
	STANDARD_MODULE_STUFF,
	NULL,                   /* config */
	NULL,                   /* registry */
	init,                   /* initialize */
	NULL,                   /* child_main */
	NULL,                   /* scoreboard */
	register_hooks          /* register hook api */
};

