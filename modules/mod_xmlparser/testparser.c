/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "parser.h"

#define BUF_SIZE			(1024 * 1024)
//#define REPEAT_TEST			10000

#include "softbot.h"
char gSoftBotRoot[MAX_PATH_LEN] = SERVER_ROOT;
//char gErrorLogFile[MAX_PATH_LEN] = DEFAULT_ERROR_LOG_FILE;
module *static_modules;

int keynum = 4;
char *keys[] = {"/Document/Author", "/Document/Title", "/Document/Body", "/Document/URL"};

int test_retrieve(parser_t *p) {
	field_t *f;
	char tmp[BUF_SIZE];
	int i;

	/* test retrieve */
	for (i=0; i<keynum; i++) {
		f = retrieve_field(p, keys[i]);
		if (f == NULL) {
			fprintf(stderr, "cannot get field[%s]\n", keys[i]);
			continue;
/*			return -1;*/
		}
		memcpy(tmp, f->value, f->size);
		tmp[f->size] = '\0';
#ifndef REPEAT_TEST
		printf("fieldname[%s], value[%s]\n", f->name, tmp);
#endif
	}

	return 1;
}

int main (int argc, char *argv[]) {
	char buf[BUF_SIZE];
	int size;
	FILE *fp;

	parser_t *p=NULL;

	if (argc != 2 && argc != 3) {
		fprintf(stderr, "usage: testparser [xml file name] [charset name]; "
				"charset name could be omiited\n");
		return 1;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		fprintf(stderr, "cannot open file[%s]: %s\n", argv[1], strerror(errno));
		return 1;
	}

	size = fread(buf, 1, BUF_SIZE, fp);
	buf[size] = '\0';

#ifdef REPEAT_TEST
	{
		int i;
		struct timeval ts, tf;
		gettimeofday(&ts, NULL);
		printf("case[1]: parsing test\n");
		for (i=0; i<REPEAT_TEST; i++) {
#endif

	/* test parse */
#ifndef REPEAT_TEST
	printf("test parse...\n");
#endif
	if (argc == 2) p = parse(NULL, buf);
	else if (argc == 3) p = parse(argv[2], buf);
	if (p == NULL) {
		fprintf(stderr, "cannot parse\n");
		return 1;
	}
/*	printf("parsing successfully\n");*/
	
	/* test retriece */
	if (test_retrieve(p) == -1) {
		fprintf(stderr, "cannot retrieve\n");
		return 1;
	}

	/* test savedom */
#ifndef REPEAT_TEST
	printf("test save...\n");
	size = savedom(p, buf, BUF_SIZE);
	if (size == -1) {
		fprintf(stderr, "cannot save\n");
		return 1;
	}
#endif

	/* test free */
#ifndef REPEAT_TEST
	printf("test free_parser...\n");
#endif
	free_parser(p);

#ifdef REPEAT_TEST
		}
		gettimeofday(&tf, NULL);
		printf("%d repeat takes %ld sec.\n", i, tf.tv_sec - ts.tv_sec);
	}
#endif

//#ifndef REPEAT_TEST
#if 0
	/* test loaddom */
	printf("test loaddom...\n");
	if (p == NULL) {
		fprintf(stderr, "cannot loaddom\n");
		return 1;
	}

	/* test retriece */
	if (test_retrieve(p) == -1) {
		fprintf(stderr, "cannot retrieve\n");
		return 1;
	}

	/* test free */
	printf("test free_parser...\n");
	free_parser(p);

	/* test loaddom2 */
	printf("test loaddom2...\n");
	p = loaddom(buf, &size);
	if (p == NULL) {
		fprintf(stderr, "cannot loaddom2\n");
		return 1;
	}

	/* test retriece */
	if (test_retrieve(p) == -1) {
		fprintf(stderr, "cannot retrieve\n");
		return 1;
	}

	/* test free */
	printf("test free_parser...\n");
	free_parser(p);
#endif

	return 0;
}
