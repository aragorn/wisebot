%{
#include "mod_docattr_general.h"
#include "mod_qp_general.h"
#include "mod_qp_general_yacc.h"

static void prepare_field(docattr_operand_t* operand);
static int prepare_value(docattr_operand_t* operand, const char* text, int len, int maybeint);

#define SET_YYLVAL \
	yylval = get_new_operand(); \
	if ( yylval == NULL ) { \
		error("error while processing [%s], MAX_OPERAND_NUM[%d]", yytext, MAX_OPERAND_NUM); \
		return OPERAND_ERROR; \
	}

#define SET_EXPR \
	yylval->operand_type = OPERAND_EXPR; \
	yylval->exec_func = operand_expr_exec_func; \
	yylval->result = &yylval->expr.result; \
	strncpy( yylval->expr.operator_string, yytext, sizeof(yylval->expr.operator_string) );

%}
A [aA]
D [dD]
N [nN]
O [oO]
R [rR]
T [tT]

%START ESC
%option nounput
%%

[\t\r\n ]+ ;
=				{ SET_YYLVAL; SET_EXPR; return EQ; }
(\<\>|!=)		{ SET_YYLVAL; SET_EXPR; return NEQ; }
\>				{ SET_YYLVAL; SET_EXPR; return GT; }
\>=				{ SET_YYLVAL; SET_EXPR; return GT_EQ; }
\<				{ SET_YYLVAL; SET_EXPR; return LT; }
\<=				{ SET_YYLVAL; SET_EXPR; return LT_EQ; }
&				{ SET_YYLVAL; SET_EXPR; return BIT_AND; }
\|				{ SET_YYLVAL; SET_EXPR; return BIT_OR; }
!				{ SET_YYLVAL; SET_EXPR; return BIT_NOT; }
(&&|{A}{N}{D})	{ SET_YYLVAL; SET_EXPR; return LOGICAL_AND; }
(\|\||{O}{R})	{ SET_YYLVAL; SET_EXPR; return LOGICAL_OR; }
(!!|{N}{O}{T})	{ SET_YYLVAL; SET_EXPR; return LOGICAL_NOT; }
\(				{ return LPAREN; }
\)				{ return RPAREN; }
\"[^\"\\]*\"	{
	SET_YYLVAL;

	if ( prepare_value( yylval, yytext, yyleng, 0 ) == SUCCESS ) return VALUE;
	else return OPERAND_ERROR;
}

\"[^\"\\]*\\.		{ BEGIN ESC; yymore(); }
<ESC>[^\"\\]*\\.	{ yymore(); }
<ESC>[^\"\\]*\"		{ BEGIN 0;
	SET_YYLVAL;

	if ( prepare_value( yylval, yytext, yyleng, 0 ) == SUCCESS ) return VALUE;
	else return OPERAND_ERROR;
}

[^=\>\<&\|!\(\)\t\r\n\" ]+ 	{
	SET_YYLVAL;

	// field �̸��� ���� �� ������ NAME �ƴϸ� VALUE
	return_docattr_field( yytext, &yylval->field );
	if ( yylval->field ) {
		prepare_field( yylval );
		return NAME;
	}
	else {
		if ( prepare_value( yylval, yytext, yyleng, 1 ) == SUCCESS ) return VALUE;
		else return OPERAND_ERROR;
	}
}
%%

static void prepare_field(docattr_operand_t* operand)
{
	operand->operand_type = OPERAND_FIELD;
	operand->value_type = operand->field->value_type;
	operand->result = &operand->field->value;

	add_field_to_cond( operand->field );
}

// "" �� �ѷ����� ���� ���ڿ��� �ؼ��� ����
// field name -> integer -> enum -> string ������ �켱������ �ؼ��Ѵ�
static int prepare_value(docattr_operand_t* operand, const char* text, int len, int maybeint)
{
	operand->operand_type = OPERAND_VALUE;
	operand->result = &operand->value;

	if ( len >= sizeof(operand->value.my_string) ) {
		error("string[%s] is too long. max is %d", text, (int) sizeof(operand->value.my_string));
		return FAIL;
	}

	// ����ǥ, escape ����
	if ( text[0] == '"' ) {
		int i;
		char *value_string = operand->value.my_string;

		len -= 2;
		strncpy( value_string, text+1, len );

		for ( i = 0; i < len; i++ ) {
			if ( value_string[i] == '\\' ) {
				len--;
				strncpy( value_string+i, value_string+i+1, len-i );
			}
		}

		value_string[len] = '\0';
	}
	else strcpy( operand->value.my_string, text );

	// ����(or enum)��� �װɷ� OK
	if ( maybeint && isNumber( text, &operand->value.integer ) == SUCCESS ) {
		operand->value_type = VALUE_INTEGER;
		return SUCCESS;
	}

	operand->value_type = VALUE_STRING;
	operand->value.string = operand->value.my_string;
	return SUCCESS;
}

void __yy_scan_string(const char* str)
{
	yy_scan_string( str );
}
