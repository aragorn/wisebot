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
	yylval->result = &yylval->o.expr.result; \
	strncpy( yylval->o.expr.operator_string, yytext, sizeof(yylval->o.expr.operator_string) );

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

	// field 이름과 같은 게 있으면 NAME 아니면 VALUE
	return_docattr_field( yytext, &yylval->o.field );
	if ( yylval->o.field ) {
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
	operand->value_type = operand->o.field->value_type;
	operand->result = &operand->o.field->value;

	add_field_to_cond( operand->o.field );
}

// "" 로 둘러싸지 않은 문자열을 해석할 때는
// field name -> integer -> enum -> string 순서의 우선순위로 해석한다
static int prepare_value(docattr_operand_t* operand, const char* text, int len, int maybeint)
{
	operand->operand_type = OPERAND_VALUE;
	operand->result = &operand->o.value;
	operand->exec_func = operand_value_exec_func;

	if ( len >= sizeof(operand->o.value.my_string) ) {
		error("string[%s] is too long. max is %d", text, (int) sizeof(operand->o.value.my_string));
		return FAIL;
	}

	// 따옴표, escape 제거
	if ( text[0] == '"' ) {
		int i;
		char *value_string = operand->o.value.my_string;

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
	else strcpy( operand->o.value.my_string, text );

	// 숫자(or enum)라면 그걸로 OK
	if ( maybeint && isNumber( text, &operand->o.value.v.integer ) == SUCCESS ) {
		operand->value_type = VALUE_INTEGER;
		return SUCCESS;
	}

	operand->value_type = VALUE_STRING;
	operand->o.value.v.string = operand->o.value.my_string;
	return SUCCESS;
}

void __yy_scan_string(const char* str)
{
	yy_scan_string( str );
}

