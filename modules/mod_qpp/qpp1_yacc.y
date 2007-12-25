/* $Id$ */
%{
#define YYDEBUG 1
#include "common_core.h"
#include "mod_qpp1.h"
#include "qpp1_lex.h"
#include <stdio.h>
//extern int yylex(void);
extern void yyerror(const char* msg);
//extern int yylineno;
//extern YY_BUFFER_STATE yy_scan_string (yyconst char *yy_str);

int parse_result = 0;
%}

%token <sval> FIELD
%token <sval> AND OR NOT BOOLEAN
%token <sval> WITHIN
%token <sval> PHRASE NOOP
%token <sval> LPAREN RPAREN
%token <sval> STRING QSTRING 
%token <sval> TEST
%left '+' '-'
%left '*' '/' '&' '(' ')'
%left AND OR
%nonassoc UMINUS
%nonassoc NOT '!' '@'
%union {
	char *sval;
	/*qpp1_node_t* node;*/
}

%%


statement_list:
	 	statement                    { parse_result = 1; }
    ;

statement:
		expression1                  { debug("exp1"); }
    ;


expression1:
		expression1 '&' expression2  { debug("expression & expression"); }
	|	expression1 '!' expression2  { debug("expression ! expression"); }
	|	expression2                  { debug("expression2"); }
	;


expression2:
		expression2 '+' expression3 { debug("expression + expression"); }
	|	expression2 expression3     { debug("expression  expression, default &"); }
	|	expression3                 { debug("expression3"); }
	;

expression3:
		single_operand               { debug("single_operand"); }
	|	'(' statement_list ')'       { debug("'(' statement_list ')'"); }
	|	FIELD single_operand         { debug("FIELD single_operand"); }
	|	FIELD '(' statement_list ')' { debug("FIELD '(' statement_list ')'"); }
	;

single_operand:
		STRING                      { 
	debug("STRING[%s]", $1);
	push_operand($1);
	}
	|	QSTRING                     {
	debug("QSTRING[%s]", $1); 
	push_operand($1);
	}
	;


%%

int qpp1_yyparse(char *input, int debug)
{
	YY_BUFFER_STATE yy_bs;

	yydebug = debug;
	yy_bs = yy_scan_string(input);

	parse_result = 0;
	yyparse();
	if (parse_result == 0) error("cannot parse query[%s]", input);

	yy_delete_buffer(yy_bs);

	return parse_result;
}


