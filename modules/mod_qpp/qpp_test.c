/* $Id$ */
#define QPP_CORE_PRIVATE

#include "qpp_test.h"

#include <glib.h>
#include <c-unit/test.h>
#include <string.h>

Int16 nRet=0;

gint init_once() {
/*	SP_setPath(TEST_CONFIG_PATH"/conf_for_qpp");*//*{{{*/
/*	LM_init(TEST_CONFIG_PATH"/conf_for_qpp");*/
/*	LM_load(SUFFIX_ON);*//*}}}*/
	// FIXME: fix to use configuration module
	_setDicFile(TEST_DB_PATH"/dic/ma.dic");// setting for morpheme analyser
	MA_init();
	return TRUE;
}

gint setUpQPP(autounit_test_t *t) {
	nRet = QPP_init();
	au_assert(t,"",nRet == SUCCESS);
	return TRUE;
}
gint tearDownQPP(autounit_test_t *t) {
	return TRUE;
}
gint testBlank(autounit_test_t *t){
	char aInfixQuery[1];
	QueryNode aPostfixQuery[1];

	aInfixQuery[0] = (char)NULL;

	nRet = QPP_parse(aInfixQuery,FALSE,0,aPostfixQuery);

	au_assert(t,"",nRet == 0); // 0 number parsed result
	return TRUE;
}

gint testOverflow(autounit_test_t *t){
	char aInfixQuery[400];
	QueryNode aPostfixQuery[1];

	strcpy(aInfixQuery,"Oranges ��");

	nRet = QPP_parse(aInfixQuery, FALSE,1, aPostfixQuery);

	au_assert(t,"",nRet == QPP_QUERY_NODE_OVERFLOW);
	return TRUE;
}

gint testOneOperand(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[1];

	strcpy(aInfixQuery,"Orange");

	nRet = QPP_parse(aInfixQuery, FALSE,1, aPostfixQuery);

	au_assert(t,"",nRet == 1);
	au_assert(t,"",strcmp(aPostfixQuery[0].word,"ORANGE") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);
	return TRUE;
}

gint testBinaryOperator(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[3];

	strcpy(aInfixQuery,"Orange&���");

	nRet = QPP_parse(aInfixQuery, FALSE,3, aPostfixQuery);

	au_assert(t,"",nRet == 3);

	au_assert(t,"",strcmp(aPostfixQuery[0].word,"ORANGE") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_AND);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);
	return TRUE;
}

gint testBinaryOperator2(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"Orange OR ��� AND ����");

	nRet = QPP_parse(aInfixQuery, FALSE,10, aPostfixQuery);

	au_assert(t,"",nRet == 5);

	au_assert(t,"",strcmp(aPostfixQuery[0].word,"ORANGE") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[2].word,"����") == 0);
	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[2].opParam < 0);

	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_AND);
	au_assert(t,"",aPostfixQuery[3].num_of_operands == 2);

	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_OR);
	au_assert(t,"",aPostfixQuery[4].num_of_operands == 2);
	return TRUE;
}

gint testBinaryOperator3(autounit_test_t *t){
	int nRet = 0;
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"Orange OR ��� NEAR ���� AND �丶��");

	nRet = QPP_parse(aInfixQuery, FALSE,10, aPostfixQuery);

	au_assert(t,"",nRet == 7);

	au_assert(t,"",strcmp(aPostfixQuery[0].word,"ORANGE") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[2].word,"����") == 0);
	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[2].opParam < 0);

	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_NEAR);
	au_assert(t,"",aPostfixQuery[3].num_of_operands == 2);

	au_assert(t,"",strcmp(aPostfixQuery[4].word,"�丶��") == 0);
	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[4].opParam < 0);

	au_assert(t,"",aPostfixQuery[5].operator == QPP_OP_AND);
	au_assert(t,"",aPostfixQuery[5].num_of_operands == 2);

	au_assert(t,"",aPostfixQuery[6].operator == QPP_OP_OR);
	au_assert(t,"",aPostfixQuery[6].num_of_operands == 2);
	return TRUE;
}

