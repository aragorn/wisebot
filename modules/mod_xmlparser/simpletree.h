/* $Id$ */
#ifndef SIMPLE_TREE_H
#define SIMPLE_TREE_H 1

#include "auto_config.h"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

typedef struct _stree_t stree_t;
typedef struct _stree_leaf_t stree_leaf_t;
#define MAX_LEAFNAME_LEN		32
struct _stree_leaf_t {
	/* public */
	char leafname[MAX_LEAFNAME_LEN];

	/* private */
#ifndef STREE_TINY_OBJECT
	int16_t parent;
#endif
	int16_t child;					/* array index of child leaf */
#ifndef STREE_TINY_OBJECT
	int16_t prev;					/* array index of previous leaf */
#endif
	int16_t next;					/* array index of next leaf */
	uint32_t offset;				/* byte offset of data */
	uint32_t size;					/* byte size of data */
};


stree_t *stree_create();
void stree_destory(stree_t *tree);
int stree_save(stree_t *tree, void *buf, int buflen);
stree_t *stree_load2(void *data, int datasize);
int stree_get_savingsize(stree_t *tree);

stree_leaf_t *stree_add_root(stree_t *tree, char key[]);
stree_leaf_t *stree_get_root(stree_t *tree);
int stree_add_child_leaf(stree_t *tree, stree_leaf_t *parent, 
						char key[], void *data, int len);
int stree_add_next_leaf(stree_t *tree, stree_leaf_t *sibling, 
						char key[], void *data, int len);
stree_leaf_t *stree_get_child_leaf(stree_t *tree, stree_leaf_t *leaf);
stree_leaf_t *stree_get_next_leaf(stree_t *tree, stree_leaf_t *leaf);
void *stree_get_data(stree_t *tree, stree_leaf_t *leaf, uint32_t *size);

#endif
