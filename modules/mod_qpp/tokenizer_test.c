/* $Id$ */
#define QPP_CORE_PRIVATE	1

#include "tokenizer_test.h"

#include <glib.h>
#include <c-unit/test.h>
#include <string.h>

Int16 nRet=0;
TokenObj tkObj;

gint init_once(autounit_test_t *t) {
	return TRUE;
}

gint setUpTokenizer(autounit_test_t *t) {
	return TRUE;
}
gint tearDownTokenizer(autounit_test_t *t) {
	return TRUE;
}

gint testInit(autounit_test_t *t) {
	nRet = tk_init();

	au_assert(t,"",nRet == SUCCESS);
	return TRUE;
}
gint testOperands(autounit_test_t *t) {
	char inputString[256] = "������   	 �ٺ�";
	QueryNode aQueryNode[1];

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,aQueryNode,256);
	au_assert(t,"",strcmp(aQueryNode[0].word,"������") == 0);
	au_assert(t,"",nRet == 6);

	nRet = tk_getNextToken(&tkObj,aQueryNode,256);
	au_assert(t,"",strcmp(aQueryNode[0].word,"�ٺ�") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,aQueryNode,256);
	au_assert(t,"",nRet == END_OF_STRING);
	return TRUE;
}
gint testLongerOperands(autounit_test_t *t){
	char inputString[256] = "  	���ξ��� ��ǻ�� ������   ���  å �ѱ��� ";
	QueryNode aQueryNode[1];

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,aQueryNode,256);
	au_assert(t,"",strcmp(aQueryNode[0].word,"���ξ���") == 0);
	au_assert(t,"",nRet == 8);

	nRet = tk_getNextToken(&tkObj,aQueryNode,256);
	au_assert(t,"",strcmp(aQueryNode[0].word,"��ǻ��") == 0);
	au_assert(t,"",nRet == 6);

	nRet = tk_getNextToken(&tkObj,aQueryNode,256);
	au_assert(t,"",strcmp(aQueryNode[0].word,"������") == 0);
	au_assert(t,"",nRet == 6);

	nRet = tk_getNextToken(&tkObj,aQueryNode,256);
	au_assert(t,"",strcmp(aQueryNode[0].word,"���") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,aQueryNode,256);
	au_assert(t,"",strcmp(aQueryNode[0].word,"å") == 0);
	au_assert(t,"",nRet == 2);

	nRet = tk_getNextToken(&tkObj,aQueryNode,256);
	au_assert(t,"",strcmp(aQueryNode[0].word,"�ѱ���") == 0);
	au_assert(t,"",nRet == 6);

	nRet = tk_getNextToken(&tkObj,aQueryNode,1);

	au_assert(t,"",nRet == END_OF_STRING);

	return TRUE;
}

gint testTokenOverflow(autounit_test_t *t){
	char inputString[256] = " �׽�Ʈ This test";
	QueryNode aQueryNode[1];

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,aQueryNode,1);
	au_assert(t,"",nRet == TOKEN_OVERFLOW);
	return TRUE;
}
gint testEscapeChar(autounit_test_t *t) {
	char inputString[256] = "%095��� %96%97 %91";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,&queryNode,20);
	au_assert(t,"",strcmp(queryNode.word,"_���") == 0);
	au_assert(t,"",nRet == 5);

	nRet = tk_getNextToken(&tkObj,&queryNode,20);
	au_assert(t,"",strcmp(queryNode.word,"`a") == 0);
	au_assert(t,"",nRet == 2);

	nRet = tk_getNextToken(&tkObj,&queryNode,20);
	au_assert(t,"",strcmp(queryNode.word,"[") == 0);
	au_assert(t,"",nRet == 1);
	return TRUE;
}

gint testOperatorAND(autounit_test_t *t){
	char inputString[256] = "����&���� ANDREW	AND test";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"����") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_AND);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"����") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"ANDREW") == 0);
	au_assert(t,"",nRet == 6);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_AND);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"test") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",nRet == END_OF_STRING);

	return TRUE;
}
gint testOperatorOR(autounit_test_t *t){
	char inputString[256] = "����|���� ORANGE OR test";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);
	nRet = tk_getNextToken(&tkObj,&queryNode,256);

	au_assert(t,"",strcmp(queryNode.word,"����") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_OR);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"����") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"ORANGE") == 0);
	au_assert(t,"",nRet == 6);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_OR);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"test") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",nRet == END_OF_STRING);
	return TRUE;
}

