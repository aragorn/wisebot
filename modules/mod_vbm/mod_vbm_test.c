/* $Id$ */
#include "softbot.h"
#include "mod_api/mod_vbm.h"

#include <glib.h>
#include <c-unit/test.h>
#include <string.h>

gint init_testcase(autounit_test_t *t) {
/*	SP_setPath("./parTest");*/
	return TRUE;
}

gint test_VBM_init(autounit_test_t *t) {
	int nRet;
	
	nRet = VBM_init();
	au_assert(t,"VBM_init error", nRet == SUCCESS);
	return TRUE;
}

gint test_VBM_initBuf(autounit_test_t *t) {
	int nRet;
	VariableBuffer varBuf;

	VBM_init();
	nRet = VBM_initBuf(&varBuf);
	au_assert(t,"VBM_initBuf error, size uninitialized",
				varBuf.size == 0);
///	au_assert(t,"VBM_initBuf error, pFirst uninitialized",
///				varBuf.pFirst != NULL);
///	au_assert(t,"VBM_initBuf error, pLast uninitialized",
///				varBuf.pLast == varBuf.pFirst);
	return TRUE;
}

gint test_VBM_append(autounit_test_t *t) {
	int nRet;
	VariableBuffer varBuf;

	VBM_init();
	nRet = VBM_initBuf(&varBuf);

	nRet = VBM_append(&varBuf,13,"this is test");
	au_assert(t,"VBM_append error, return value error",
				nRet == 13);
	au_assert(t,"VBM_append error, size calculation error",
				varBuf.size == 13);

	nRet = VBM_append(&varBuf,20,"12345678901234567890");
	au_assert(t,"VBM_append error, return value error",
				nRet == 20);
	au_assert(t,"VBM_append error, size calculation error",
				varBuf.size == 33);
	return TRUE;
}

gint test_VBM_appendBuf(autounit_test_t *t) {
	int nRet=0,i=0;
	VariableBuffer varBuf1;
	VariableBuffer varBuf2;
	char tmp[256];
	
	VBM_init();
	for (i=0; i<10; i++) {
		VBM_initBuf(&varBuf1);
		VBM_initBuf(&varBuf2);

		nRet = VBM_append(&varBuf1,14,"this is test1");
		au_assert(t,"VBM_append error, return value error",
					nRet == 14);
		au_assert(t,"VBM_append error, size calculation error",
					varBuf1.size == 14);

		nRet = VBM_append(&varBuf2,14,"this is test2");
		au_assert(t,"VBM_append error, return value error",
					nRet == 14);
		au_assert(t,"VBM_append error, size calculation error",
					varBuf2.size == 14);

		nRet = VBM_appendBuf(&varBuf1,&varBuf2);
		au_assert(t,"VBM_append error, return value error",
					nRet == SUCCESS);

		nRet = VBM_get(&varBuf1,0,14+14,tmp);
		au_assert(t,"VBM_get error, return value error",
					nRet == 14+14);
		au_assert(t,"VBM_get error, data incorrect",
					strcmp(tmp,"this is test1\0this is test2") == 0);
		VBM_freeBuf(&varBuf1);
		VBM_freeBuf(&varBuf2);
	}
	return TRUE;
}

