#  include "common_core.h"

#ifndef TEST_PROGRAM
#  include "memory.h"
#else
#  include <stdlib.h>  // malloc, free

#  undef error
#  define error printf

#  define sb_malloc malloc
#  define sb_free   free
#endif

#include "mod_api/index_word_extractor.h"
#include "stack.h"
#include "mod_qpp.h"
#include "daum_qpp.h"

#include <string.h>

static struct daum_tree_node* make_daum_tree_node()
{
	struct daum_tree_node* ret = (struct daum_tree_node*) sb_malloc( sizeof(struct daum_tree_node) );

	ret->left = NULL;
	ret->right = NULL;

	return ret;
}

static const char* operator_str(enum daum_op op)
{
	switch(op) {
		case DAUM_OP_AND: return "&";
		case DAUM_OP_OR: return "|";
		default: return "?";
	}
}

static int pop_stack(struct daum_tree_node** operand_stack, int* operand_stack_pos,
		struct daum_tree_node** operator_stack, int* operator_stack_pos)
{
	struct daum_tree_node* operator;
    struct daum_tree_node* operand1;
    struct daum_tree_node* operand2;

	if ( *operand_stack_pos < 1 ) {
		error("invalid operand stack state [need more operand]");
		return FAIL;
	}

	if ( *operator_stack_pos < 0 ) {
		error("invalid operator stack state [need more operator]");
		return FAIL;
	}

	operator = operator_stack[(*operator_stack_pos)--];
	operand1 = operand_stack[(*operand_stack_pos)--];
	operand2 = operand_stack[*operand_stack_pos];

	operator->left = operand2;
	operator->right = operand1;
	operand_stack[*operand_stack_pos] = operator;

	if ( operator->node.op == DAUM_OP_LPAREN ) {
		error("not matched right parenthesis");
		return FAIL;
	}

	return SUCCESS;
}

#define OPERAND_STACK_SIZE 64
#define OPERATOR_STACK_SIZE 32
struct daum_tree_node* parse_daum_query(index_word_t* indexwords, int count)
{
	struct daum_tree_node* operand_stack[OPERAND_STACK_SIZE];
	struct daum_tree_node* operator_stack[OPERATOR_STACK_SIZE];

	int operand_stack_pos = -1, operator_stack_pos = -1;
	int i;

	for ( i = 0; i < count; i++ ) {
		enum daum_op op;

		if(strlen(indexwords[i].word) == 0)
			continue;

		if ( indexwords[i].word[1] == '\0' ) {
			char c = indexwords[i].word[0];

			if ( c == '&' ) op = DAUM_OP_AND;
			else if ( c == '|' ) op = DAUM_OP_OR;
			else if ( c == '(' || c == '{' ) op = DAUM_OP_LPAREN;
			else if ( c == ')' || c == '}' ) op = DAUM_OP_RPAREN;
			else op = DAUM_OP_NONE;
		}
		else op = DAUM_OP_NONE;

		// oprerand
		if ( op == DAUM_OP_NONE ) {
			struct daum_tree_node* node = make_daum_tree_node();
			node->node.word = indexwords[i].word;
			node->is_op = 0;

			if ( operand_stack_pos >= OPERAND_STACK_SIZE-1 ) {
				error("not enough operand stack size");
				goto return_error;
			}
			operand_stack[++operand_stack_pos] = node;
		}
		// pop till lparen
		else if ( op == DAUM_OP_RPAREN ) {
			struct daum_tree_node* lparen;

			while ( operator_stack_pos >= 0
					&& operator_stack[operator_stack_pos]->node.op != DAUM_OP_LPAREN ) {

				if ( pop_stack(operand_stack, &operand_stack_pos,
							operator_stack, &operator_stack_pos) != SUCCESS ) goto return_error;
			}

			if ( operator_stack_pos < 0 ) {
				error("not matched left parenthesis");
				goto return_error;
			}

			// pop lparen
			lparen = operator_stack[operator_stack_pos--];
			destroy_daum_tree(lparen);
		}
		// operator
		else {
			struct daum_tree_node* node;

			while ( operator_stack_pos >= 0
					&& operator_stack[operator_stack_pos]->node.op != DAUM_OP_LPAREN ) {

				// 원래는 연산자 priority인데 daum query에서는 필요없다.
				if ( operator_stack[operator_stack_pos]->node.op < op ) break;

				if ( pop_stack(operand_stack, &operand_stack_pos,
							operator_stack, &operator_stack_pos) != SUCCESS ) goto return_error;
			}

			// push
			node = make_daum_tree_node();
			node->node.op = op;
			node->is_op = 1;

			if ( operator_stack_pos >= OPERATOR_STACK_SIZE-1 ) {
				error("not enough operator stack size");
				goto return_error;
			}
			operator_stack[++operator_stack_pos] = node;
		}
	}

