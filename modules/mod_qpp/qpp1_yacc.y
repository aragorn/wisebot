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

statement:
		search_exp             { INFO("search_exp"); }
	|	/* empty statement */  { debug("empty input"); }
	;

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
	|	FIELD ':' '(' search_exp ')' { debug("FIELD: infield_exp"); }
	|	search_exp '&' search_exp  { debug("another search_term"); }
	|	'(' search_exp ')'     { debug("( search_exp )"); }
	|	search_term            { debug("search_term"); }
	;
	/*
	|	STRING                 { debug("got STRING"); }
	|	QSTRING                { debug("got QSTRING"); }
	|	'*'                    { debug("got *"); }
	;
	*/

  /*
infield_exp:
		infield_exp search_term { debug(" another search_term"); }
	|	search_term            { debug("search_term"); }
	;
  */

search_term:
		STRING                 { debug("infield STRING"); }
	|	QSTRING                { debug("infield QSTRING"); }
	|	'*'                    { debug("got *"); }
	;

 /*
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

