/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dh.h"

char *key[] = {"key0", "key1", "key2", "key3", "key4", "key5", "key6", "key7", "key8", "key9"};
char *value[] = {"value0", "value1", "value2", "value3", "value4", "value5", "value6", "value7", "value8", "value9"};

int main()
{
	int i, size;
	dh_t *dh;
	char buf[4096];
	struct test {
		char value[10];
	} var, *p;

	// test create
	dh = create(sizeof(struct test));
	if (dh == NULL) {
		fprintf(stderr, "cannot create dh object\n");
		return 1;
	}
	printf("create object successfully\n");

	// test insert
	printf("\ntest insert...\n");
	for (i=0; i<10; i++) {
		bzero(&var, sizeof(var));
		strcpy(var.value, value[i]);
		if (insert(dh, key[i], &var) == -1) {
			fprintf(stderr, "cannot insert key[%s], value[%s]\n", key[i], value[i]);
			return 1;
		}
		printf("insert successfully: key[%s], value[%s]\n", key[i], value[i]);
	}

	// test search
	printf("\ntest search...\n");
	for (i=0; i<10; i++) {
		if (search(dh, key[i], (void **)&p) == -1) {
			fprintf(stderr, "cannot search key[%s]\n", key[i]);
			return 1;
		}
		printf("search successfully: key[%s], value[%s]\n", key[i], p->value);
	}

	// test save
	if ((size = save2(dh, buf, 4096)) == -1) {
		fprintf(stderr, "cannot save\n");
		return 1;
	}
	printf("\nsave1 successfully\n");

	// test destroy
	destroy(dh);
	printf("\ndestroy object successfully\n");

	// test load
	dh = load1(buf, size);
	if (dh == NULL) {
		fprintf(stderr, "cannot load\n");
		return 1;
	}
	printf("\nload1 successfully\n");

	printf("\ntest search...\n");
	for (i=0; i<10; i++) {
		if (search(dh, key[i], (void **)&p) == -1) {
			fprintf(stderr, "cannot search key[%s]\n", key[i]);
			return 1;
		}
		printf("search successfully: key[%s], value[%s]\n", key[i], p->value);
	}

	destroy(dh);

	return 0;
}
