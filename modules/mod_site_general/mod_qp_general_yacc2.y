%token EQ NEQ GT GT_EQ LT LT_EQ BIT_AND BIT_OR BIT_NOT
%token PLUS MINUS MULTIPLY DIVIDE
%token LOGICAL_AND LOGICAL_OR LOGICAL_NOT
%token LPAREN RPAREN NAME VALUE NOT
%token OPERAND_ERROR
%token LLIST RLIST COMMA IN COMMON
%left LOGICAL_OR
%left LOGICAL_AND
%nonassoc LOGICAL_NOT
%left COMMA
%left PLUS MINUS
%left MULTIPLY DIVIDE
%left BIT_OR
%left BIT_AND
%nonassoc BIT_NOT
%{
#include "common_core.h"
#include "mod_qp_general2.h"

#define yyerror __yyerror
void __yyerror(char* msg);
extern int yylex(void);

%}
%%

logi_expr:  logi_expr LOGICAL_AND logi_expr {
				parser_result.root_operand = $2;
				$$ = $2;
				$$->value_type = VALUE_BOOLEAN;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				$$->o.expr.exec_func = expr_logical_and;
			}
			| logi_expr LOGICAL_OR logi_expr {
				parser_result.root_operand = $2;
				$$ = $2;
				$$->value_type = VALUE_BOOLEAN;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				$$->o.expr.exec_func = expr_logical_or;
			}
			| LOGICAL_NOT logi_expr {
				parser_result.root_operand = $1;
				$$ = $1;
				$$->value_type = VALUE_BOOLEAN;
				$$->o.expr.operand1 = $2;
				$$->o.expr.exec_func = expr_logical_not;
			}
			| LPAREN logi_expr RPAREN { $$ = $2; }
			| cmp_expr { parser_result.root_operand = $1; $$ = $1; }
			;

cmp_expr:	calc_expr EQ calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_eq_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr NEQ calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_neq_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr GT calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_gt_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr GT_EQ calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_gteq_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr LT calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_lt_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr LT_EQ calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_lteq_set($$) != SUCCESS ) YYERROR;
			}
			| list IN list {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_in_set($$) != SUCCESS ) YYERROR;
			}
			| list COMMON list {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_common_set($$) != SUCCESS ) YYERROR;
			}
			;

list:	LLIST _list RLIST { $$ = $2; }
		| calc_expr {
			$$ = get_new_operand();
			if ( $$ == NULL ) {
				error("error while making list");
				YYERROR;
			}

			$$->operand_type = OPERAND_LIST;
			$$->value_type = VALUE_LIST;
			$$->o.list = get_new_list();
			if ( $$->o.list == NULL ) {
				error("error while making list");
				YYERROR;
			}
			$$->result = NULL; // obsolete. already NULL;

			append_to_list($$->o.list, $1);
		}
		;

_list:	_list COMMA calc_expr {
			$$ = $1;
			append_to_list($$->o.list, $3);
		}
		| calc_expr {
			$$ = get_new_operand();
			if ( $$ == NULL ) {
				error("error while making list");
				YYERROR;
			}

			$$->operand_type = OPERAND_LIST;
			$$->value_type = VALUE_LIST;
			$$->o.list = get_new_list();
			if ( $$->o.list == NULL ) {
				error("error while making list");
				YYERROR;
			}
			$$->result = NULL; // obsolete. already NULL;

			append_to_list($$->o.list, $1);
		}
		;

calc_expr:	calc_expr BIT_AND calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_bitand_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr BIT_OR calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_bitor_set($$) != SUCCESS ) YYERROR;
			}
			| BIT_NOT calc_expr {
				$$ = $1;
				$$->o.expr.operand1 = $2;
				if ( expr_bitnot_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr PLUS calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_plus_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr MINUS calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_minus_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr MULTIPLY calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_multiply_set($$) != SUCCESS ) YYERROR;
			}
			| calc_expr DIVIDE calc_expr {
				$$ = $2;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				if ( expr_divide_set($$) != SUCCESS ) YYERROR;
			}
			| LPAREN calc_expr RPAREN { $$ = $2; }
			| operand { $$ = $1; }
			;

operand: NAME { $$ = $1; }
	   | VALUE { $$ = $1; }
	   | OPERAND_ERROR	{ YYERROR; }
	   ;

%%

void __yyerror(char* s)
{
	error("%s\n", s);
}

int __yyparse()
{
	return yyparse();
}