gint testParenthesis(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"(Orange OR ���) AND �丶��");

	nRet = QPP_parse(aInfixQuery, FALSE,10, aPostfixQuery);

	au_assert(t,"",nRet == 5);

	au_assert(t,"",strcmp(aPostfixQuery[0].word,"ORANGE") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_OR);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);

	au_assert(t,"",strcmp(aPostfixQuery[3].word,"�丶��") == 0);
	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[3].opParam < 0);

	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_AND);
	au_assert(t,"",aPostfixQuery[4].num_of_operands == 2);
	return TRUE;
}

gint testParenthesis2(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"(Orange OR ���) NEAR (�丶�� AND �䳢)");

	nRet = QPP_parse(aInfixQuery,FALSE, 10, aPostfixQuery);

	au_assert(t,"",nRet == 7);

	au_assert(t,"",strcmp(aPostfixQuery[0].word,"ORANGE") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_OR);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);

	au_assert(t,"",strcmp(aPostfixQuery[3].word,"�丶��") == 0);
	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[3].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[4].word,"�䳢") == 0);
	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[4].opParam < 0);

	au_assert(t,"",aPostfixQuery[5].operator == QPP_OP_AND);
	au_assert(t,"",aPostfixQuery[5].num_of_operands == 2);

	au_assert(t,"",aPostfixQuery[6].operator == QPP_OP_NEAR);
	au_assert(t,"",aPostfixQuery[6].num_of_operands == 2);
}

gint testFuzzyInsertion(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"�Ҹ� ���");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 3);
	
	au_assert(t,"",strcmp(aPostfixQuery[0].word,"�Ҹ�") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);
}

gint testFuzzyInsertion2(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"(�Ҹ� ���) (����)");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 5);
	
	au_assert(t,"",strcmp(aPostfixQuery[0].word,"�Ҹ�") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);

	au_assert(t,"",strcmp(aPostfixQuery[3].word,"����") == 0);
	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[3].opParam < 0);

	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[4].num_of_operands == 2);
	return TRUE;
}

gint testField(autounit_test_t *t) {
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"�Ҹ� ��� (body:(����))");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 5);

	au_assert(t,"",strcmp(aPostfixQuery[0].word,"�Ҹ�") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);

	au_assert(t,"",strcmp(aPostfixQuery[3].word,"����") == 0);
	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[3].opParam == 1);

	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[4].num_of_operands == 2);
	return TRUE;
}

gint testField2(autounit_test_t *t) {
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"body:�Ҹ� ��� (title:(����))");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 5);

	au_assert(t,"",strcmp(aPostfixQuery[0].word,"�Ҹ�") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam == 1);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);

	au_assert(t,"",strcmp(aPostfixQuery[3].word,"����") == 0);
	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[3].opParam == 0);

	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[4].num_of_operands == 2);
	return TRUE;
}

gint testNumeric(autounit_test_t *t) {
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"body:12-13 AND ��� title:(����)");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 5);

	au_assert(t,"",strcmp(aPostfixQuery[0].word,"12-13") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_NUMERIC);
	au_assert(t,"",aPostfixQuery[0].opParam == 1);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_AND);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);

	au_assert(t,"",strcmp(aPostfixQuery[3].word,"����") == 0);
	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[3].opParam == 0);

	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[4].num_of_operands == 2);
	return TRUE;
}

gint testNumeric2(autounit_test_t *t) {
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"body:113 AND ��� title:(����)");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 5);

	au_assert(t,"",strcmp(aPostfixQuery[0].word,"113") == 0);
	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_NUMERIC);
	au_assert(t,"",aPostfixQuery[0].opParam == 1);

	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_AND);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);

	au_assert(t,"",strcmp(aPostfixQuery[3].word,"����") == 0);
	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[3].opParam == 0);

	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[4].num_of_operands == 2);
	return TRUE;
}

