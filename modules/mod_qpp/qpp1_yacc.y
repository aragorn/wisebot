/* $Id$ */
%{
#define YYDEBUG 1
#include "common_core.h"
#include <stdio.h>
extern int yylex(void);
extern void yyerror(const char* msg);
extern int yylineno;
//extern YY_BUFFER_STATE yy_scan_string (yyconst char *yy_str);

int parse_result = 0;
%}

%token FIELD
%token AND OR NOT BOOLEAN
%token WITHIN
%token PHRASE NOOP
%token LPAREN RPAREN
%token STRING QSTRING 
%token TEST
%left '+' '-'
%left '*' '/' '&' '(' ')'
%left AND OR
%nonassoc UMINUS
%nonassoc NOT '!' '@'

%%


statement_list:
	 	statement                   { parse_result = 1; }
    ;

statement:
	    expression1                 { debug("hello"); }
    ;


expression1:
	   FIELD ':' expression2       { debug("FIELD ':' expression2"); }
    |  expression1 '&' expression2  { debug("expression & expression"); }
    |  expression1 '!' expression2  { debug("expression ! expression"); }
	|  expression2                 { debug("expression2"); }
	;


expression2:
        expression2 '+' expression3 { debug("expression + expression"); }
    |   expression2 expression3     { debug("expression  expression, default &"); }
	|   expression3                 { debug("expression3"); }
	;

expression3:
	   primary_expression
	|  '(' statement_list ')'
	;

primary_expression:
	 	STRING                 { debug("STRING"); }
	|	QSTRING                { debug("QSTRING"); }
	;


%%

int qpp1_parse(char *input, int debug)
{
	yydebug = debug;
	yy_scan_string(input);
	parse_result = 0;
	yyparse();
	if (parse_result == 0) error("cannot parse query[%s]", input);

	return parse_result;
}

int main(int argc, char *argv[])
{
	if (argc > 1) yydebug = 1;
    yyparse();
	printf("lines[%d] chars[%d]\n", yylineno, -1);
	
    return 0;
}

