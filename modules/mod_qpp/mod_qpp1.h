/* $Id$ */
#ifndef MOD_QPP1_H
#define MOD_QPP1_H 1

typedef struct qpp1_node_t qpp1_node_t;

typedef enum {
	OPERATOR_AND = 1,
	OPERATOR_OR,
	LAST_OPERATOR,
	OPERAND_STD,
	OPERAND_PHRASE,
	OPERAND_RIGHT_TRUNCATED,
	OPERAND_LEFT_TRUNCATED
} node_type_t;

struct qpp1_node_t {
	node_type_t type;

	char *string;

	int num_of_operands;
	int param;

	qpp1_node_t *next;
};

void init_nodes();
qpp1_node_t* new_qpp1_node();
int push_operand(char *string);
void print_stack();

#define IS_QPP1_OPERATOR(type) (type < LAST_OPERATOR ? 1 : 0 )
#define IS_QPP1_OPERAND(type)  (type > LAST_OPERATOR ? 1 : 0 )

extern char QPP1_NODE_TYPES[][30];

#endif
