%token EQ NEQ GT GT_EQ LT LT_EQ BIT_AND BIT_OR BIT_NOT
%token LOGICAL_AND LOGICAL_OR LOGICAL_NOT
%token LPAREN RPAREN NAME VALUE NOT
%token OPERAND_ERROR
%left LOGICAL_OR
%left LOGICAL_AND
%nonassoc LOGICAL_NOT
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
				$$->expr.operand1 = $1;
				$$->expr.operand2 = $3;
				$$->expr.exec_func = expr_logical_and;
			}
			| logi_expr LOGICAL_OR logi_expr {
				general_cond.root_operand = $2;
				$$ = $2;
				$$->expr.operand1 = $1;
				$$->expr.operand2 = $3;
				$$->expr.exec_func = expr_logical_or;
			}
			| LOGICAL_NOT logi_expr {
				general_cond.root_operand = $1;
				$$ = $1;
				$$->expr.operand1 = $2;
				$$->expr.exec_func = expr_logical_not;
			}
			| LPAREN logi_expr RPAREN { $$ = $2; }
			| cmp_expr { general_cond.root_operand = $1; $$ = $1; }
			;

cmp_expr: operand EQ operand {
			$$ = $2;
			$$->expr.operand1 = $1;
			$$->expr.operand2 = $3;
			if ( expr_eq_set($$) != SUCCESS ) YYERROR;
		}
		| operand NEQ operand {
			$$ = $2;
			$$->expr.operand1 = $1;
			$$->expr.operand2 = $3;
			if ( expr_neq_set($$) != SUCCESS ) YYERROR;
		}
		| operand GT operand {
			$$ = $2;
			$$->expr.operand1 = $1;
			$$->expr.operand2 = $3;
			if ( expr_gt_set($$) != SUCCESS ) YYERROR;
		}
		| operand GT_EQ operand {
			$$ = $2;
			$$->expr.operand1 = $1;
			$$->expr.operand2 = $3;
			if ( expr_gteq_set($$) != SUCCESS ) YYERROR;
		}
		| operand LT operand {
			$$ = $2;
			$$->expr.operand1 = $1;
			$$->expr.operand2 = $3;
			if ( expr_lt_set($$) != SUCCESS ) YYERROR;
		}
		| operand LT_EQ operand {
			$$ = $2;
			$$->expr.operand1 = $1;
			$$->expr.operand2 = $3;
			if ( expr_lteq_set($$) != SUCCESS ) YYERROR;
		}
		| operand BIT_AND operand {
			$$ = $2;
			$$->expr.operand1 = $1;
			$$->expr.operand2 = $3;
			if ( expr_bitand_set($$) != SUCCESS ) YYERROR;
		}
		| operand BIT_OR operand {
			$$ = $2;
			$$->expr.operand1 = $1;
			$$->expr.operand2 = $3;
			if ( expr_bitor_set($$) != SUCCESS ) YYERROR;
		}
		| operand BIT_NOT operand { error("bit not operator is not supported"); YYERROR; }
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

