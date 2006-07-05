#ifndef _DAUM_QPP_H_
#define _DAUM_QPP_H_

// daum_koma�� ��ȣ�� �׻� ���ֱ� ������ priority�� �ʿ����.
// �׷��� �ϴ��� enum���� priority�� �����ϰ� ����.
enum daum_op {
	DAUM_OP_NONE = 0,

	DAUM_OP_OR = 10,
	DAUM_OP_AND = 20,

	// parenthesis�� priority�� �� Ư���ϴ�...
	DAUM_OP_LPAREN = 99,
	DAUM_OP_RPAREN = 1
};

struct daum_tree_node {
	union {
		enum daum_op op;
		char* word; // �ٸ� ���ڿ��� pointing�ϰ� �ִ�. ����!!
	} node;
	int is_op; // op�� 1, word�� 0

	struct daum_tree_node* left;
	struct daum_tree_node* right;
};

struct daum_tree_node* parse_daum_query(index_word_t* indexwords, int count);
void destroy_daum_tree(struct daum_tree_node* root);
const char* print_daum_tree(struct daum_tree_node* tree, char* buf, int size);
int push_daum_tree(void* word_db, StateObj* pStObj, struct daum_tree_node* tree);

#endif // _DAUM_QPP_H_

