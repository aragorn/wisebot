/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "dh.h"

#define BUF_SIZE			(1024 * 1024)
#define REPEAT_TEST			1000000

char *key[] = {"key0", "key1", "key2", "key3", "key4", "key5", "key6", "key7", "key8", "key9"};
char *value[] = {"value0", "value1", "value2", "value3", "value4", "value5", "value6", "value7", "value8", "value9"};

int main()
{
	int i, size;
	dh_t *dh;
	char buf[BUF_SIZE];
	struct test {
		char value[10];
	} var, *p;

#ifdef REPEAT_TEST
	{
		int j;
		struct timeval ts, tf;
		gettimeofday(&ts, NULL);
		for (j=0; j<REPEAT_TEST; j++) {
#endif

	// test create
	dh = dh_create(sizeof(struct test));
	if (dh == NULL) {
		fprintf(stderr, "cannot create dh object\n");
		return 1;
	}
#ifndef REPEAT_TEST
	printf("create object successfully\n");
#endif

	// test insert
#ifndef REPEAT_TEST
	printf("\ntest insert...\n");
#endif
	for (i=0; i<10; i++) {
		bzero(&var, sizeof(var));
		strcpy(var.value, value[i]);
		if (dh_insert(dh, key[i], &var) == -1) {
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
	for (i=0; i<10; i++) {
		if (dh_search(dh, key[i], (void **)&p) == -1) {
			fprintf(stderr, "cannot search key[%s]\n", key[i]);
			return 1;
		}
#ifndef REPEAT_TEST
		printf("search successfully: key[%s], value[%s]\n", key[i], p->value);
#endif
	}

	// test save
#ifndef REPEAT_TEST
	if ((size = dh_save(dh, buf, BUF_SIZE)) == -1) {
		fprintf(stderr, "cannot save\n");
		return 1;
	}
	printf("\nsave1 successfully\n");
#endif

	// test destroy
	dh_destroy(dh);
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
#ifndef REPEAT_TEST
	dh = dh_load(buf, &size);
	if (dh == NULL) {
		fprintf(stderr, "cannot load\n");
		return 1;
	}
	printf("\nload1 successfully\n");

	printf("\ntest search...\n");
	for (i=0; i<10; i++) {
		if (dh_search(dh, key[i], (void **)&p) == -1) {
			fprintf(stderr, "cannot search key[%s]\n", key[i]);
			return 1;
		}
		printf("search successfully: key[%s], value[%s]\n", key[i], p->value);
	}

	dh_destroy(dh);
	printf("\ndestroy object successfully\n");
#endif

	return 0;
}
