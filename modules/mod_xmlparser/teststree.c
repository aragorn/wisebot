/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "simpletree.h"


#define BUF_SIZE			(1024 * 1024)
//#define REPEAT_TEST			1000000

char *key[] = {"key0", "key1", "key2", "key3", "key4", "key5", "key6", "key7", "key8", "key9"};
char *value[] = {"value0", "value1", "value2", "value3", "value4", "value5", "value6", "value7", "value8", "value9"};

int main()
{
	int i, size;
	char buf[BUF_SIZE];
	stree_t *tree;
	stree_leaf_t *root, *leaf;

#ifdef REPEAT_TEST
	{
		int j;
		struct timeval ts, tf;
		gettimeofday(&ts, NULL);
		for (j=0; j<REPEAT_TEST; j++) {
#endif

	// test create
	tree = stree_create();
	if (tree == NULL) {
		fprintf(stderr, "cannot create dh object\n");
		return 1;
	}

#ifndef REPEAT_TEST
	printf("create object successfully\n");
#endif

	// test add root
	root = stree_add_root(tree, "Document");
	if (root == NULL) {
		fprintf(stderr, "cannot add root\n");
		return 1;
	}

	// test add leaf
#ifndef REPEAT_TEST
	printf("\ntest add_leaf...\n");
#endif
	for (i=0; i<10; i++) {
		if (stree_add_child_leaf(tree, root, key[i], value[i], 16) == -1) {
			fprintf(stderr, "cannot insert key[%s], value[%s]\n", key[i], value[i]);
			return 1;
		}
#ifndef REPEAT_TEST
		printf("insert successfully: key[%s], value[%s]\n", key[i], value[i]);
#endif
	}

	// test search
#ifndef REPEAT_TEST
	printf("\ntest search...\n");
#endif
	leaf = stree_get_child_leaf(tree, root);
	while (leaf != NULL) {
		char *data;
		int size;
		printf("leaf name: %s\n", leaf->leafname);
		data = (char *)stree_get_data(tree, leaf, &size);
		if (data == NULL) {
			fprintf(stderr, "cannot get data of leaf[%s]\n", leaf->leafname);
			return 1;
		}
		printf("leaf value: %s\n", data);
		leaf = stree_get_next_leaf(tree, leaf);
	}

	// test save
#ifndef REPEAT_TEST
	if ((size = stree_save(tree, buf, BUF_SIZE)) == -1) {
		fprintf(stderr, "cannot save\n");
		return 1;
	}
	printf("\nsave1 successfully\n");
#endif

	// test destroy
	stree_destory(tree);
#ifndef REPEAT_TEST
	printf("\ndestroy object successfully\n");
#endif

#ifdef REPEAT_TEST
		}
		gettimeofday(&tf, NULL);
		printf("%d repeat takes %ld sec.\n", j, tf.tv_sec - ts.tv_sec);
	}
#endif

	// test load
#if 0
#ifndef REPEAT_TEST
	stree = stree_load(buf, &size);
	if (dh == NULL) {
		fprintf(stderr, "cannot load\n");
		return 1;
	}
	printf("\nload1 successfully\n");

	printf("\ntest search...\n");

	stree_destroy(dh);
	printf("\ndestroy object successfully\n");
#endif
#endif

	return 0;
}
