/* $Id$ */
#ifndef MOD_QPP1_H
#define MOD_QPP1_H 1

typedef struct qpp1_node_t qpp1_node_t;

typedef enum {
	OPERATOR_AND = 1,
	OPERATOR_OR,
	OPERATOR_NOT,
	OPERATOR_WITHIN,
	OPERATOR_FIELD,
	LAST_OPERATOR,
	OPERAND_STD,
	OPERAND_PHRASE,
	OPERAND_RIGHT_TRUNCATED,
	OPERAND_LEFT_TRUNCATED
} node_type_t;

struct qpp1_node_t {
	node_type_t type;

	char *string;

	int param;
	qpp1_node_t *left;
	qpp1_node_t *right;
};

void init_nodes();
qpp1_node_t* new_qpp1_node();
qpp1_node_t* new_operand(char *string);
qpp1_node_t* new_operator(int type, char* param);
qpp1_node_t* field_operator(qpp1_node_t* node, char* field);
void set_tree(qpp1_node_t* node);
void print_tree();
void print_node(qpp1_node_t* node, int depth);

#define IS_QPP1_OPERATOR(type) (type < LAST_OPERATOR ? 1 : 0 )
#define IS_QPP1_OPERAND(type)  (type > LAST_OPERATOR ? 1 : 0 )

extern char QPP1_NODE_TYPES[][30];

#endif
