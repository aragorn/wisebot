%{
/* $Id$ */
#define YYDEBUG 1
#include "common_core.h"
#include <stdio.h>
extern int yylex(void);
extern void yyerror(const char* msg);
extern int yylineno;
int num_lines = 0;
int num_chars = 0;
%}

%token SELECT SEARCH WHERE LIMIT BY
%token GROUP COUNT GROUP_BY COUNT_BY VIRTUAL_ID
%token ORDER ORDER_BY ASC DESC
%token IN IS NULLX COMPARISON BETWEEN
%token FIELD TERM
%token AND OR NOT BOOLEAN
%token WITHIN
%token PHRASE NOOP
%token LPAREN RPAREN
%token STRING NAME INTNUM
%token FUNCTION_NAME NON_EMPTY
%left '+' '-'
%left '*' '/' '&' '(' ')'
%left AND OR
%nonassoc UMINUS
%nonassoc NOT '!' '@'

%%

sql_list:
		sql ';'                { NOTICE("single sql"); }
		sql_list sql ';'       { NOTICE("sql_list sql ;"); }
	;

sql:
	 	simple_statement       { debug("SEARCH search_exp"); }
	;

simple_statement:
		SEARCH search_exp      { debug("SEARCH search_exp"); }
	;

sql:
		search_statement       { debug("search_statement"); }
	;

search_statement:
        SELECT selection
		SEARCH opt_search_exp
		opt_where_clause
		opt_order_by_clause
		opt_limit_clause
	;

selection:
		field_commalist
	/* 	scalar_exp_commalist */
    |   '*'
	;

field_commalist:
		field_ref
	|	field_commalist ',' field_ref
	;

opt_limit_clause:
		/* empty */
	|	LIMIT INTNUM
	|	LIMIT INTNUM ',' INTNUM
	;

opt_order_by_clause:
		/* empty */
	|	ORDER BY ordering_spec_commalist
	|	ORDER_BY ordering_spec_commalist
	;

ordering_spec_commalist:
		ordering_spec
	|	ordering_spec_commalist ',' ordering_spec
	;

ordering_spec:
		field_ref opt_asc_desc
	;

opt_asc_desc:
		/* empty */
	|	ASC
	|	DESC
	;

opt_where_clause:
		/* empty */
	|	WHERE where_condition
	;

where_condition:
		/* empty */
	|	where_condition OR where_condition
	|	where_condition AND where_condition
	|	NOT where_condition
	|	'(' where_condition ')'
	|	predicate
	;

predicate:
		comparison_predicate
	|	between_predicate
	|	test_for_null
	|	in_predicate
	;

comparison_predicate:
		scalar_exp COMPARISON scalar_exp
	;

between_predicate:
		scalar_exp NOT BETWEEN scalar_exp AND scalar_exp
	|	scalar_exp BETWEEN scalar_exp AND scalar_exp
	;

test_for_null:
		field_ref IS NOT NULLX
	|	field_ref IS NULLX
	;

in_predicate:
		scalar_exp NOT IN '(' atom_commalist ')'
	|	scalar_exp IN '(' atom_commalist ')'
	;

atom_commalist:
		atom
	|	atom_commalist ',' atom
	;

opt_search_exp:
		/* empty */
	|	search_exp
	;

search_exp:
		search_term
	|	'(' search_exp ')'
	|	search_exp '&' search_exp
	|	search_exp '/' INTNUM '/' search_exp
	|	search_exp '+' search_exp
	|	search_term search_term
	|	search_term '(' search_exp ')'
	|	'(' search_exp ')' search_term
	|	'(' search_exp ')' '(' search_exp ')'
	|	'!' search_exp %prec UMINUS
	|	'@' search_exp %prec UMINUS
	|	field_ref ':' search_term
	|	field_ref ':' '(' search_exp ')'
	;

search_term:
		TERM                  { debug("this is a term"); }
	|	STRING                { debug("this is a string"); }
	|	NAME                  { debug("this is a name"); }
	;

scalar_exp:
		scalar_exp '+' scalar_exp
	|	scalar_exp '-' scalar_exp
	|	scalar_exp '*' scalar_exp
	|	scalar_exp '/' scalar_exp
	|	'+' scalar_exp %prec UMINUS
	|	'-' scalar_exp %prec UMINUS
	|	atom
	|	field_ref
	|	function_ref
	|	'(' scalar_exp ')'
	;

	/*
scalar_exp_commalist:
		scalar_exp
	|	scalar_exp_commalist ',' scalar_exp
	;
	*/

function_ref:
		FUNCTION_NAME '(' '*' ')' /* COUNT(*) */
	|	FUNCTION_NAME '(' scalar_exp ')'
	;

atom:
		literal
	;

literal:
		STRING
	|	INTNUM
	;

field_ref:
		NAME
	;

sql:
		test_statement         { debug("test_statement"); }
	;

test_statement:
		TEST NAME              { debug("TEST NAME"); }
	;

%%


int main(int argc, char *argv[])
{
	if (argc > 1) yydebug = 1;
    yyparse();
	printf("lines[%d] chars[%d]\n", yylineno, num_chars);
	
    return 0;
}
