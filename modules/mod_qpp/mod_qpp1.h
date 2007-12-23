/* $Id$ */
#ifndef MOD_QPP1_H
#define MOD_QPP1_H 1

typedef struct qpp1_operand_t qpp1_operand_t;

typedef enum {
	OPERAND_STD = 1,
	OPERAND_PHRASE,
	OPERAND_RIGHT_TRUNCATED,
	OPERAND_LEFT_TRUNCATED
} qpp1_operand_type_t;

struct qpp1_operand_t {
	qpp1_operand_type_t operand_type;
	
	char string[STRING_SIZE+1];
};

void init_operands();
qpp1_operand_t* new_qpp1_operand();

#endif
