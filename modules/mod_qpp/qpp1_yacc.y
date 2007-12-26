/* $Id$ */
%{
#define YYDEBUG 1
#include "common_core.h"
#include "mod_qpp1.h"
#include <stdio.h>
extern int yylex(void);
extern void yyerror(const char* msg);
//extern int yylineno;

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

extern YY_BUFFER_STATE yy_scan_string (const char *yy_str);
extern void yy_delete_buffer (YY_BUFFER_STATE b);

int parse_result = 0;
%}

%token <sval> FIELD
%token <sval> AND OR NOT BOOLEAN
%token <sval> WITHIN
%token <sval> PHRASE NOOP
%token <sval> LPAREN RPAREN
%token <sval> STRING QSTRING 
%token <sval> TEST
%type <node> single_operand
%type <node> expression1 expression2 expression3
%type <node> statement 
%left '+' '-'
%left '*' '/' '&' '(' ')'
%left AND OR
%nonassoc UMINUS
%nonassoc NOT '!' '@'
%union {
	char* sval;
	qpp1_node_t* node;
}

%%


statement:
  expression1 {
	parse_result = 1;
	set_tree($1);
}
;


expression1:
  expression1 '&' expression2  { 
	debug("expression & expression");
	$$ = new_operator(OPERATOR_AND, NULL);
	$$->left  = $1;
	$$->right = $3;
}
| expression1 '!' expression2  {
	debug("expression ! expression");
	$$ = new_operator(OPERATOR_NOT, NULL);
	$$->left  = $1;
	$$->right = $3;
}
| expression1     expression2  {
	debug("expression  expression, default &");
	$$ = new_operator(OPERATOR_AND, NULL);
	$$->left  = $1;
	$$->right = $2;
}
| expression2 {
	debug("expression2");
	$$ = $1;
}
;


expression2:
  expression2 '+' expression3 {
	debug("expression + expression");
}
| expression3 { $$ = $1; }
;

expression3:
  single_operand {
	debug("single_operand");
	$$ = $1;
}
| FIELD single_operand {
	debug("FIELD single_operand");
	$$ = field_operator($2,$1);
}
| '(' expression1 ')' {
	debug("'(' expression1 ')'");
	$$ = $2;
}
| FIELD '(' expression1 ')' {
	debug("FIELD '(' expression1 ')'");
	$$ = field_operator($3,$1);
}
;

single_operand:
  STRING {
	debug("STRING[%s]", $1);
	$$ = new_operand($1);
}
| QSTRING {
	debug("QSTRING[%s]", $1); 
	$$ = new_operand($1);
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


