/* $Id$ */
#define QPP_CORE_PRIVATE 1

#include "stack_test.h"

#include <glib.h>
#include <c-unit/test.h>
#include <string.h>

Int16 nRet=0,i=0;
Stack stack;

gint init_once(autounit_test_t *t) {
	return TRUE;
}

gint setUpStack(autounit_test_t *t) {
	return TRUE;
}
gint tearDownStack(autounit_test_t *t) {
	return TRUE;
}

gint testPushPop(autounit_test_t *t) {
	QueryNode queryNode;

	stk_init(&stack);

	queryNode.operator = QPP_OP_STAR;
	stk_push(&stack,&queryNode);
	stk_pop(&stack,&queryNode);

	au_assert(t,"",queryNode.operator == QPP_OP_STAR);
	return TRUE;
}

gint testMorePushPop(autounit_test_t *t) {
	QueryNode queryNode[10];
	QueryNode tmp;

	queryNode[0].operator = QPP_OP_AND;

	queryNode[1].operator = QPP_OP_OR;
	queryNode[1].num_of_operands = 2;

	queryNode[2].operator = QPP_OP_STAR;
	queryNode[2].num_of_operands = 1;
	queryNode[2].opParam = STAR_BEGIN;

	queryNode[3].operator = QPP_OP_BEG_PHRASE;

	queryNode[4].operator = -1;
	queryNode[4].wordid = 4;

	queryNode[5].operator = QPP_OP_END_PHRASE;

	queryNode[6].operator = -1;
	queryNode[6].wordid = 6;


	stk_init(&stack);

	for (i = 0; i<7; i++)
		stk_push(&stack,&(queryNode[i]) );
	
	stk_pop(&stack,&tmp);
	au_assert(t,"",tmp.operator == -1);
	au_assert(t,"",tmp.wordid == 6);

	stk_pop(&stack,&tmp);
	au_assert(t,"",tmp.operator == QPP_OP_END_PHRASE);

	stk_pop(&stack,&tmp);
	au_assert(t,"",tmp.operator == -1);
	au_assert(t,"",tmp.wordid == 4);

	stk_pop(&stack,&tmp);
	au_assert(t,"",tmp.operator == QPP_OP_BEG_PHRASE);


	stk_pop(&stack,&tmp);
	au_assert(t,"",tmp.operator == QPP_OP_STAR);
	au_assert(t,"",tmp.num_of_operands == 1);
	au_assert(t,"",tmp.opParam == STAR_BEGIN);

	stk_pop(&stack,&tmp);
	au_assert(t,"",tmp.operator == QPP_OP_OR);
	au_assert(t,"",tmp.num_of_operands == 2);

	stk_pop(&stack,&tmp);
	au_assert(t,"",tmp.operator == QPP_OP_AND);

	nRet = stk_pop(&stack,&tmp);
	au_assert(t,"",nRet == STACK_UNDERFLOW);
	return TRUE;
}

gint testInit(autounit_test_t *t) {
	nRet = stk_init(&stack);
	au_assert(t,"",nRet == SUCCESS);
	return TRUE;
}

gint testMove(autounit_test_t *t) {
	Stack stkFrom,stkTo;
	QueryNode tmp;

	stk_init(&stkFrom);
	stk_init(&stkTo);

	tmp.operator = QPP_OP_OPERAND;
	stk_push(&stkFrom,&tmp);	

	tmp.operator = QPP_OP_AND;
	stk_push(&stkFrom,&tmp);	

	tmp.operator = QPP_OP_BEG_PAREN;
	stk_push(&stkFrom,&tmp);	

	tmp.operator = QPP_OP_OR;
	stk_push(&stkFrom,&tmp);	

	tmp.operator = QPP_OP_AND;
	stk_push(&stkFrom,&tmp);	

	stk_moveTillParen(&stkFrom,&stkTo);

	au_assert(t,"",stkTo.queryNodes[0].operator == QPP_OP_AND);
	au_assert(t,"",stkTo.queryNodes[1].operator == QPP_OP_OR);
	au_assert(t,"",stkTo.index == 2);
	return TRUE;
}

gint testMove2(autounit_test_t *t) {
	Stack stkFrom,stkTo;
	QueryNode tmp;

	stk_init(&stkFrom);
	stk_init(&stkTo);

	tmp.operator = QPP_OP_OR;
	stk_push(&stkFrom,&tmp);	

	tmp.operator = QPP_OP_AND;
	stk_push(&stkFrom,&tmp);	

	tmp.operator = QPP_OP_NEAR;
	stk_push(&stkFrom,&tmp);	

	stk_moveTillParen(&stkFrom,&stkTo);

	au_assert(t,"",stkTo.queryNodes[0].operator == QPP_OP_NEAR);
	au_assert(t,"",stkTo.queryNodes[1].operator == QPP_OP_AND);
	au_assert(t,"",stkTo.queryNodes[2].operator == QPP_OP_OR);
	au_assert(t,"",stkTo.index == 3);
	
	return TRUE;
}

gint testErrors(autounit_test_t *t) {
	QueryNode tmp;

	stk_init(&stack);

	nRet = stk_pop(&stack,&tmp);

	au_assert(t,"",nRet == STACK_UNDERFLOW);
	return TRUE;
}

typedef struct {
	gboolean forking;
	char *name;
	autounit_test_fp_t test_fp;
	gboolean isEnabled;
} test_link_t;

static test_link_t tests[] = {
	{FORK,"",testInit,TRUE},
	{FORK,"",testPushPop,TRUE},
	{FORK,"",testMorePushPop,TRUE},
	{FORK,"",testMove,TRUE},
	{FORK,"",testMove2,TRUE},
	{FORK,"",testErrors,TRUE},
	{FORK,0, 0, FALSE}
};

int
main() {
	autounit_testcase_t *test_stack;
	int test_no;
	gint result;
	autounit_test_t *tmp_test;

	test_stack = 
		au_new_testcase(g_string_new("stack_for_qpp testcase"),
						setUpStack,tearDownStack);

	test_no = 0;
	while (tests[test_no].name != 0) {
		if (tests[test_no].isEnabled == TRUE) {
			tmp_test = au_new_test(g_string_new(tests[test_no].name),
								tests[test_no].test_fp);
			au_set_fork_mode(tmp_test,tests[test_no].forking);
			au_add_test(test_stack,tmp_test);
		}
		else {
			printf("?! '%s' test disabled\n",tests[test_no].name);
		}
		test_no++;
	}

	result = au_run_testcase(test_stack);
	return 0;
}
