/* $Id$ */
#include "softbot.h"
#include "mod_qp.h"

#include <glib.h>
#include <c-unit/test.h>
#include <string.h>

gint init_testcase_eachtime(autounit_test_t *t) {
	init_free_list();
	return TRUE;
}

typedef struct {
	gboolean forking;
	char *name;
	autounit_test_fp_t test_fp;
	gboolean isEnabled;
} test_link_t;

gint test_query_handle(autounit_test_t *t) {
	request r;
	handler *h;

	r.type = NO_HANDLER;
	h = get_handler(&r);

	au_assert(t,"",h==NULL);

	return TRUE;
}

gint test_stack_operation(autounit_test_t *t) {
	sb_stack_t stack;
	index_list_t pushed1,pushed2;
	index_list_t *poped1,*poped2;

	init_stack(&stack);

	pushed1.ndocs = 1;
	stack_push(&stack,&pushed1);

	au_assert(t,"",stack.first->ndocs == 1);
	au_assert(t,"",stack.last->ndocs == 1);
	au_assert(t,"",stack.first->next == NULL);
	au_assert(t,"",stack.first->prev == NULL);

	pushed2.ndocs = 2;
	stack_push(&stack,&pushed2);

	au_assert(t,"",stack.first->ndocs == 1);
	au_assert(t,"",stack.last->ndocs == 2);
	au_assert(t,"",stack.first->next == &pushed2);
	au_assert(t,"",stack.last->next == NULL);
	au_assert(t,"",stack.last->prev == &pushed1);

	poped2 = stack_pop(&stack);

	au_assert(t,"",poped2->ndocs == 2);
	au_assert(t,"",stack.first->ndocs == 1);
	au_assert(t,"",stack.last->ndocs == 1);
	au_assert(t,"",stack.last->next == NULL);
	au_assert(t,"",stack.first->prev == NULL);
	au_assert(t,"",stack.last == stack.first);
	au_assert(t,"",stack.first == &pushed1);
	
	poped1 = stack_pop(&stack);

	au_assert(t,"",poped1->ndocs == 1);
	au_assert(t,"",stack.first == NULL);
	au_assert(t,"",stack.last == NULL);

	return TRUE;
}
gint test_hit_bitposition(autounit_test_t *t) {
	hit_t hit;

	hit.std_hit.type = 0;
	hit.std_hit.field = 7;
	hit.std_hit.position = 4095;

	au_assert(t,"",hit.ext_hit.type == 0);

	return TRUE;
}
gint test_operator_and(autounit_test_t *t) {
	sb_stack_t stack;
	doc_hit_t dochit1[2],dochit2[2];
	uint32_t relevancy1[2],relevancy2[2];
	index_list_t *result,operand1,operand2;
	QueryNode node;

	node.num_of_operands = 2;
	node.operator = QPP_OP_AND;

	operand1.doc_hits = dochit1;
	operand2.doc_hits = dochit2;
	operand1.relevancy = relevancy1;
	operand2.relevancy = relevancy2;
	operand1.ndocs = 2;
	operand2.ndocs = 2;

	dochit1[0].docid = 1;
	dochit1[0].nhits = 1;
	dochit1[0].field = 1;
	dochit1[0].hits[0].std_hit.type = 0;
	dochit1[0].hits[0].std_hit.field = 1;
	dochit1[0].hits[0].std_hit.position = 1;

	dochit1[1].docid = 2;
	dochit1[1].nhits = 2;
	dochit1[1].field = 2;
	dochit1[1].hits[0].std_hit.type = 0;
	dochit1[1].hits[0].std_hit.field = 2;
	dochit1[1].hits[0].std_hit.position = 1;

	dochit2[0].docid = 2;
	dochit2[0].nhits = 2;
	dochit2[0].field = 2;
	dochit2[0].hits[0].std_hit.type = 0;
	dochit2[0].hits[0].std_hit.field = 2;
	dochit2[0].hits[0].std_hit.position = 2;

	dochit2[1].docid = 3;
	dochit2[1].nhits = 3;
	dochit2[1].field = 3;
	dochit2[1].hits[0].std_hit.type = 0;
	dochit2[1].hits[0].std_hit.field = 2;
	dochit2[1].hits[0].std_hit.position = 2;

	init_stack(&stack);
	stack_push(&stack,&operand1);
	stack_push(&stack,&operand2);

	operator_and(&stack,&node);
	result = stack_pop(&stack);

	au_assert(t,"",result->ndocs == 1);
	au_assert(t,"",result->doc_hits[0].docid== 2);

	return TRUE;
}
void debug_print_result(index_list_t *result_list)
{
	int i;
	debug("number of searched documents: %ld",result_list->ndocs);
	debug("result: [did] [rel]");
	for ( i = 0; i < result_list->ndocs && i < 20; i++ ){
		debug("  %ld  [%ld] [%ld]", i,
				result_list->doc_hits[i].docid,
				result_list->relevancy[i]);
	}
}
gint test_sort(autounit_test_t *t)
{
	index_list_t unsorted;
	uint32_t relevancy[10] = {7,6,4,8,2,4,1,3,5,5};
	doc_hit_t doc_hits[10];

	doc_hits[0].docid = 1;
	doc_hits[1].docid = 2;
	doc_hits[2].docid = 3;
	doc_hits[3].docid = 4;
	doc_hits[4].docid = 5;
	doc_hits[5].docid = 6;
	doc_hits[6].docid = 7;
	doc_hits[7].docid = 8;
	doc_hits[8].docid = 9;
	doc_hits[9].docid = 10;

	unsorted.relevancy = relevancy;
	unsorted.doc_hits = doc_hits;
	unsorted.ndocs = 10;

	sort_list(&unsorted);

	au_assert(t,"",unsorted.doc_hits[0].docid == 7);
	au_assert(t,"",unsorted.doc_hits[1].docid == 5);
	au_assert(t,"",unsorted.doc_hits[2].docid == 8);
	au_assert(t,"",unsorted.doc_hits[3].docid == 6 || 
					unsorted.doc_hits[3].docid == 3);
	au_assert(t,"",unsorted.doc_hits[4].docid == 3 || 
					unsorted.doc_hits[4].docid == 6);
	au_assert(t,"",unsorted.doc_hits[5].docid == 10 || 
					unsorted.doc_hits[5].docid == 9);
	au_assert(t,"",unsorted.doc_hits[6].docid == 9 || 
					unsorted.doc_hits[6].docid == 10);
	au_assert(t,"",unsorted.doc_hits[7].docid == 2);
	au_assert(t,"",unsorted.doc_hits[8].docid == 1);
	au_assert(t,"",unsorted.doc_hits[9].docid == 4);

	au_assert(t,"",unsorted.relevancy[0] == 1);
	au_assert(t,"",unsorted.relevancy[1] == 2);
	au_assert(t,"",unsorted.relevancy[2] == 3);
	au_assert(t,"",unsorted.relevancy[3] == 4);
	au_assert(t,"",unsorted.relevancy[4] == 4);
	au_assert(t,"",unsorted.relevancy[5] == 5);
	au_assert(t,"",unsorted.relevancy[6] == 5);
	au_assert(t,"",unsorted.relevancy[7] == 6);
	au_assert(t,"",unsorted.relevancy[8] == 7);
	au_assert(t,"",unsorted.relevancy[9] == 8);
/*	debug_print_result(&unsorted);*/
	return TRUE;
}
static test_link_t tests[] = {
	{FORK,"stack operation test",test_stack_operation,TRUE},
	{FORK,"standard_hit_t bit field test",test_hit_bitposition,TRUE},
	{FORK,"test non existing query handling",test_query_handle,TRUE},
	{FORK,"AND operator test",test_operator_and,TRUE},
	{FORK,"sort test",test_sort,TRUE},
	{FORK_NOT,0, 0, FALSE}
};

int 
main() {
	autounit_testcase_t *mod_qp_test;
	int test_no;
	gint result;
	autounit_test_t *tmp_test;

	mod_qp_test =
		au_new_testcase(g_string_new("module qp testcase"),
								init_testcase_eachtime,NULL);

	test_no = 0;
	while (tests[test_no].name != 0) {
		if (tests[test_no].isEnabled == TRUE) {
			tmp_test = au_new_test(g_string_new(tests[test_no].name),
								tests[test_no].test_fp);
			au_set_fork_mode(tmp_test,tests[test_no].forking);
			au_add_test(mod_qp_test,tmp_test);
		}
		else {
			printf("?! '%s' test disabled\n",tests[test_no].name);
		}
		test_no++;
	}

	result = au_run_testcase(mod_qp_test);
	return result;
}
