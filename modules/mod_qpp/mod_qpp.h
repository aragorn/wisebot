/* $Id$ */
#ifndef MOD_QPP_H
#define MOD_QPP_H 1

#define QPP_OP_OPERAND		(-1)
#define QPP_OP_STAR			(20)
#define QPP_OP_BEG_PHRASE	(21)
#define QPP_OP_END_PHRASE	(22)
#define QPP_OP_BEG_PAREN	(23)
#define QPP_OP_END_PAREN	(24)

#define QPP_OP_NATURAL_BEGIN (25)
#define QPP_OP_NATURAL_END   (26)

#define FIELD_CLEAR		(2)

extern char *mMorpAnalyzerId;

typedef struct {
	Stack postfixStack;
	Stack operatorStack;

	int32_t searchField;
	int8_t nextTurn;
	int8_t posWithinPhrase;
	int8_t numPhraseOperand;
	char truncated;
	uint32_t virtualfield;
	int virtualfield_morpid;
	int8_t natural_search;
} StateObj;

int pushOperand(void* word_db, StateObj *pStObj,QueryNode *pQuNode);

#endif