gint testTruncate(autounit_test_t *t){
	int nRet = 0;
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"*���");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 3);

	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);
	au_assert(t,"",aPostfixQuery[0].wordid > 0);

	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);
	au_assert(t,"",aPostfixQuery[1].wordid > 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_OR);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);
	return TRUE;
}

gint testTruncate2(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"title:�ҹ�*");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 4);

	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam == 0);
	au_assert(t,"",aPostfixQuery[0].wordid > 0);

	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam == 0);
	au_assert(t,"",aPostfixQuery[1].wordid > 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[2].opParam == 0);
	au_assert(t,"",aPostfixQuery[2].wordid > 0);

	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OR);
	au_assert(t,"",aPostfixQuery[3].num_of_operands == 3);
	return TRUE;
}

gint testTruncate3(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"title:(�ҹ�*) !�ҹ漭");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

///	QPP_postfixPrint(stdout,aPostfixQuery,nRet);

	au_assert(t,"",nRet == 7);

	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[0].opParam == 0);
	au_assert(t,"",aPostfixQuery[0].wordid > 0);

	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[1].opParam == 0);
	au_assert(t,"",aPostfixQuery[1].wordid > 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[2].opParam == 0);
	au_assert(t,"",aPostfixQuery[2].wordid > 0);

	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OR);
	au_assert(t,"",aPostfixQuery[3].num_of_operands == 3);

	au_assert(t,"",strcmp(aPostfixQuery[4].word,"�ҹ漭") == 0);
	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[4].opParam < 0);

	au_assert(t,"",aPostfixQuery[5].operator == QPP_OP_NOT);
	au_assert(t,"",aPostfixQuery[5].num_of_operands == 1);

	au_assert(t,"",aPostfixQuery[6].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[6].num_of_operands == 2);
	return TRUE;
}

gint testTruncate4(autounit_test_t *t) {
	char aInfixQuery[100];
	QueryNode aPostfixQuery[10];

	strcpy(aInfixQuery,"!�ҹ漭 title:(�ҹ�*) ");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 7);

	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[0].word,"�ҹ漭") == 0);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_NOT);
	au_assert(t,"",aPostfixQuery[1].num_of_operands == 1);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[2].opParam == 0);
	au_assert(t,"",aPostfixQuery[2].wordid > 0);

	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[3].opParam == 0);
	au_assert(t,"",aPostfixQuery[3].wordid > 0);

	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_OPERAND);
	au_assert(t,"",aPostfixQuery[4].opParam == 0);
	au_assert(t,"",aPostfixQuery[4].wordid > 0);

	au_assert(t,"",aPostfixQuery[5].operator == QPP_OP_OR);
	au_assert(t,"",aPostfixQuery[5].num_of_operands == 3);

	au_assert(t,"",aPostfixQuery[6].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[6].num_of_operands == 2);
	return TRUE;
}

gint testPhrase(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"\"���\"");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 2);

	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[0].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_PHRASE);
	au_assert(t,"",aPostfixQuery[1].num_of_operands == 1);
	return TRUE;
}

gint testPhrase2(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[9];

	strcpy(aInfixQuery,"\"��� �ٺ�\"");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 3);

	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[0].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[1].word,"�ٺ�") == 0);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_PHRASE);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);
	return TRUE;
}