gint test_VBM_get(autounit_test_t *t) {
	int nRet;
	VariableBuffer varBuf;
	char tmp[256];

	VBM_init();
	nRet = VBM_initBuf(&varBuf);
	nRet = VBM_append(&varBuf,15,"this is a test");

	nRet = VBM_get(&varBuf,0,15,(void*)tmp);

	au_assert(t,"VBM_get error, return value error",
				nRet == 15);

	au_assert(t,"VBM_get error, taken-data incorrect",
				strcmp(tmp,"this is a test") == 0);

	nRet = VBM_get(&varBuf,2,13,(void*)tmp);

	au_assert(t,"VBM_get error, return value error",
				nRet == 13);

	au_assert(t,"VBM_get error, taken-data incorrect",
				strcmp(tmp,"is is a test") == 0);
	return TRUE;
}
gint test_VBM_getSize(autounit_test_t *t){
	int nRet;
	VariableBuffer varBuf;
	char tmp[256];

	VBM_init();
	nRet = VBM_initBuf(&varBuf);
	nRet = VBM_append(&varBuf,15,"this is a test");

	nRet = VBM_getSize(&varBuf);

	au_assert(t,"VBM_getSize error, return Value error",
				nRet == 15);
	return TRUE;
}
gint test_overflow(autounit_test_t *t) {
	int nRet;
	VariableBuffer varBuf;
	char tmp[61];

	VBM_init();
	nRet = VBM_initBuf(&varBuf);
	nRet = VBM_append(&varBuf,60,tmp);

	au_assert(t,"overflow test failure",
				nRet == 60);

	nRet = VBM_append(&varBuf,1,tmp+60);

	au_assert(t,"overflow test failure",
				nRet == VBM_INSUFFICIENT_BUF_BLOCK);
	return TRUE;
}
gint test_VBM_freeBuf(autounit_test_t *t) {
	int nRet;
	VariableBuffer varBuf;
	char tmp[60];

	VBM_init();
	nRet = VBM_initBuf(&varBuf);
	nRet = VBM_append(&varBuf,59,tmp);
	VBM_freeBuf(&varBuf);

	nRet = VBM_append(&varBuf,60,tmp);

	au_assert(t,"VBM_freeBuf error, cannot free whole memory",
				nRet == 60);
	return TRUE;
}
gint test_VBM_allornon(autounit_test_t *t) {
	int nRet;
	VariableBuffer varBuf;
	char tmp[60];

	VBM_init();
	nRet = VBM_initBuf(&varBuf);
	nRet = VBM_append(&varBuf,61,tmp);

	au_assert(t,"VBM_append error, return Value error",
				nRet == VBM_INSUFFICIENT_BUF_BLOCK);

	nRet = VBM_append(&varBuf,60,tmp);

	au_assert(t,"VBM_append allornon error, garbage not cleaned",
				nRet = 60);
	return TRUE;
}
gint test_read_write(autounit_test_t *t) {
	int nRet,i;
	VariableBuffer varBuf;
	char tmp[10]="123456789";
	char tmp2[10];

	VBM_init();
	nRet = VBM_initBuf(&varBuf);

	for ( i=0; i<5; i++) {
		nRet = VBM_append(&varBuf,10,tmp);

		au_assert(t,"read/write error append", nRet == 10);

		nRet = VBM_get(&varBuf,0,10,tmp2);

		au_assert(t,"read/write error VBM_get, return Value error", 
					nRet == 10);
		au_assert(t,"read/write error VBM_get, data incorrect", 
					strcmp(tmp2,tmp)==0);
	}
	return TRUE;
}

gint test_small_read_write(autounit_test_t *t) {
	int nRet,i;
	VariableBuffer varBuf;
	char tmp[10]="9";
	char tmp2[10];

	VBM_init();
	nRet = VBM_initBuf(&varBuf);

	for ( i=0; i<10; i++) {
		nRet = VBM_append(&varBuf,2,tmp);

		au_assert(t,"read/write error append", nRet == 2);

		nRet = VBM_get(&varBuf,i*2,2,tmp2);

		au_assert(t,"read/write error VBM_get, return Value error", 
					nRet == 2);
		au_assert(t,"read/write error VBM_get, data incorrect", 
					strcmp(tmp2,tmp)==0);
	}
	return TRUE;
}

typedef struct {
	gboolean forking;
	char *name;
	autounit_test_fp_t test_fp;
	gboolean isEnabled;
} test_link_t;

static test_link_t tests[] = {
	{FORK_NOT,"VBM_init test", test_VBM_init, TRUE},
	{FORK_NOT,"VBM_initBuf test",test_VBM_initBuf,TRUE},
	{FORK_NOT,"VBM_append test",test_VBM_append,TRUE},
	{FORK_NOT,"VBM_get test",test_VBM_get,TRUE},
	{FORK_NOT,"VBM_appendBuf test",test_VBM_appendBuf,TRUE},
	{FORK_NOT,"VBM_getSize test",test_VBM_getSize,TRUE},
	{FORK_NOT,"VBM overflow test",test_overflow,FALSE},
	{FORK_NOT,"VBM_freeBuf test",test_VBM_freeBuf,TRUE},
	{FORK_NOT,"all or none test at VBM_append",test_VBM_allornon,FALSE},
	{FORK_NOT,"test read/write ",test_read_write,TRUE},
	{FORK_NOT,"test small read/write ",test_small_read_write,TRUE},
	{FORK_NOT,0, 0, FALSE}
};

int
main() {
	autounit_testcase_t *test_vbm;
	int test_no;
	gint result;
	autounit_test_t *tmp_test;

	test_vbm = 
		au_new_testcase(g_string_new("variable buffer manager testcase"),
						init_testcase,NULL);

	test_no = 0;
	while (tests[test_no].name != 0) {
		if (tests[test_no].isEnabled == TRUE) {
			tmp_test = au_new_test(g_string_new(tests[test_no].name),
								tests[test_no].test_fp);
			au_set_fork_mode(tmp_test,tests[test_no].forking);
			au_add_test(test_vbm,tmp_test);
		}
		else {
			printf("?! '%s' test disabled\n",tests[test_no].name);
		}
		test_no++;
	}

	result = au_run_testcase(test_vbm);
	return result;
}
