/* $Id$ */
#include "softbot.h"
#include "simpletree.h"

struct _stree_t {
	/* saving data */
	uint16_t num_of_leaves;
	uint32_t datasize;

	/* non-saving sata */
	uint16_t allocated_leaves;
	uint32_t allocated_datasize;
	stree_leaf_t *leaves;
	void *datachunk;

	int flag;
#define READABLE				1
#define WRITABLE				3
};

typedef struct _stree_saving_data_t {
	uint16_t num_of_leaves;
	uint32_t datasize;
} stree_saving_data_t;

#define DEFAULT_NUM_OF_LEAVES						32
#define DEFAULT_DATACHUNK_SIZE						4096

static int extend_leaves(stree_t *tree);
static int extend_datachunk(stree_t *tree, int size);

stree_t *stree_create()
{
	int i;
	stree_t *tree;

	tree = (stree_t *)sb_malloc(sizeof(stree_t));
	if (tree == NULL) {
		return NULL;
	}

	tree->num_of_leaves = 0;
	tree->datasize = 0;

	tree->leaves = (stree_leaf_t *)sb_calloc(DEFAULT_NUM_OF_LEAVES,
			sizeof(stree_leaf_t));
	if (tree->leaves == NULL) {
		sb_free(tree);
		return NULL;
	}

	tree->allocated_leaves = DEFAULT_NUM_OF_LEAVES;

	for (i=0; i<tree->allocated_leaves; i++) {
		tree->leaves[i].leafname[0] = '\0';
		tree->leaves[i].child = -1;
		tree->leaves[i].next = -1;
		tree->leaves[i].offset = 0;
		tree->leaves[i].size = 0;
	}

	tree->datachunk = sb_malloc(DEFAULT_DATACHUNK_SIZE);
	if (tree->datachunk == NULL) {
		sb_free(tree->leaves);
		sb_free(tree);
		return NULL;
	}

	tree->allocated_datasize = DEFAULT_DATACHUNK_SIZE;

	tree->flag = READABLE | WRITABLE;

	return tree;
}

void stree_destory(stree_t *tree)
{
	if (tree->flag | WRITABLE) {
		sb_free(tree->datachunk);
		sb_free(tree->leaves);
	}
	sb_free(tree);
}

int stree_get_savingsize(stree_t *tree)
{
	return sizeof(stree_saving_data_t) +
		   tree->num_of_leaves * sizeof(stree_leaf_t) +
		   tree->datasize;
}

int stree_save(stree_t *tree, void *buf, int buflen)
{ 
	int offset;

	if (buflen < sizeof(stree_saving_data_t) +
			tree->datasize + tree->num_of_leaves * sizeof(stree_leaf_t)) {
		return -1;
	}

	memcpy(buf, tree, sizeof(stree_saving_data_t));
	offset = sizeof(stree_saving_data_t);

	memcpy(buf + offset, tree->leaves, 
			tree->num_of_leaves * sizeof(stree_leaf_t));
	offset += tree->num_of_leaves * sizeof(stree_leaf_t);

	memcpy(buf + offset, tree->datachunk,
			tree->datasize);
	offset += tree->datasize;

	return offset;
}

stree_t *stree_load2(void *data, int datasize)
{
	int offset;
	stree_t *tree;

	tree = (stree_t *)sb_malloc(sizeof(stree_t));
	if (tree == NULL) {
		return NULL;
	}

	memcpy(tree, data, sizeof(stree_saving_data_t));
	offset = sizeof(stree_saving_data_t);

	if (datasize != sizeof(stree_saving_data_t) + 
			tree->datasize + tree->num_of_leaves * sizeof(stree_leaf_t)) {
		return NULL;
	}

	tree->allocated_leaves = tree->num_of_leaves;
	tree->allocated_datasize = tree->datasize;

	tree->leaves = data + offset;
	offset += tree->num_of_leaves * sizeof(stree_leaf_t);

	tree->datachunk = data + offset;

	tree->flag = READABLE;

	return tree;
}

stree_leaf_t *stree_add_root(stree_t *tree, char key[])
{
	if (tree->leaves[0].leafname[0] != '\0') {
		// error("there is already root");
		return NULL;
	}

	strncpy(tree->leaves[0].leafname, key, MAX_LEAFNAME_LEN);
	tree->leaves[0].leafname[MAX_LEAFNAME_LEN-1] = '\0';

	tree->num_of_leaves++;
	return &(tree->leaves[0]);
}

stree_leaf_t *stree_get_root(stree_t *tree)
{
	if (tree->leaves[0].leafname[0] == '\0') {
		// error("there is no root");
		return NULL;
	}

	return &(tree->leaves[0]);
}

