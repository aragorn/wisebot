/* $Id$  */
/* XXX under-processing...
 *
 *		Simple Persistant-DOMs XML Parser
 *
 *	features
 *	1) this parser is so simple that just only can retrieve 
 *	   field and attribute
 *	2) the dom object that is got after parsing can not be
 *	   modified
 *	3) the dom object is packed. so, it can be stored to file 
 *	   and loaded in whole to a sequetial memory
 *	4) all the result data is not duplicated one. so, should
 *	   not be freed by caller
 */


/* data */

// external
typedef struct parser_t parser_t;
struct parser_t {
	/* is need to be known to caller? */
};

typedef struct {
	char name[MAX_FIELD_NAME_LENGTH];
	int len;
	char *value;
	int attrnum;
	attribute_t *attrs;
} field_t;

typedef struct {
	char name[MAX_ATTRIBUTE_NAME_LENGTH];
	char value[MAX_ATTRIBUTE_VALUE_LENGTH];
} attribute_t;

/**
 * internal
 *

data start

header	.	<parser header-object length>
		.	<document & parser infomation>
		.	<field-infomations used by dynamic hash>
				: data of dynamic hash is composed of three element
				1. dynamic hash infomation
				2. directory array
				3. buckets
				- data that insert to bucket may be equal to
				  node infomation, so think about some trick.

cf) key of dynamic hash is query string of xpath

body	.	<attribute objects>
				: array of attribute_t structure
		.	<field value>
				: variable length data
		.
		.
		.
		.
		.
		.
		.

cf) body has node informations

data end

*/

/**
 *  XPath
 *

this parser supports xpath grammar limitedly.
just these three features.

1. path query ::= /root/children
   root ::= fieldname
   children ::= fieldname | fieldname/children

2. path query ::= //element
   element ::= fieldname | fieldname/element

3. fieldname ::= string | string[@attribute="string"]
   attribute ::= string

this parser assume that every field can be retrieved by
xpath query identically. that is, a field cannot have
more than one children of same field name.
this limitation will be fixed some day.

*/
#define MAX_CHARSET_LEN				32
typedef struct {
	/* saving data */
	char charset[MAX_CHARSET_LEN];

	/* non-saving data */
	dh_t *dh;
} parser_t;

typedef struct {
	char name[MAX_FIELD_NAME_LENGTH];
	int offset;
	int size;
} node_t;

/* Interface */

/* creates xml parser object and returns
 *
 * charset: character set of xml document to parse
 * xmltext: string of xml document to parse */
parser_t *parse(const char *charset, const char *xmltext)
{
}

/* free xml parser object
 * return nothing */
void free_parser(parser_t *p)
{
}

/* creates xml parser object and load dom object to
 * it and returns.
 * 'loaddom1' method does not duplicate data. so,
 * do not free data after calling this method.
 *
 * data: dom data saved by 'savedom1' or manualy 
 * saved with 'savedom2'. */
parser_t *loaddom1(void *data, int len)
{
}

/* create xml parser object and load dom object to
 * it and returns.
 * 'loaddom2' method duplicate data. so, you can
 * free data after calling this method.
 *
 * data: dom data saved by 'savedom1' or manualy saved
 * with 'savedom2'. */
parser_t *loaddom2(void *data, int len)
{
}

/* save data of xml parser object to sequential memory.
 * this method duplicate all infomation data of parser and 
 * text data of document to buf, so it will take a lot.
 * if you want save dom object to file, use 'savedom2' method
 * and get vectors to save to file and save to file with
 * writev(3) function, I recommend.
 *
 * p: parser object
 * buf: buffer to save parser data
 * len: length of buffer */
int savedom1(parser_t *p, void *buf, int len)
{
}

/* return vectors of parser data, just save these datas that
 * vectors indicate in sequential memory. caller need not to
 * know what each vector means. just save in sequential memory.
 * if you want to save dom object to file, I recommend to save
 * with writev(3) function.
 *
 * p: parser object
 * buf1, len1: a pair to return vector 
 * ... */
int savedom2(parser_t *p, void **buf1, int *len1, ...)
{
}

/* return field object retrieved by a query.
 * query can have only format of above description
 * of limited support of xpath language.
 *
 * p: parser object
 * query: xpath query string */
field_t *retrieve_field(parser_t *p, char *query)
{
}

/* return attribute object retrieved by a query.
 * query can have only format of /string[@string] 
 * above description of limited support of xpath language.
 * this method is not proper to xpath query, but it is
 * convenience purpose.
 *
 * p: parser object
 * qeury: xpath query string */
attribute_t *retrieve_attr(parser_t *p, char *query)
{
}

/* above two method is just wapping function of this.
 */
static node_t *query(parser_t *p, char *query)
{
}

static int parse_query(char *query)
{
}
