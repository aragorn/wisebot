/* $Id$ */
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "stack.h"

#define REPEAT_TEST				1000000

char *data[] = {"data 1", "data 2", "data 3", "data 4", "data 5", "data 6"};

int main() {
	xmlparser_stack_t *st;
	char tmp[256], *ptr;
	int len, i;
	void *p;

#ifdef REPEAT_TEST
	{
		int j;
		struct timeval ts, tf;
		gettimeofday(&ts, NULL);
		printf("case 1 test...\n");
		for (j=0; j<REPEAT_TEST; j++) {
#endif

#ifndef REPEAT_TEST
	printf("test create...\n");
#endif
	st = st_create();
	if (st == NULL) {
		fprintf(stderr, "cannot create stack\n");
		return 1;
	}

#ifndef REPEAT_TEST
	printf("test push...\n");
#endif
	for (i=0; i<6; i++) {
		if (st_push(st, data[i], strlen(data[i])) == -1) {
			fprintf(stderr, "cannot push[%s]\n", data[i]);
			return 1;
		}
#ifndef REPEAT_TEST
		printf("successfully push[%s]\n", data[i]);
#endif
	}

#ifndef REPEAT_TEST
	printf("test pop...\n");
#endif
	for (i=0; i<6; i++) {
		if (st_pop(st, (void **)&ptr, &len) == -1) {
			fprintf(stderr, "cannot pop\n");
			return 1;
		}
		memcpy(tmp, ptr, len);
		sb_free(ptr);
		tmp[len] = '\0';
#ifndef REPEAT_TEST
		printf("successfully pop[%s]\n", tmp);
#endif
	}

#ifndef REPEAT_TEST
	printf("test destroy...\n");
#endif
	st_destroy(st);

#ifdef REPEAT_TEST
		}
		gettimeofday(&tf, NULL);
		printf("%d repeat takes %ld sec.\n", j, tf.tv_sec - ts.tv_sec);
	}
#endif

#ifndef REPEAT_TEST
	p = alloca(4096);
	if (p == NULL) {
		fprintf(stderr, "cannot alloca");
		return 1;
	}
#ifndef REPEAT_TEST
	printf("test create2...\n");
#endif
	st = st_create2(p, 4096);
	if (st == NULL) {
		fprintf(stderr, "cannot create stack\n");
		return 1;
	}

#ifndef REPEAT_TEST
	printf("test push...\n");
#endif
	for (i=0; i<6; i++) {
		if (st_push(st, data[i], strlen(data[i])) == -1) {
			fprintf(stderr, "cannot push[%s]\n", data[i]);
			return 1;
		}
#ifndef REPEAT_TEST
		printf("successfully push[%s]\n", data[i]);
#endif
	}

#ifndef REPEAT_TEST
	printf("test pop2...\n");
#endif
	for (i=0; i<6; i++) {
		if (st_pop2(st, (void **)&ptr, &len) == -1) {
			fprintf(stderr, "cannot pop\n");
			return 1;
		}
		memcpy(tmp, ptr, len);
		tmp[len] = '\0';
#ifndef REPEAT_TEST
		printf("successfully pop[%s]\n", tmp);
#endif
	}

#ifndef REPEAT_TEST
	printf("test destroy...\n");
#endif
	st_destroy(st);
#endif

	return 0;
}
