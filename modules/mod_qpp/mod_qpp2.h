/* $Id$ */
#ifndef _MOD_QPP2_H_
#define _MOD_QPP2_H_ 1

#include "mod_api/lexicon.h"

typedef struct {
	int32_t field;  /* �ִ� 32�� �ʵ� ���� */
	int     weight; /* ���ü� ����ġ       */
	word_t  word;   /* ���Ǿ� - lexicon.h  */
} qpp2_operand_t;

/* Operator ID */
enum op_id {
	OP_FIELD,
	OP_AND,
	OP_OR,
	OP_NOT,
	OP_WITHIN,
	OP_PHRASE,
	OP_LPHRASE,
	OP_RPHRASE,
	OP_LPARAN,
	OP_RPARAN,
	OP_LTRUNC,
	OP_RTRUNC,
};

typedef struct {
	enum op_id id;
	int param1;
} qpp2_operator_t;

#endif
