%{
/* $Id$ */
#define YYDEBUG 1
#include "common_core.h"
#include <stdio.h>
extern int yylex(void);
extern void yyerror(const char* msg);
extern int yylineno;
%}

%token SEARCH TEST INTNUM COMPARISON NAME

%%

sql_list:
		sql ';'                { NOTICE("single sql"); }
	|	sql_list sql ';'       { NOTICE("sql_list sql ;"); }
	;

sql:
	 	simple_statement       { debug("SEARCH search_exp"); }
	|	expression             { NOTICE("= %d", $1); }
	|	error                  { warn("sql parse error"); }
	;

simple_statement:
		SEARCH                 { debug("SEARCH"); }
	|	TEST                   { debug("TEST input"); }
	;

expression:
		expression '+' INTNUM  { $$ = $1 + $3; debug("add"); }
	|	expression '-' INTNUM  { $$ = $1 - $3; debug("minus"); }
	|	INTNUM                 { debug("INTNUM = %d", $1); }

%%

int main(int argc, char *argv[])
{
	if (argc > 1) yydebug = 1;
    yyparse();
	printf("lines[%d]\n", yylineno);
	
    return 0;
}