int stree_add_child_leaf(stree_t *tree, stree_leaf_t *parent, 
						char key[], void *data, int len)
{
	stree_leaf_t *leaf, *prev;

	/* check leaves chunk space */
	if (tree->num_of_leaves == tree->allocated_leaves) {
		if (extend_leaves(tree) == -1) {
			return -1;
		}
	}

	/* check data chunk space */
	if (tree->datasize + len > tree->allocated_datasize) {
		if (extend_datachunk(tree, len) == -1) {
			return -1;
		}
	}

	/* set data properly */
	strncpy(tree->leaves[tree->num_of_leaves].leafname, key, MAX_LEAFNAME_LEN);
	tree->leaves[tree->num_of_leaves].leafname[MAX_LEAFNAME_LEN-1] = '\0';
	memcpy(tree->datachunk + tree->datasize, data, len);
	tree->leaves[tree->num_of_leaves].offset = tree->datasize;
	tree->leaves[tree->num_of_leaves].size = len;
	tree->datasize += len;

	/* set link with parent leaf */
	leaf = stree_get_child_leaf(tree, parent);
	if (leaf == NULL) {
		parent->child = tree->num_of_leaves;
	}
	else {
		prev = leaf;
		while ((leaf = stree_get_next_leaf(tree, leaf)) != NULL) {
			prev = leaf;
		}
		prev->next = tree->num_of_leaves;
	}

	tree->num_of_leaves++;
	return 1;
}

int stree_add_next_leaf(stree_t *tree, stree_leaf_t *sibling, 
						char key[], void *data, int len)
{
	/* check leaves chunk space */
	if (tree->num_of_leaves == tree->allocated_leaves) {
		if (extend_leaves(tree) == -1) {
			return -1;
		}
	}

	/* check data chunk space */
	if (tree->datasize + len > tree->allocated_datasize) {
		if (extend_datachunk(tree, len) == -1) {
			return -1;
		}
	}

	/* set data properly */
	strncpy(tree->leaves[tree->num_of_leaves].leafname, key, MAX_LEAFNAME_LEN);
	tree->leaves[tree->num_of_leaves].leafname[MAX_LEAFNAME_LEN-1] = '\0';
	memcpy(tree->datachunk + tree->datasize, data, len);
	tree->leaves[tree->num_of_leaves].offset = tree->datasize;
	tree->leaves[tree->num_of_leaves].size = len;
	tree->datasize += len;

	/* set link with sibling leaf */
	tree->leaves[tree->num_of_leaves].next = sibling->next;
	sibling->next = tree->num_of_leaves;

	tree->num_of_leaves++;
	return 1;
}

#ifndef STREE_TINY_OBJECT
stree_leaf_t *stree_get_parent_leaf(stree_t *tree, stree_leaf_t *leaf)
{
	if (leaf->parent == -1) return NULL;
	return &(tree->leaves[leaf->parent]);
}
#endif

stree_leaf_t *stree_get_child_leaf(stree_t *tree, stree_leaf_t *leaf)
{
	if (leaf->child == -1) return NULL;
	return &(tree->leaves[leaf->child]);
}

#ifndef STREE_TINY_OBJECT
stree_leaf_t *stree_get_prev_leaf(stree_t *tree, stree_leaf_t *leaf)
{
	if (leaf->prev == -1) return NULL;
	return &(tree->leaves[leaf->prev]);
}
#endif

stree_leaf_t *stree_get_next_leaf(stree_t *tree, stree_leaf_t *leaf)
{
	if (leaf->next == -1) return NULL;
	return &(tree->leaves[leaf->next]);
}

void *stree_get_data(stree_t *tree, stree_leaf_t *leaf, uint32_t *size)
{
	*size = leaf->size;
	return tree->datachunk + leaf->offset;
}

static int extend_leaves(stree_t *tree)
{
	stree_leaf_t *tmp;
	int doubled_num, i;

	doubled_num = tree->allocated_leaves * 2;
	tmp = (stree_leaf_t *)sb_realloc(tree->leaves, 
								doubled_num * sizeof(stree_leaf_t));
	if (tmp == NULL) {
		return -1;
	}
	tree->leaves = tmp;

	/* initialize newly allocated leaves */
	for (i=tree->allocated_leaves; i<doubled_num; i++) {
		tree->leaves[i].leafname[0] = '\0';
		tree->leaves[i].child = -1;
		tree->leaves[i].next = -1;
		tree->leaves[i].offset = 0;
		tree->leaves[i].size = 0;
	}

	tree->allocated_leaves = doubled_num;

	return 1;
}

static int extend_datachunk(stree_t *tree, int size)
{
	void *tmp;
	int newsize;

	newsize = tree->allocated_datasize;
	while ((newsize *= 2) < tree->allocated_datasize + size);
	tmp = sb_realloc(tree->datachunk, newsize);
	if (tmp == NULL) {
		return -1;
	}
	tree->datachunk = tmp;
	tree->allocated_datasize = newsize;

	return 1;
}
