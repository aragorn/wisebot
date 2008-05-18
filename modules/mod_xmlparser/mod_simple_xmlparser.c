/* $Id$ */
#ifndef TEST_PROGRAM

#  include "common_core.h"
#  include "memory.h"

#  include "mod_api/xmlparser.h"

#else // TEST_PROGRAM

#  include "constants.h"
#  include <stdio.h>
#  include <errno.h>

#  define sb_malloc malloc
#  define sb_free free

#  define error printf
#  define warn printf
#  define info printf

#endif

#include <string.h> // strcpy
#include <stdlib.h> // atoi
#include <ctype.h>  // isspace

typedef struct {
	char rootnode[SHORT_STRING_SIZE];
	int fieldcount;
	char fieldnames[MAX_FIELD_NUM][SHORT_STRING_SIZE];
	char* fieldvalues[MAX_FIELD_NUM];
	int fieldlengths[MAX_FIELD_NUM];
} parser_t;

/*
 * read_xxxx 함수들은 원하는 text를 읽었으면 읽은 길이를 return한다.
 * 못읽으면 0이다.
 */
#define GETC \
	if ( left == 0 ) { \
		error("unexpected document end"); \
		return 0; \
	} \
	c = *text;

#define GETC2 \
	if ( left == 0 ) return 0; \
	c = *text;

#define SKIPSPACE \
	while ( left > 0 && isspace(c=*text) ) { \
		if ( c == '\n' ) { \
			_line++; \
			_col = 0; \
		} \
\
		NEXT; \
	} \
\
	if ( left == 0 ) { \
		error("unexpected document end"); \
		return 0; \
	}

#define NEXT \
	text++; \
	_col++; \
	count++; \
	left--;

#define RETURN \
	*line = _line; \
	*col = _col; \
	return count;

