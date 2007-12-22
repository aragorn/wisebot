/* $Id$ */
%{
#define YYDEBUG 1
#include "common_core.h"
#include <stdio.h>
extern int yylex(void);
extern void yyerror(const char* msg);
extern int yylineno;
//extern YY_BUFFER_STATE yy_scan_string (yyconst char *yy_str);
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


 /*
search_exp:
		search_exp '+' search_exp2 { debug("search_exp + search_exp2"); }
	|	search_exp2                { debug("single search_exp2"); }
	;

		
search_exp2:
		search_exp2 '&' search_term { debug("search_exp2 & search_term"); }
	|	search_exp2     search_term { debug("search_exp2 search_term"); }
	|	search_term
	;
 */

search_exp:
		FIELD ':' STRING      { debug("FIELD: STRING"); }
	|	FIELD ':' QSTRING     { debug("FIELD: QSTRING"); }
	|	STRING                 { debug("got STRING"); }
	|	QSTRING                { debug("got QSTRING"); }
	;

  /*
	|	FIELD ':' infield_exp { debug("FIELD: infield_exp"); }
infield_exp:
		'(' infield_exp ')'    { debug("( infield_exp )"); }
	|	STRING                 { debug("got STRING"); }
	|	QSTRING                { debug("got QSTRING"); }
	;

	|	search_exp2 '/' INTNUM '/' search_term

search_term:
		'(' search_exp ')'
	|	'!' search_exp %prec UMINUS
	|	'@' search_exp %prec UMINUS
	|	field_ref ':' STRING  { debug("field_ref : STRING"); }
	|	field_ref ':' QSTRING { debug("field_ref : QSTRING"); }
	|	field_ref ':' '(' search_exp ')'
	;
  */

%%

int qpp1_parse(char *input, int debug)
{
	yydebug = debug;
	yy_scan_string(input);
	yyparse();
	printf("lines[%d] chars[%d]\n", yylineno, -1);

	return SUCCESS;
}

int main(int argc, char *argv[])
{
	if (argc > 1) yydebug = 1;
    yyparse();
	printf("lines[%d] chars[%d]\n", yylineno, -1);
	
    return 0;
}

