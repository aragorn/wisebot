#ifndef _MOD_QP_GENERAL_H_
#define _MOD_QP_GENERAL_H_ 1

#include "mod_docattr_general.h"

typedef docattr_operand_t* yystype;
#define YYSTYPE yystype // obsolete ... 라고 한다...
#define YYSTYPE_IS_DECLARED 2
#define YYSTYPE_IS_TRIVIAL 1 // 이걸 해야되는 건지 모르겠다...??

extern int __yyparse(void);
extern void __yy_scan_string(const char* str);

#define MAX_OPERAND_NUM 100

extern general_cond_t general_cond;
extern void init_operands();
extern docattr_operand_t* get_new_operand();
extern void add_field_to_cond(docattr_field_t* field);

// expr->exec_func 와 관련있는 것들...
extern int expr_eq_set(docattr_operand_t* operand);
extern int expr_neq_set(docattr_operand_t* operand);
extern int expr_gt_set(docattr_operand_t* operand);
extern int expr_gteq_set(docattr_operand_t* operand);
extern int expr_lt_set(docattr_operand_t* operand);
extern int expr_lteq_set(docattr_operand_t* operand);
extern int expr_bitand_set(docattr_operand_t* operand);
extern int expr_bitor_set(docattr_operand_t* operand);
extern int expr_bitnot_set(docattr_operand_t* operand);

extern int expr_logical_and(docattr_expr_t* expr);
extern int expr_logical_or(docattr_expr_t* expr);
extern int expr_logical_not(docattr_expr_t* expr);

#endif

