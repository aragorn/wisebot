#ifndef _TRIE_H
#define _TRIE_H 1

// XXX: temporary
#define MAX_PATHSTRING_SIZE			(512)

typedef struct _trie_node_t trie_node_t;

typedef struct _trie_t {
	// public
	char path[MAX_PATHSTRING_SIZE];
	trie_node_t *root_node;
} trie_t;

struct _trie_node_t {
	// public 
	// XXX: not general data... -_-
	char Code;
	char Str[30];
	int index;

	// XXX: temporary
	// reserved data: for inheritance implement in C
	int rsv0;
	int rsv1;

	// private
	struct _trie_node_t *ynext;
	struct _trie_node_t *xnext;
};

trie_t *create_trie_handler();
void destroy_trie_handler(trie_t *this);
int trie_insert_entry(trie_t *trie, const char *key, int rsv1, int rsv2);
void optimize(trie_t *trie, trie_node_t *node);
void print_tree(trie_node_t *node, int depth);

#endif