int read_rootnode_start(const char* text, int left, char* rootnode, int* line, int* col)
{
	int _line = *line;
	int _col = *col;
	int count = 0;
	char c = '\0';

	SKIPSPACE;

	if ( c != '<' ) {
		error("invalid character '%c', expected '<': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	while ( left > 0 && (c=*text) != '>' ) {
		*(rootnode++) = c;

		NEXT;
	}
	*rootnode = '\0';

	if ( c != '>' ) {
		error("invalid character '%c', expected '>': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	RETURN;
}

int read_rootnode_end(const char* text, int left, char* rootnode, int* line, int* col)
{
	int _line = *line;
	int _col = *col;
	int count = 0;
	char c = '\0';

	SKIPSPACE;

	if ( c != '<' ) {
		error("invalid character '%c', expected '<': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	GETC;
	if ( c != '/' ) {
		error("invalid character '%c', expected '/': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	while ( left > 0 && (c=*(rootnode++)) != '\0' ) {
		if ( *text != c ) {
			error("invalid closing root node name: line %d, col %d", _line, _col);
			return 0;
		}
		NEXT;
	}

	GETC;
	if ( c!= '>' ) {
		error("invalid character '%c', expected '>': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	RETURN;
}

int read_field_start(const char* text, int left, char* fieldname, int* line, int* col)
{
	int _line = *line;
	int _col = *col;
	int count = 0;
	char c = '\0';

	SKIPSPACE;

	if ( c != '<' ) {
		error("invalid character '%c', expected '<': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	if ( *text == '/' ) return 0; // maybe </Document>

	while ( left > 0 && (c=*text) != '>' ) {
		*(fieldname++) = c;

		NEXT;
	}
	*fieldname = '\0';

	if ( c != '>' ) {
		error("invalid character '%c', expected '>': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	RETURN;
}

int read_field_end(const char* text, int left, char* fieldname, int* line, int* col)
{
	int _line = *line;
	int _col = *col;
	int count = 0;
	char c = '\0';

	SKIPSPACE;

	if ( c != '<' ) {
		error("invalid character '%c', expected '<': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	GETC;
	if ( c != '/' ) {
		error("invalid character '%c', expected '/': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	while ( left > 0 && (c=*(fieldname++)) != '\0' ) {
		if ( *text != c ) {
			error("invalid closing field name: line %d, col %d", _line, _col);
			return 0;
		}
		NEXT;
	}

	GETC;
	if ( c!= '>' ) {
		error("invalid character '%c', expected '>': line %d, col %d", c, _line, _col);
		return 0;
	}
	NEXT;

	RETURN;
}

int read_cdata(const char* text, int left,
		char** fieldtext, int* fieldlength, int* line, int* col)
{
	int _line = *line;
	int _col = *col;
	int count = 0;
	char c = '\0';

	int state = 0;
	int tmp;

	SKIPSPACE;

	if ( strncmp(text, "<![CDATA[", 9) != 0 ) return 0;
	tmp = count;

	text += 9;
	count += 9;
	_col += 9;
	left -= 9;

	*fieldtext = (char*) text;

	while ( 1 ) {
		GETC2;

		if ( c == '\n' ) {
			_line++;
			_col = 0;
		}

		switch (state) {
			case 0:
				if ( c == ']' ) state = 1;
				break;
			case 1:
				if ( c == ']' ) state = 2;
				else state = 0;
				break;
			case 2:
				if ( c == '>' ) goto match;
				else if ( c != ']' ) state = 0;
				break;
		}

		NEXT;
	}

match:
	NEXT;
	*fieldlength = count-tmp-12;

	RETURN;
}

int read_text(const char* text, int left,
		char** fieldtext, int* fieldlength, int* line, int* col)
{
	int _line = *line;
	int _col = *col;
	int count = 0;
	char c = '\0';

	*fieldtext = (char*)text;

	while ( left > 0 ) {
		GETC2;
		if ( c == '<' ) break;
		else if ( c == '\n' ) {
			_line++;
			_col = 0;
		}

		NEXT;
	}

	*fieldlength = count;

	RETURN;
}

static void* parselen(const char* charset, const char* xmltext, const int len)
{
	int fieldcount = 0;
	int pos = 0; // 현재 읽고 있는 xmltext 위치
	int line = 1, col = 1;
	int r;

	parser_t* parser = (parser_t*) sb_malloc(sizeof(parser_t));

	r = read_rootnode_start(xmltext+pos, len-pos, parser->rootnode, &line, &col);
	if ( r == 0 ) goto error;
	else pos += r;

	while ( 1 ) {
		r = read_field_start(xmltext+pos, len-pos,
				parser->fieldnames[fieldcount], &line, &col);
		if ( r == 0 ) break; // <field>가 아닌 </Document>일 것이다
		else pos += r;

		// CDATA 이거나 text 이거나....
		r = read_cdata(xmltext+pos, len-pos, &parser->fieldvalues[fieldcount],
				&parser->fieldlengths[fieldcount], &line, &col);
		if ( r == 0 ) {
			r = read_text(xmltext+pos, len-pos, &parser->fieldvalues[fieldcount],
				&parser->fieldlengths[fieldcount], &line, &col);
		}

		if ( r != 0 ) pos += r;

		r = read_field_end(xmltext+pos, len-pos,
			parser->fieldnames[fieldcount], &line, &col);
		if ( r == 0 ) {
			error("invalid character at line %d, column %d", line, col);
			goto error;
		}
		else pos += r;

		fieldcount++;
	}

	r = read_rootnode_end(xmltext+pos, len-pos, parser->rootnode, &line, &col);
	if ( r == 0 ) goto error;

	parser->fieldcount = fieldcount;

	return parser;

error:
	sb_free(parser);
	return NULL;
}

// query는 /Document/Title 과 같은 문자열이다
// wisebot의 xml document는 항상 2단이므로 뒤쪽의 Title만 알아보면 된다.
static int retrieve_field(void* p, const char* query, char** field_value, int* field_length)
{
	parser_t* parser = (parser_t*) p;
	int query_len = strlen(query);
	char* fieldname = (char*)query + query_len-1;
	int i;

	for ( ; query != fieldname && *fieldname != '/'; fieldname-- );
	fieldname++;

	for ( i = parser->fieldcount-1; i >= 0; i-- ) {
		if ( strcasecmp( parser->fieldnames[i], fieldname ) == 0 ) {
			*field_value = parser->fieldvalues[i];
			*field_length = parser->fieldlengths[i];
			return SUCCESS;
		}
	}

	return FAIL;
}

static void free_parser(void* p)
{
	sb_free(p);
}

#ifndef TEST_PROGRAM

static void register_hooks(void)
{
	sb_hook_xmlparser_parselen(parselen,NULL,NULL,HOOK_MIDDLE);
	sb_hook_xmlparser_retrieve_field(retrieve_field,NULL,NULL,HOOK_MIDDLE);
	sb_hook_xmlparser_free_parser(free_parser,NULL,NULL,HOOK_MIDDLE);
}

module simple_xmlparser_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,               /* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks		/* register hook api */
};

#else  // TEST_PROGRAM

int main(int argc, char** argv)
{
	if ( argc <= 1 ) {
		fprintf(stderr, "need xml filename\n");
		return 1;
	}

	FILE* fp = fopen(argv[1], "r");
	if ( fp == NULL ) {
		fprintf(stderr, "cannot open file[%s]\n", argv[0]);
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	int length = ftell(fp);
	char* buffer = (char*) sb_malloc( length );

	fseek(fp, 0, SEEK_SET);
	int readlen = fread(buffer, length, 1, fp);
	fclose(fp);

	printf("length: %d, read: %d\n", length, readlen);
	if ( readlen != 1 ) {
		fprintf(stderr, "%s\n", strerror(errno));
		return 1;
	}

	parser_t* p = (parser_t*) parselen("", buffer, length);
	if ( p == NULL ) {
		fprintf(stderr, "parsing error\n");
		return 1;
	}

	printf("root node : %s\n", p->rootnode);
	int i;
	for ( i = 0; i < p->fieldcount; i++ ) {
		p->fieldvalues[i][p->fieldlengths[i]] = '\0';

		char* field_value; int field_length;
		char query[64];
		snprintf(query, sizeof(query), "/%s/%s", p->rootnode, p->fieldnames[i]);

		retrieve_field(p, query, &field_value, &field_length);

		printf("hack field[%s]: %s(%d)\n", p->fieldnames[i], p->fieldvalues[i], p->fieldlengths[i]);
		printf("     field[%s]: %s(%d)\n", p->fieldnames[i], field_value, field_length);
	}

	free_parser(p);
	sb_free(buffer);

	return 0;
}

#endif // TEST_PROGRAM