gint testPhrase3(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[10];

	strcpy(aInfixQuery,"body:�ٺ� \" ��� �ҹ�� ������\"");

	nRet = QPP_parse(aInfixQuery,FALSE,10,aPostfixQuery);

	au_assert(t,"",nRet == 6);

	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[0].word,"�ٺ�") == 0);
	au_assert(t,"",aPostfixQuery[0].opParam == 1);

	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[1].word,"���") == 0);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[2].word,"�ҹ��") == 0);
	au_assert(t,"",aPostfixQuery[2].opParam < 0);

	au_assert(t,"",aPostfixQuery[3].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[3].word,"������") == 0);
	au_assert(t,"",aPostfixQuery[3].opParam < 0);

	au_assert(t,"",aPostfixQuery[4].operator == QPP_OP_PHRASE);
	au_assert(t,"",aPostfixQuery[4].num_of_operands == 3);

	au_assert(t,"",aPostfixQuery[5].operator == QPP_OP_FUZZY);
	au_assert(t,"",aPostfixQuery[5].num_of_operands == 2);

	return TRUE;
}

gint testMorpheme(autounit_test_t *t){
	char aInfixQuery[100];
	QueryNode aPostfixQuery[15];

	strcpy(aInfixQuery,"�۹�����");

	nRet = QPP_parse(aInfixQuery,FALSE,50,aPostfixQuery);

	au_assert(t,"",nRet == 3);

	au_assert(t,"",aPostfixQuery[0].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[0].word,"�۹�") == 0);
	au_assert(t,"",aPostfixQuery[0].opParam < 0);

	au_assert(t,"",aPostfixQuery[1].operator == QPP_OP_OPERAND);
	au_assert(t,"",strcmp(aPostfixQuery[1].word,"����") == 0);
	au_assert(t,"",aPostfixQuery[1].opParam < 0);

	au_assert(t,"",aPostfixQuery[2].operator == QPP_OP_MORP);
	au_assert(t,"",aPostfixQuery[2].num_of_operands == 2);
	return TRUE;
}

typedef struct {
	gboolean forking;
	char *name;
	autounit_test_fp_t test_fp;
	gboolean isEnabled;
} test_link_t;

static test_link_t tests[] = {
	{FORK,"",testBlank,TRUE},
	{FORK,"",testOverflow,TRUE},
	{FORK,"",testOneOperand,TRUE},
	{FORK,"",testBinaryOperator,TRUE},
	{FORK,"",testBinaryOperator2,TRUE},
	{FORK,"",testBinaryOperator3,TRUE},
	{FORK,"",testParenthesis,TRUE},
	{FORK,"",testParenthesis2,TRUE},
	{FORK,"",testFuzzyInsertion,TRUE},
	{FORK,"",testFuzzyInsertion2,TRUE},
	{FORK,"",testField,TRUE},
	{FORK,"",testField2,TRUE},
	{FORK,"",testNumeric,TRUE},
	{FORK,"",testNumeric2,TRUE},
/*	{FORK,"",testTruncate,TRUE},*/
/*	{FORK,"",testTruncate2,TRUE},*/
/*	{FORK,"",testTruncate3,TRUE},*/
/*	{FORK,"",testTruncate4,TRUE},*/
	{FORK,"",testPhrase,TRUE},
	{FORK,"",testPhrase2,TRUE},
	{FORK,"",testPhrase3,TRUE},
	{FORK,"",testMorpheme,TRUE},
	{FORK,0, 0, TRUE}
};

int
main() {
	autounit_testcase_t *test_qpp;
	int test_no;
	gint result;
	autounit_test_t *tmp_test;

	test_qpp = 
		au_new_testcase(g_string_new("query preprocessor manager testcase"),
						setUpQPP,tearDownQPP);

	test_no = 0;
	while (tests[test_no].name != 0) {
		if (tests[test_no].isEnabled == TRUE) {
			tmp_test = au_new_test(g_string_new(tests[test_no].name),
								tests[test_no].test_fp);
			au_set_fork_mode(tmp_test,tests[test_no].forking);
			au_add_test(test_qpp,tmp_test);
		}
		else {
			printf("?! '%s' test disabled\n",tests[test_no].name);
		}
		test_no++;
	}

	init_once();
	result = au_run_testcase(test_qpp);
	return result;
}