	// 남은거 정리
	while ( operator_stack_pos >= 0 ) {
		if ( pop_stack(operand_stack, &operand_stack_pos,
					operator_stack, &operator_stack_pos) != SUCCESS ) goto return_error;
	}

	if ( operand_stack_pos != 0 || operator_stack_pos != -1 ) {
		error("invalid operand stack state. operand_stack[%d], operator_stack[%d]",
				operand_stack_pos, operator_stack_pos);
		goto return_error;
	}

	return operand_stack[0];

return_error:
	for ( i = 0; i < operand_stack_pos; i++ )
		destroy_daum_tree( operand_stack[i] );

	for ( i = 0; i < operator_stack_pos; i++ )
		destroy_daum_tree( operator_stack[i] );

	return NULL;
}

void destroy_daum_tree(struct daum_tree_node* root)
{
	if ( root->left ) destroy_daum_tree( root->left );
	if ( root->right ) destroy_daum_tree( root->right );
	sb_free(root);
}

// buf에 결과를 저장하고, buf가 return된다.
static const char* _print_daum_tree(struct daum_tree_node* tree, char* buf, int size)
{
	if ( tree->left ) {
		strncat(buf, "(", size-strlen(buf)-1);
		_print_daum_tree(tree->left, buf, size);
	}

	if ( tree->is_op ) {
		strncat(buf, operator_str(tree->node.op), size-strlen(buf)-1);
	}
	else {
		strncat(buf, tree->node.word, size-strlen(buf)-1);
	}

	if ( tree->right ) {
		_print_daum_tree(tree->right, buf, size);
		strncat(buf, ")", size-strlen(buf)-1);
	}

	return buf;
}

const char* print_daum_tree(struct daum_tree_node* tree, char* buf, int size)
{
	buf[0] = '\0';
	return _print_daum_tree(tree, buf, size);
}

#ifndef TEST_PROGRAM
// pStObj에 tree를 집어넣는다.
int push_daum_tree(void* word_db, StateObj* pStObj, struct daum_tree_node* tree)
{
	QueryNode qnode;
	int ret;

	if ( tree->left &&
			push_daum_tree(word_db, pStObj, tree->left) != SUCCESS ) return FAIL;
	if ( tree->right &&
			push_daum_tree(word_db, pStObj, tree->right) != SUCCESS ) return FAIL;

	if ( tree->is_op ) { // operator
		if ( tree->node.op == DAUM_OP_AND ) qnode.operator = QPP_OP_AND;
		else if ( tree->node.op == DAUM_OP_OR ) qnode.operator = QPP_OP_OR;
		else {
			error("unknown daum operator: %s", operator_str(tree->node.op));
			return FAIL;
		}

		qnode.type = OPERATOR;
		qnode.num_of_operands = 2;
		ret = stk_push(&(pStObj->postfixStack), &qnode);
		if ( ret < 0 ) {
			error("error while pushing original word into stack");
			return ret;
		}
	}
	else { // operand
		strncpy(qnode.word_st.string, tree->node.word, MAX_WORD_LEN-1);
		qnode.word_st.string[MAX_WORD_LEN-1] = '\0';
		ret = pushOperand(word_db, pStObj, &qnode);
		if ( ret < 0 ) {
			error("failed to push operand for word[%s]", qnode.word_st.string);
			return ret;
		}
	}

	return SUCCESS;
}
#endif

#ifdef TEST_PROGRAM
int main(int argc, char** argv)
{
	index_word_t tokens[128];
	int i = 0;
	char* tok;
	struct daum_tree_node* tree;
	char buf[1024];

	if ( argc < 2 ) {
		fprintf(stderr, "need query\n");
		return 1;
	}

	tok = strtok(argv[1], " ");
	while ( tok != NULL ) {
		strcpy( tokens[i++].word, tok );
		tok = strtok(NULL, " ");
	}

	buf[0] = '\0';

	tree = parse_daum_query(tokens, i);
	if ( tree == NULL ) {
		fprintf(stderr, "parsing error\n");
		return 1;
	}
	printf("%s\n", print_daum_tree(tree, buf, sizeof(buf)));
	destroy_daum_tree(tree);

	return 0;
}
#endif
