#include "mod_qp_general.h"
#include "mod_api/qp.h"
#include "mod_qp/mod_qp.h"
#include <ctype.h>
#include <stdio_ext.h>

static docattr_operand_t operands[MAX_OPERAND_NUM];
static int current_operand_index = 0;

general_at_t parser_result; // yacc의 결과는 여기에 들어간다.

// 원래는 docattr_cond_t 를 사용해야 하지만 용량이 모자라서 ...
#define MAX_GROUP_FIELD 3 // 통합검색 가능한 최대 field수
#define MAX_GROUP_FIELD_VALUE 32
general_at_t at; // AT=
general_at_t at2; // AT2=
docattr_field_t* group_field_list[MAX_GROUP_FIELD]; // GR=
docattr_integer group_field_limit[MAX_GROUP_FIELD];
docattr_integer group_field_count[MAX_GROUP_FIELD][MAX_GROUP_FIELD_VALUE]; // 통합검색 결과저장

void init_operands()
{
	current_operand_index = 0;
	memset( &parser_result, 0, sizeof(general_at_t) );
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

char* get_value_type_name(docattr_value_type_t type)
{
	static char name[][20] = { "INTEGER", "STRING", "MD5", "BOOLEAN", "UNKNOWN" };

	switch( type ) {
		case VALUE_INTEGER:
			return name[0];
		case VALUE_STRING:
			return name[1];
		case VALUE_MD5:
			return name[2];
		case VALUE_BOOLEAN:
			return name[3];
	}

	return name[4];
}

void print_operand(docattr_operand_t* operand)
{
	char* value_type;

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
	}
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
docattr_field_t* find_field_from_cond(const char* name)
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

#define GET_RESULT1 \
	if ( expr->operand1->operand_type == OPERAND_EXPR ) \
		expr->operand1->o.expr.exec_func( &expr->operand1->o.expr );
#define GET_RESULT2 \
	if ( expr->operand2->operand_type == OPERAND_EXPR ) \
		expr->operand2->o.expr.exec_func( &expr->operand2->o.expr );

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
	if ( OPERAND1->value_type == VALUE_INTEGER
			&& OPERAND2->value_type == VALUE_INTEGER ) {
		operand->o.expr.exec_func = expr_bitnot_int;
	}
	else {
		print_operand(OPERAND1);
		print_operand(OPERAND2);
		error("operator BIT_NOT doesn't support above types");
		return FAIL;
	}

	operand->value_type = VALUE_INTEGER;
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
				warn("invalid group field[%s] value[%ld]", field->name, field_value);
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
			if ( field->field_type == FIELD_ENUM || field->field_type == FIELD_ENUM8
					|| field->field_type == FIELD_ENUMBIT ) {
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

static void register_hooks(void)
{
	sb_hook_docattr_compare_function(compare_function,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_compare2_function(compare2_function,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_set_group_result_function(set_group_result_function,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_docattr_query_process(docattr_qpp,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_docattr_query2_process(docattr_qpp2,NULL,NULL,HOOK_MIDDLE);
	sb_hook_qp_docattr_group_query_process(docattr_qpp_group,NULL,NULL,HOOK_MIDDLE);
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

