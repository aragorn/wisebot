/* $Id$ */
#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include "mod_api/qpp.h" /* QueryNode */

#define TOKEN_DUPLICATED	(-11)
#define END_OF_STRING		(-21)
#define TOKEN_OVERFLOW		(-22)
#define TOKEN_OPERATOR		(-23)
#define TOKEN_STAR			(-24)
#define TOKEN_NUMERIC		(-25)

#define STAR_BEGIN			(1)
#define STAR_END			(2)

#define MAX_INPUTSTR_LEN	(512)
typedef struct tagTokenObj{
	char inputStr[MAX_INPUTSTR_LEN];// input string
	int16_t  idx;						// current index of string

	int8_t  currentOperator;
	int8_t  currentNumOperand;
	int16_t  currentOpLen;
	int32_t currentOpParam;
	int8_t  lastOperator;

	int16_t starStrBufferLen ;
	char starStrBuffer[MAX_INPUTSTR_LEN];
	int  isBeginPhraseTurn;
	int  isPositionWithinPhrase;
} TokenObj;

void tk_setString(TokenObj *pObj,char* theString);
int tk_getNextToken(TokenObj *pObj,QueryNode *pQueryNode,int maxLen);
int tk_init();
void tk_printAllOperators();

extern char *mQppMorpAnalyzerId;
extern char *mIndexerMorpAnalyzerId;

// configuration setting functions
void set_op_and(configValue v);
void set_op_or(configValue v);
void set_op_not(configValue v);
void set_op_paragraph(configValue v);
void set_op_within(configValue v);
void set_op_fuzzy(configValue v);
void set_op_field(configValue v);
void set_fieldname(configValue v);
void set_virtual_fieldname(configValue v);
void set_fieldnum(configValue v);
void set_doc_attr_field_name(configValue v);
void set_op_star(configValue v);
void set_op_phrase(configValue v);
void set_begin_op_phrase(configValue v);
void set_end_op_phrase(configValue v);

const char* qpp_op_to_string(int op, int param);

extern int mNumOfField;
extern char mFieldName[MAX_EXT_FIELD][SHORT_STRING_SIZE];

extern int mVirtualFieldNum;
extern char mVirtualFieldName[MAX_EXT_FIELD][SHORT_STRING_SIZE];
extern uint32_t mVirtualFieldIds[MAX_EXT_FIELD];
#endif
