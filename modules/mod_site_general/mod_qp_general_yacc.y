%token EQ NEQ GT GT_EQ LT LT_EQ BIT_AND BIT_OR BIT_NOT
%token LOGICAL_AND LOGICAL_OR LOGICAL_NOT
%token LPAREN RPAREN NAME VALUE NOT
%token OPERAND_ERROR
%left LOGICAL_OR
%left LOGICAL_AND
%nonassoc LOGICAL_NOT
%left BIT_OR
%left BIT_AND
%nonassoc BIT_NOT
%{
#include "mod_docattr_general.h"
#include "mod_qp_general.h"

#define yyerror __yyerror
void __yyerror(char* msg);
extern int yylex(void);

%}
%%

logi_expr:  logi_expr LOGICAL_AND logi_expr {
				general_cond.root_operand = $2;
				$$ = $2;
				$$->value_type = VALUE_BOOLEAN;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				$$->o.expr.exec_func = expr_logical_and;
			}
			| logi_expr LOGICAL_OR logi_expr {
				general_cond.root_operand = $2;
				$$ = $2;
				$$->value_type = VALUE_BOOLEAN;
				$$->o.expr.operand1 = $1;
				$$->o.expr.operand2 = $3;
				$$->o.expr.exec_func = expr_logical_or;
			}
			| LOGICAL_NOT logi_expr {
				general_cond.root_operand = $1;
				$$ = $1;
				$$->value_type = VALUE_BOOLEAN;
				$$->o.expr.operand1 = $2;
				$$->o.expr.exec_func = expr_logical_not;
			}
			| LPAREN logi_expr RPAREN { $$ = $2; }
			| cmp_expr { general_cond.root_operand = $1; $$ = $1; }
			;

cmp_expr: calc_expr EQ calc_expr {
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
			| LPAREN calc_expr RPAREN { $$ = $2; }
			| operand { $$ = $1; }
			;

operand:  NAME { $$ = $1; }
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