gint testOperatorNOT(autounit_test_t *t){
	char inputString[256] = "���� AND !���� OR NOTHERN AND	 NOT test";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"����") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_AND);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_NOT);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"����") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_OR);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);


	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"NOTHERN") == 0);
	au_assert(t,"",nRet == 7);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_AND);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_NOT);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"test") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",nRet == END_OF_STRING);
	return TRUE;
}

gint testOperatorFUZZY(autounit_test_t *t){
	char inputString[256] = "���� FUZZY FUZZYTHEORY";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);
	nRet = tk_getNextToken(&tkObj,&queryNode,256);

	au_assert(t,"",strcmp(queryNode.word,"����") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_FUZZY);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"FUZZYTHEORY") == 0);
	au_assert(t,"",nRet == 11);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",nRet == END_OF_STRING);
	return TRUE;
}

gint testOperatorNEAR(autounit_test_t *t){
	char inputString[256] = "THIS NEAR THAT NEAR18 THESE NEAR:7THOSE";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"THIS") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_NEAR);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",queryNode.opParam == 1);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"THAT") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_NEAR);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",queryNode.opParam == 18);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"THESE") == 0);
	au_assert(t,"",nRet == 5);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_NEAR);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",queryNode.opParam == 7);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"THOSE") == 0);
	au_assert(t,"",nRet == 5);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",nRet == END_OF_STRING);

	return TRUE;
}

gint testOperatorWITHIN(autounit_test_t *t){
	char inputString[256] = "THIS WITHIN THAT WITHIN99 THESE WITHIN/23THOSE";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"THIS") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_WITHIN);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",queryNode.opParam == 1);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"THAT") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_WITHIN);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",queryNode.opParam == 99);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"THESE") == 0);
	au_assert(t,"",nRet == 5);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_WITHIN);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",queryNode.opParam == 23);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"THOSE") == 0);
	au_assert(t,"",nRet == 5);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",nRet == END_OF_STRING);
	return TRUE;
}

gint testOperatorFIELD(autounit_test_t *t){
	char inputString[256] = "title: ��� AND body:��� |body:�Ҹ� body:3-4 body:98";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_FIELD);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",queryNode.opParam == 0);	// title id is 0
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"���") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_AND);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_FIELD);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",queryNode.opParam == 1);	// body id is 1
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"���") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_OR);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_FIELD);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",queryNode.opParam == 1);	// body id is 1
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"�Ҹ�") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_FIELD);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",queryNode.opParam == 1);	// body id is 1
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
///	printf("queryNode.operator:%d\n",queryNode.operator);
///	printf("nRet:%d",nRet);
	au_assert(t,"",queryNode.operator == QPP_OP_NUMERIC);
	au_assert(t,"",strcmp(queryNode.word,"3-4") == 0);	// body id is 1
	au_assert(t,"",nRet == TOKEN_NUMERIC);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_FIELD);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",queryNode.opParam == 1);	// body id is 1
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_NUMERIC);
	au_assert(t,"",strcmp(queryNode.word,"98") == 0);	// body id is 1
	au_assert(t,"",nRet == TOKEN_NUMERIC);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",nRet == END_OF_STRING);
	return TRUE;
}

gint testOperatorSTAR(autounit_test_t *t){
	char inputString[256] = "���*�̸� &body: *���|��%38��";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_STAR);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",queryNode.opParam == STAR_END);	
	au_assert(t,"",strcmp(queryNode.word,"���") == 0);
	au_assert(t,"",nRet == TOKEN_STAR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"�̸�") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_AND);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_FIELD);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",queryNode.opParam == 1);	// body id is 1
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_STAR);
	au_assert(t,"",queryNode.num_of_operands == 1);
	au_assert(t,"",queryNode.opParam == STAR_BEGIN);	
	au_assert(t,"",strcmp(queryNode.word,"���") == 0);
	au_assert(t,"",nRet == TOKEN_STAR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_OR);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"��&��") == 0);
	au_assert(t,"",nRet == 5);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",nRet == END_OF_STRING);
	return TRUE;
}

