#ifndef _DAUM_QPP_H_
#define _DAUM_QPP_H_

// daum_koma는 괄호를 항상 쳐주기 때문에 priority가 필요없다.
// 그래도 일단은 enum값을 priority로 간주하고 주자.
enum daum_op {
	DAUM_OP_NONE = 0,

	DAUM_OP_OR = 10,
	DAUM_OP_AND = 20,

	// parenthesis는 priority가 좀 특이하다...
	DAUM_OP_LPAREN = 99,
	DAUM_OP_RPAREN = 1
};

struct daum_tree_node {
	union {
		enum daum_op op;
		char* word; // 다른 문자열을 pointing하고 있다. 주의!!
	} node;
	int is_op; // op면 1, word면 0

	struct daum_tree_node* left;
	struct daum_tree_node* right;
};

struct daum_tree_node* parse_daum_query(index_word_t* indexwords, int count);
void destroy_daum_tree(struct daum_tree_node* root);
const char* print_daum_tree(struct daum_tree_node* tree, char* buf, int size);
int push_daum_tree(void* word_db, StateObj* pStObj, struct daum_tree_node* tree);

#endif // _DAUM_QPP_H_

