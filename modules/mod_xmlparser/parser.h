/* $Id$ */
#ifndef _XML_PARSER_H_
#define _XML_PARSER_H_ 1

// remove in case of compiling for general use of this xml parser
// <<<<<<<
#include "auto_config.h"
// >>>>>>>

#include "dh.h"
#include "constants.h"

#define MAX_PATHKEY_LEN				MAX_KEY_LEN
#define MAX_CHARSET_LEN				16
typedef struct _parser_t parser_t;
typedef struct _field_t field_t;
typedef struct _attribute_t attribute_t;

/* readdb function hook is to specify database reading
 * function. this must return non-zero if success, null if fail
 * souroff is offset from start point of db that is reservoir
 * of attributes and values of field, db can be in memory,
 * file, so on. this must return the pointer of data that you
 * read from db */
typedef void *(*readdb)(void *data, int souroff, int len);

#define MAX_FIELD_NAME_LENGTH		SHORT_STRING_SIZE
struct _field_t {
	char *value;
	int size;
	attribute_t *attrs;
	unsigned char attrnum;
	unsigned char flag;
        char padding[2];
	char name[MAX_FIELD_NAME_LENGTH];
#define CDATA_SECTION			1
};

#define MAX_ATTRIBUTE_NAME_LENGTH	16
#define MAX_ATTRIBUTE_VALUE_LENGTH	256
struct _attribute_t {
	char name[MAX_ATTRIBUTE_NAME_LENGTH];
	char value[MAX_ATTRIBUTE_VALUE_LENGTH];
};

#if defined (__cplusplus)
extern "C" {
#endif

parser_t *parselen(const char *charset, const char *xmltext, const int len);
parser_t *parse(const char *charset, const char *xmltext);

int parselen2(parser_t *p, const char *xmltext, const int len);
int parse2(parser_t *p, const char *xmltext);

parser_t *parser_create(const char *charset);
void free_parser(parser_t *p);

parser_t *loaddom(void *data, int *len);
parser_t *loaddom2(void *data, int *len);
parser_t *loaddom3(void *data, int *len, readdb func, void *userdata);

int savedom(parser_t *p, void *buf, int len);
int savedom2(parser_t *p, 
			void **buf1, int *len1, 
			void **buf2, int *len2,
		    void **buf3, int *len3, 
			void **buf4, int *len4, 
			void **buf5, int *len5);
int get_domsize(parser_t *p);

field_t *retrieve_field(parser_t *p, char *query);
attribute_t *retrieve_attr(parser_t *p, const char *query);

extern char unicode_name[SHORT_STRING_SIZE];

#if defined (__cplusplus)
}
#endif

#endif