gint testOperatorPARENTHESIS(autounit_test_t *t){
	char inputString[256] = " ( ����%38����)(ANDREW) AND test";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_BEG_PAREN);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"����&����") == 0);
	au_assert(t,"",nRet == 9);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_END_PAREN);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_BEG_PAREN);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"ANDREW") == 0);
	au_assert(t,"",nRet == 6);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_END_PAREN);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_AND);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"test") == 0);
	au_assert(t,"",nRet == 4);
	return TRUE;
}

gint testOperatorPHRASE(autounit_test_t *t){
							// "����&����"���"\ANDREW AND test"
	char inputString[256] = " \"����&����\" ���\"ANDREW AND test\"";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_BEG_PHRASE);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"����&����") == 0);
	au_assert(t,"",nRet == 9);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_END_PHRASE);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"���") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_BEG_PHRASE);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"ANDREW") == 0);
	au_assert(t,"",nRet == 6);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"AND") == 0);
	au_assert(t,"",nRet == 3);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",strcmp(queryNode.word,"test") == 0);
	au_assert(t,"",nRet == 4);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_END_PHRASE);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	return TRUE;
}

gint testBoundaryCondition(autounit_test_t *t){
	char inputString[256] = "�����а� AND ������к�  ";
	QueryNode queryNode;

	tk_setString(&tkObj,inputString);
	nRet = tk_getNextToken(&tkObj,&queryNode,256);

	au_assert(t,"",strcmp(queryNode.word,"�����а�") == 0);
	au_assert(t,"",nRet == 8);

	nRet = tk_getNextToken(&tkObj,&queryNode,256);
	au_assert(t,"",queryNode.operator == QPP_OP_AND);
	au_assert(t,"",queryNode.num_of_operands == 2);
	au_assert(t,"",nRet == TOKEN_OPERATOR);

	nRet = tk_getNextToken(&tkObj,&queryNode,9);
	au_assert(t,"",nRet == TOKEN_OVERFLOW);

	nRet = tk_getNextToken(&tkObj,&queryNode,10);
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
	{FORK,"",testOperands,TRUE},
	{FORK,"",testLongerOperands,TRUE},
	{FORK,"",testTokenOverflow,TRUE},
	{FORK,"",testEscapeChar,TRUE},
	{FORK,"",testOperatorAND,TRUE},
	{FORK,"",testOperatorOR,TRUE},
	{FORK,"",testOperatorNOT,TRUE},
	{FORK,"",testOperatorFUZZY,TRUE},
	{FORK,"",testOperatorNEAR,TRUE},
	{FORK,"",testOperatorWITHIN,TRUE},
	{FORK,"",testOperatorFIELD,TRUE},
	{FORK,"",testOperatorSTAR,TRUE},
	{FORK,"",testOperatorPARENTHESIS,TRUE},
	{FORK,"",testOperatorPHRASE,TRUE},
	{FORK,"",testBoundaryCondition,TRUE},
	{FORK,0, 0, FALSE}
};

int
main() {
	autounit_testcase_t *test_tokenizer;
	int test_no;
	gint result;
	autounit_test_t *tmp_test;

	test_tokenizer = 
		au_new_testcase(g_string_new("tokenizer_for_qpp testcase"),
						setUpTokenizer,tearDownTokenizer);

	test_no = 0;
	while (tests[test_no].name != 0) {
		if (tests[test_no].isEnabled == TRUE) {
			tmp_test = au_new_test(g_string_new(tests[test_no].name),
								tests[test_no].test_fp);
			au_set_fork_mode(tmp_test,tests[test_no].forking);
			au_add_test(test_tokenizer,tmp_test);
		}
		else {
			printf("?! '%s' test disabled\n",tests[test_no].name);
		}
		test_no++;
	}

	result = au_run_testcase(test_tokenizer);
	return 0;
}
