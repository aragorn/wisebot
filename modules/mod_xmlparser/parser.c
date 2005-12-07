/* $Id$ */
#include "softbot.h"
#include "parser.h"

/* XXX iconv libary in /usr/lib has fatal error on AIX.
 *     be sure to link lib/libiconv.a (install from srclib : make iconv-install)
 *     include include/iconv/iconv.h.
 *     see AIXPorting wiki page for more information.
 *     --aragorn, 2004/02/10
 */
#ifdef SRCLIB_ICONV
#  include "iconv/iconv.h"
#elif defined(USR_LOCAL_INCLUDE_ICONV)
#  include "/usr/local/include/iconv.h"
#else
#  include <iconv.h>
#endif

#ifdef SRCLIB_EXPAT
#  include "expat/expat.h"
#else
#  include <expat.h>
#endif

#include "stack.h"
#include "dh.h"
#include "checksum.h"

#ifdef HAVE_ALLOCA
	#define USE_ALLOCA
#endif

#define PARSER_HEADER_CRC_CHECK
//#undef PARSER_HEADER_CRC_CHECK

struct _parser_t {
	/* non-saving data */
	dh_t *dh;
	void *db;

	/* saving data: should be same with struct _saving_part_parser_t */
#ifdef PARSER_HEADER_CRC_CHECK
	unsigned long crc;
#endif
	int usedsize;

	int allocatedsize;
	int perm;
#define PERM_RD			1
#define PERM_WR			2
#define PERM_RD_ONLY	1
#define PERM_RD_WR		3

	void *userdata;
	readdb read_db_func;

	void *(*malloc)(size_t);
	void *(*realloc)(void *, size_t);
	void (*free)(void *);

	char charset[MAX_CHARSET_LEN];
};

struct _saving_part_parser_t {
#ifdef PARSER_HEADER_CRC_CHECK
	unsigned long crc;
#endif
	int usedsize;
	char charset[MAX_CHARSET_LEN];
};

typedef struct {
	void *rsv1;

	/* data can be expose to caller */
	int size;

	void *rsv2;

	unsigned char attrnum;
	unsigned char flag;
	char padding[2];

	char name[MAX_FIELD_NAME_LENGTH];

	/* data db internal purpose */
	int offset;
#define FLAG_CDATA_SECTION			1
} node_t;

#define MAX_PATH_DEPTH				8
typedef struct {
	parser_t *p;
	xmlparser_stack_t *st;

	int depth;
	int panicerror;

	iconv_t iconv_unknown_to_unicode;
	iconv_t iconv_utf8_to_unknown;

	node_t nodes[MAX_PATH_DEPTH];
	char path[MAX_PATH_DEPTH][MAX_FIELD_NAME_LENGTH];
} localdata_t;

#define DEFAULT_DB_SIZE				4096
#define EXPAND_DB_SIZE				4096

/* initialize node structure
 * a: pointer of node structure 
 * b: name of node */
#define INIT_NODE(a,b)			strncpy((a)->name,(b),MAX_FIELD_NAME_LENGTH); \
								(a)->name[MAX_FIELD_NAME_LENGTH-1] = '\0'; \
								(a)->rsv1 = NULL; \
								(a)->size = 0; (a)->attrnum = 0; \
								(a)->rsv2 = NULL; \
								(a)->flag = 0; (a)->offset = -1
/* convert node structure to field structure
 * a: pointer of parser object
 * b: pointer of node structrue to convert
 * c: pointer of field structure to get result */
#define CONVERT_NODE_TO_FIELD(a,b,c) \
	if ((a)->read_db_func == NULL) { \
		(b)->rsv1 = (a)->db + (b)->offset + sizeof(attribute_t)*(b)->attrnum; \
		(b)->rsv2 = (a)->db + (b)->offset; \
		(c) = (field_t *)(b); \
	} else { \
		(b)->rsv1 = (a)->read_db_func((a)->userdata, \
							  (b)->offset+sizeof(attribute_t)*(b)->attrnum, \
							  (b)->size); \
		(b)->rsv2 = (a)->read_db_func((a)->userdata, \
							  (b)->offset, \
							  sizeof(attribute_t)*(b)->attrnum); \
		(c) = (field_t *)(b); \
	}
/* calculate memory address of value
 * a: pointer of parser object
 * b: pointer of node object */
#define GET_VALUE(a,b)				((a)->db+(b)->offset+sizeof(attribute_t)*(a)->attrnum)
/* calculate memory address of attribute
 * a: pointer of parser object
 * b: pointer of node object */
#define GET_ATTR_ARRAY(a,b)			((a)->db+(b)->offset)
#define ENOUGH_SPACE				4096
/* approximate data base size
 * a: string of xml document */
#define APPROXIMATE_DB_SIZE(a)		(strlen(a)+ENOUGH_SPACE)

/* calculate pointer to copy data in db
 * a: pointer of parser object */
#define DB_DEST(a)					((a)->db+(a)->usedsize)
/* calclate offset to copy data in db
 * a: pointer of parser object */
#define DB_DEST_OFFSET(a)			((a)->usedsize)
/* check overflow if more data is inserted
 * a: pointer of parser object
 * b: data size that will be inserted */
#define IS_OVERFLOW(a,b)				((a)->usedsize+(b)>(a)->allocatedsize)

static int get_path(localdata_t *data, char buf[], int len);
static int parse_with_expat(parser_t *p, const char *xmltext, const int len);
static void start_element_handler(void *data, const char *el, const char **attr);
static void end_element_handler(void *data, const char *el);
static void character_handler(void *data, const char *s, int len);
static void start_cdata_section_handler(void *data);
static int convert_charset(void *data, const char *str);
static void release_iconv(void *data);
static int unknown_encoding_handler(void* encodingHandlerData,
							 const XML_Char *name, XML_Encoding *info);
static int expanddblen(parser_t *p, const int len);
static int expanddb(parser_t *p);

char unicode_name[SHORT_STRING_SIZE];

/* creates xml parser object and parse xml document and 
 * returns parser object. length of xml document is needed.
 *
 * charset: character set of xml document to parse
 * xmltext: string of xml document to parse */
parser_t *parselen(const char *charset, const char *xmltext, const int len)
{
	parser_t *p;

	//info("a. charset:%p, xmltext:%p, len:%x", charset, xmltext, len);
	p = parser_create(charset);
	if (p == NULL) {
		info("parser_create(charset=\"%s\") returned NULL. xmltext:%p, len:%x", charset, xmltext, len);
		return NULL;
	}
	debug("parser_create(charset=\"%s\") returned %p", charset, p);

	if (expanddblen(p, APPROXIMATE_DB_SIZE(xmltext)) == -1) {
		info("expanddblen() returned -1. charset:%p, xmltext:%p, len:%x", charset, xmltext, len);
		free_parser(p);
		return NULL;
	}

	if (parse_with_expat(p, xmltext, len) == -1) {
		info("c. charset:%p, xmltext:%p, len:%x", charset, xmltext, len);
		free_parser(p);
		return NULL;
	}

	return p;
}

/* if you want to customize allocation function and other
 * hookable handler, first create parser object by parser_create
 * function, second set handler and finaly parse xml text with
 * parselen2 or parse2 function */
int parselen2(parser_t *p, const char *xmltext, const int len)
{
	if (expanddblen(p, APPROXIMATE_DB_SIZE(xmltext)) == -1) {
		free_parser(p);
		return -1;
	}

	if (parse_with_expat(p, xmltext, len) == -1) {
		free_parser(p);
		return -1;
	}

	return 1;
}

/* create parser object and parse xml document and returns
 * parser object. this method is just wrapper function of
 * 'parserlen' function. input xml document string has null
 * character end. */
parser_t *parse(const char *charset, const char *xmltext)
{
	return parselen(charset, xmltext, strlen(xmltext));
}

/* wapper function of parselen2 function */
int parse2(parser_t *p, const char *xmltext)
{
	return parselen2(p, xmltext, strlen(xmltext));
}

/* create xml parser object and set default value
 * charset: character set to use if you want to use default 
 * charset just send null */
parser_t *parser_create(const char *charset)
{
	parser_t *p;

	/* allocate parser object */
	p = (parser_t *)sb_malloc(sizeof(parser_t));
	if (p == NULL) {
		error("cannot create parser object: %s", strerror(errno));
		return NULL;
	}

	/* initialize parser object */
	if (charset) {
		strncpy(p->charset, charset, MAX_CHARSET_LEN);
		p->charset[MAX_CHARSET_LEN-1] = '\0';
	} else
		p->charset[0] = '\0';

	p->dh = dh_create(sizeof(node_t));
	if (p->dh == NULL) {
		error("cannot create dynamic hash object");
		sb_free(p);
		return NULL;
	}

	p->db = NULL;
	p->usedsize = 0;
	p->allocatedsize = 0;

	p->perm = PERM_RD_WR;

	p->userdata = NULL;
	p->read_db_func = NULL;

	p->malloc = NULL;
	p->realloc = NULL;
	p->free = NULL;

	return p;
}

/* free xml parser object
 * return nothing */
void free_parser(parser_t *p)
{
	if (p == NULL) return;
	dh_destroy(p->dh);
	p->dh = NULL;
	if (p->perm & PERM_WR) {
		sb_free(p->db);
		p->db = NULL;
	}
	sb_free(p);
}

/* create xml parser object and load dom object to
 * it and returns.
 * 'loaddom' method duplicate data. so, you can
 * free data after calling this method.
 *
 * data: dom data saved by 'savedom' or manualy saved
 * with 'savedom2'. 
 * len: input size of buffer(data), output total size */
parser_t *loaddom(void *data, int *len)
{
	int offset, tmp;
	parser_t *p;
#ifdef PARSER_HEADER_CRC_CHECK
	unsigned long crc;
#endif

	if (*len < sizeof(parser_t)) {
		return NULL;
	}

	/* allocate parser object */
	p = (parser_t *)sb_malloc(sizeof(parser_t));
	if (p == NULL) {
		// error("cannot create parser object: %s", strerror(errno));
		return NULL;
	}

	memcpy(p, data, sizeof(struct _saving_part_parser_t));
	offset = sizeof(struct _saving_part_parser_t);

	tmp = *len - offset;
	p->dh = dh_load(data + offset, &tmp);
	if (p->dh == NULL) {
		// error("cannot create dynamic hash object");
		sb_free(p);
		return NULL;
	}
	offset += tmp;
#ifdef PARSER_HEADER_CRC_CHECK
	crc = checksum(0, data + sizeof(unsigned long),
				   offset - sizeof(unsigned long));
	if (crc != p->crc) {
		// error("crc error: parser header chuck is wrong");
		dh_destroy(p->dh);
		sb_free(p);
		return NULL;
	}
#endif

	if (*len < offset + p->usedsize) {
		dh_destroy(p->dh);
		sb_free(p);
		return NULL;
	}

	p->db = sb_malloc(p->usedsize);
	if (p->db == NULL) {
		dh_destroy(p->dh);
		sb_free(p);
		return NULL;
	}
	p->allocatedsize = p->usedsize;

	memcpy(p->db, data + offset, p->usedsize);
	offset += p->usedsize;

	p->read_db_func = NULL;
	p->userdata = NULL;
	p->malloc = NULL;
	p->realloc = NULL;
	p->free = NULL;

	p->perm = PERM_RD_WR;

	*len = offset;
	return p;
}

/* creates xml parser object and load dom object to
 * it and returns.
 * 'loaddom2' method does not duplicate data. so,
 * do not free data after calling this method.
 *
 * data: dom data saved by 'savedom' or manualy 
 * saved with 'savedom2'.
 * len: input size of buffer(data), output total size */
parser_t *loaddom2(void *data, int *len)
{
	int offset, tmp;
	parser_t *p;
#ifdef PARSER_HEADER_CRC_CHECK
	unsigned long crc;
#endif

	/* allocate parser object */
	p = (parser_t *)sb_malloc(sizeof(parser_t));
	if (p == NULL) {
		// error("cannot create parser object: %s", strerror(errno));
		return NULL;
	}

	memcpy(p, data, sizeof(struct _saving_part_parser_t));
	offset = sizeof(struct _saving_part_parser_t);

	tmp = *len - offset;
	p->dh = dh_load2(data + offset, &tmp);
	if (p->dh == NULL) {
		// error("cannot create dynamic hash object");
		return NULL;
	}
	offset += tmp;

#ifdef PARSER_HEADER_CRC_CHECK
	/* compare checksum of parser header chunk */
	crc = checksum(0, data + sizeof(unsigned long),
				   offset - sizeof(unsigned long));
	if (crc != p->crc) {
		// error("crc error: parser header chuck is wrong");
		dh_destroy(p->dh);
		sb_free(p);
		return NULL;
	}
#endif

	p->db = data + offset;
	p->allocatedsize = p->usedsize;
	offset += p->usedsize;

	p->read_db_func = NULL;
	p->userdata = NULL;
	p->malloc = NULL;
	p->realloc = NULL;
	p->free = NULL;

	p->perm = PERM_RD_ONLY;

	*len = offset;
	return p;
}

/* in case you don't want read whole db from disk because
 * you don't want whole data or reading db takes too much so
 * you want to read db later, read parser header chunk first
 * and set readdb handler which access db(file) and read db 
 * partly 
 *
 * data: pointer of header chunk
 * len: input estimated length of header chunk,
 *      output exact length of header chunk
 * func: readdb handler
 * userdata: data for readdb handler, pass to readdb handler
 *           as first parameter */
parser_t *loaddom3(void *data, int *len, readdb func, void *userdata)
{
	int offset, tmp;
	parser_t *p;
#ifdef PARSER_HEADER_CRC_CHECK
	unsigned long crc;
#endif

	/* allocate parser object */
	p = (parser_t *)sb_malloc(sizeof(parser_t));
	if (p == NULL) {
		// error("cannot create parser object: %s", strerror(errno));
		return NULL;
	}

	memcpy(p, data, sizeof(struct _saving_part_parser_t));
	offset = sizeof(struct _saving_part_parser_t);

	tmp = *len - offset;
	p->dh = dh_load2(data + offset, &tmp);
	if (p->dh == NULL) {
		// error("cannot create dynamic hash object");
		return NULL;
	}
	offset += tmp;

#ifdef PARSER_HEADER_CRC_CHECK
	/* compare checksum of parser header chunk */
	crc = checksum(0, data + sizeof(unsigned long),
				   offset - sizeof(unsigned long));
	if (crc != p->crc) {
		// error("crc error: parser header chuck is wrong");
		dh_destroy(p->dh);
		sb_free(p);
		return NULL;
	}
#endif

	p->db = NULL;
	p->allocatedsize = 0;

	p->read_db_func = func;
	p->userdata = userdata;

	p->malloc = NULL;
	p->realloc = NULL;
	p->free = NULL;

	p->perm = PERM_RD_ONLY;

	*len = offset;
	return p;
}

/* save data of xml parser object to sequential memory.
 * this method duplicate all infomation data of parser and 
 * text data of document to buf, so it will take a lot.
 * if you want save dom object to file, use 'savedom2' method
 * and get vectors to save to file and save to file with
 * writev(3) function, I recommend.
 * return total dom size
 *
 * p: parser object
 * buf: buffer to save parser data
 * len: length of buffer */
int savedom(parser_t *p, void *buf, int len)
{
	int offset, ret;
#ifdef PARSER_HEADER_CRC_CHECK
	unsigned long crc;
#endif

	if (buf == NULL) return -1;

	if (len < sizeof(struct _saving_part_parser_t)) {
		return -1;
	}
	memcpy(buf, p, sizeof(struct _saving_part_parser_t));
	offset = sizeof(struct _saving_part_parser_t);
	
	if ((ret = dh_save(p->dh, buf + offset, len - offset)) == -1) {
		return -1;
	}
	offset += ret;

#ifdef PARSER_HEADER_CRC_CHECK
	/* checksum for header chunk */
	crc = checksum(0, (void *)buf + sizeof(unsigned long), 
				   offset - sizeof(unsigned long));
	/* save checksum */
	memcpy(buf, &crc, sizeof(unsigned long));
#endif

	if (len < offset + p->usedsize) {
		return -1;
	}
	memcpy(buf+offset, p->db, p->usedsize);
	offset += p->usedsize;


	return offset;
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
int savedom2(parser_t *p,
		     void **buf1, int *len1,
		     void **buf2, int *len2,
		     void **buf3, int *len3,
		     void **buf4, int *len4,
		     void **buf5, int *len5)
{
#ifdef PARSER_HEADER_CRC_CHECK
	unsigned long crc;
#endif

	/* checksum for header chunk */
	*buf1 = (void *)p; *len1 = sizeof(struct _saving_part_parser_t);
#ifdef PARSER_HEADER_CRC_CHECK
	crc = checksum(0, *buf1, *len1);
#endif

	dh_save2(p->dh, buf2, len2, buf3, len3, buf4, len4);
#ifdef PARSER_HEADER_CRC_CHECK
	crc = checksum(crc, *buf2, *len2);
	crc = checksum(crc, *buf3, *len3);
	crc = checksum(crc, *buf4, *len4);

	/* save checksum */
	p->crc = crc;
#endif

	*buf5 = p->db; *len5 = p->usedsize;

	return *len1 + *len2 + *len3 + *len4 + *len5;
}

/* return total size of dom 
 *
 * p: parser object */
int get_domsize(parser_t *p)
{
	return sizeof(struct _saving_part_parser_t) +
			dh_get_savingsize(p->dh) +
			p->usedsize;
}

/* return field object retrieved by a query.
 * query can have only format of description of README.xmlparser.c
 * in doc/ directory, limited support of xpath language.
 * CAUTION: the field object returned by this method must not be
 *          freed by caller.
 *
 * p: parser object
 * query: xpath query string */
field_t *retrieve_field(parser_t *p, char *query)
{
	node_t *node;
	field_t *field; 

	if (dh_search(p->dh, query, (void **)&node) == -1) {
		return NULL;
	}

	CONVERT_NODE_TO_FIELD(p, node, field);

	return field;
}

/* return attribute object retrieved by a query.
 * query can have only format of /string[@string] 
 * description in README.xmlparser.c, limited support of
 * xpath language.
 * this method is not proper to xpath query, but it is
 * convenient purpose.
 * CAUTION: the attribute object returned by this method 
 *          must not be freed by caller.
 *
 * p: parser object
 * qeury: xpath query string */
attribute_t *retrieve_attr(parser_t *p, const char *query)
{
	int i;
	const char *tmp1, *tmp2;
	char path[MAX_KEY_LEN], attr[MAX_ATTRIBUTE_NAME_LENGTH];
	node_t *node;
	field_t *field;

	tmp1 = query;
	if ((tmp2 = strrchr(query, '[')) == NULL) {
		// error("query format is not proper");
		return NULL;
	}
	for (i=0; tmp1<tmp2; tmp1++, i++) path[i] = *tmp1;
	path[i] = '\0';

	if (*(++tmp1) != '@') {
		// error("query format is not proper");
		return NULL;
	}

	tmp1++;
	if ((tmp2 = strrchr(query, ']')) == NULL) {
		// error("query format is not proper");
		return NULL;
	}
	for (i=0; tmp1<tmp2; tmp1++, i++) attr[i] = *tmp1;
	path[i] = '\0';

	if (dh_search(p->dh, path, (void **)&node) == -1) {
		return NULL;
	}
	CONVERT_NODE_TO_FIELD(p, node, field);

	for (i=0; i<field->attrnum && 
			  strncasecmp(field->attrs[i].name, attr, MAX_ATTRIBUTE_NAME_LENGTH);
			  i++);

	return &(field->attrs[i]);
}

void *query(parser_t *p, char *query)
{
	return NULL;
}


static int get_path(localdata_t *data, char buf[], int len)
{
	localdata_t *d = (localdata_t *)data;
	int i;

	buf[0] = '/'; buf[1] = '\0'; len -= 2;
	for (i=0; i<d->depth; i++) {
		len -= strlen(d->path[i]) + 1;
		if (len < 0) return -1;
		strcat(buf, d->path[i]);
		strcat(buf, "/");
	}
	buf[strlen(buf)-1] = '\0';
	return 1;
}

static int parse_with_expat(parser_t *p, const char *xmltext, const int len)
{
	XML_Parser expat;
	localdata_t data;
	void *stack;
	int ret;

    memset(&data, 0x00, sizeof(localdata_t));
	//info("1. p:%p, p->charset:%s, xmltext:%p, len:%x", p, p->charset, xmltext, len);

	/* create expat parser object */
	if ( p->charset[0] != '\0' ) {
		expat = XML_ParserCreate(p->charset);
	} else {
		expat = XML_ParserCreate(NULL);
	}

	if ( expat == NULL ) {
		error("XML_ParserCreate returned NULL: %s", strerror(errno)); 
		return -1;
	}

	/* initial local data */
	data.p = p;
	data.depth = 0;
	stack = (xmlparser_stack_t *)sb_malloc(APPROXIMATE_DB_SIZE(xmltext));
	if (stack == NULL) {
		error("cannot malloc (xmlparser_stack_t *)stack: %s", strerror(errno));
		XML_ParserFree(expat);
		return -1;
	}
	data.st = st_create2(stack, APPROXIMATE_DB_SIZE(xmltext));
	if (data.st == NULL) {
		sb_free(stack);
		XML_ParserFree(expat);
		return -1;
	}

	//info("2. p:%p, p->charset:%s, xmltext:%p, len:%x", p, p->charset, xmltext, len);

	if (p->charset[0]) {
		//info("setting up charset conversion for [%s]", p->charset);
		data.iconv_utf8_to_unknown = iconv_open(p->charset, "UTF-8");
	
		if (data.iconv_utf8_to_unknown == (iconv_t)-1) {
			error("iconv_open(to=\"%s\", from=\"%s\") failed: %s", p->charset, "UTF-8", strerror(errno));
			error("setting up charset conversion failed, but this  happen [%s]", strerror(errno));
			st_destroy(data.st);
			sb_free(stack);
			XML_ParserFree(expat);
			return -1;
		}
	}
	else
		data.iconv_utf8_to_unknown = (iconv_t)-1;

//	info("data.iconv_utf8_to_unknown = %p", data.iconv_utf8_to_unknown);


	data.panicerror = 0;

	/* set user data of expat parser */
	XML_SetUserData(expat, &data);

	/* set unknown encoding handler */
	if (p->charset[0]) {
		//info("setting unknown to unicode convertor [%s]", p->charset);
		XML_SetUnknownEncodingHandler(expat, unknown_encoding_handler, 
							  (void *)&(data.iconv_unknown_to_unicode));
	//	info("data.iconv_unknown_to_unicode = %p", data.iconv_unknown_to_unicode);

		}

	/* set handler of expat parser */

	XML_SetElementHandler(expat, start_element_handler, end_element_handler);
	XML_SetCharacterDataHandler(expat, character_handler);
	XML_SetStartCdataSectionHandler(expat, start_cdata_section_handler);

	/* parse xml string */
	ret = 1;

	if (XML_Parse(expat, xmltext, len, 1) == 0) {
		error("cannot parse xml document: at line %d: %s",
		       XML_GetCurrentLineNumber(expat),
		       XML_ErrorString(XML_GetErrorCode(expat)));
		ret = -1;
	}

	XML_ParserFree(expat);
	st_destroy(data.st);
	if (p->charset[0]) iconv_close(data.iconv_utf8_to_unknown);

	sb_free(stack);

	return ret;
}

static void start_element_handler(void *data, const char *el, const char **attr)
{
	localdata_t *d = (localdata_t *)data;
	attribute_t tmpattr;
	int i;

	if (d->panicerror) return;

	if (d->depth >= MAX_PATH_DEPTH) {
		d->depth++;
		return;
	}

	/* initialize node */
	INIT_NODE(&(d->nodes[d->depth]), el);

	/* push */
	if (st_push(d->st, NULL, 0) == -1) {
		return;
	}

	/* push element name */
	strncpy(d->path[d->depth], el, MAX_FIELD_NAME_LENGTH);
	d->path[d->depth][MAX_FIELD_NAME_LENGTH-1] = '\0';

	/* push attribute */
	for (i=0; attr[i]; i+=2) {
		strncpy(tmpattr.name, attr[i], MAX_ATTRIBUTE_NAME_LENGTH);
		tmpattr.name[MAX_ATTRIBUTE_NAME_LENGTH-1] = '\0';
		strncpy(tmpattr.value, attr[i+1], MAX_ATTRIBUTE_VALUE_LENGTH);
		tmpattr.value[MAX_ATTRIBUTE_VALUE_LENGTH-1] = '\0';

		st_append(d->st, &tmpattr, sizeof(tmpattr));
		d->nodes[d->depth].attrnum++;
	}

	d->depth++;
}

static void end_element_handler(void *data, const char *el)
{
	localdata_t *d = (localdata_t *)data;
	char key[MAX_KEY_LEN];
	void *ptr;
	int len;

	if (d->panicerror) return;

	if (d->depth > MAX_PATH_DEPTH) {
		d->depth--;
		return;
	}

	/* get path name that will be used as key */
	get_path(d, key, MAX_KEY_LEN);

	if (--d->depth < 0) {
		return;
	}

	/* pop attributes & character data of this element */
	if (st_pop2(d->st, &ptr, &len) == -1) {
		return;
	}

	/* copy attributes & character data to db */
	while (IS_OVERFLOW(d->p, len)) {
		if (expanddb(d->p) == -1) {
			return;
		}
	}

	if (ptr) memcpy(DB_DEST(d->p), ptr, len);

	/* set node finally */
	d->nodes[d->depth].offset = DB_DEST_OFFSET(d->p);
	d->nodes[d->depth].size = len;

	d->p->usedsize += len;

	/* insert node to dynamic hash */
	if (dh_insert(d->p->dh, key, &(d->nodes[d->depth])) == -1) {
/*		fprintf(stderr, "error in dh_insert\n");*/
/*		if (dhpanic) {*/
/*			fprintf(stderr, "!!!!! DH PANIC !!!!!\n");*/
/*		}*/
		d->panicerror = 1;
		return;
	}
}

#define CONVERTING_UNIT_BYTE			256
static void character_handler(void *data, const char *s, int len)
{
	localdata_t *d = (localdata_t *)data;
	iconv_t iconv_handle = d->iconv_utf8_to_unknown;
	size_t ibuflen, obuflen, ret;
	char obuf[CONVERTING_UNIT_BYTE];
#if defined (SOLARIS)
	const char *iptr;
#else
	char *iptr;
#endif
	char *optr;

	if (d->panicerror) return;

	if (iconv_handle == (iconv_t)-1) {
		if (st_append(d->st, (void *)s, len) == -1) {
			// error("cannot append character data to top of stack");
			return;
		}
		return;
	}

	iptr = (char *)s;
	ibuflen = len;
	for ( ; ibuflen>0; ) {
		optr = obuf;
		obuflen = CONVERTING_UNIT_BYTE;
		ret = iconv(iconv_handle, &iptr, &ibuflen, &optr, &obuflen);          
		if (ret == (size_t)-1 && (errno == EILSEQ || errno == EINVAL)) {
			iptr++;
			ibuflen--;
			continue;
		}

		if (st_append(d->st, (void *)obuf, CONVERTING_UNIT_BYTE - obuflen) == -1) {
			// error("cannot append character data to top of stack");
			return;
		}
	}
}

static void start_cdata_section_handler(void *data)
{
	localdata_t *d = (localdata_t *)data;
	d->nodes[d->depth-1].flag |= FLAG_CDATA_SECTION;
}

static int convert_charset(void *data, const char *str)
{
	iconv_t iconv_handle = *(iconv_t *)data;
	size_t ibuflen = 2, obuflen=4, done_count;
#if defined (SOLARIS)
	const char* iptr = str;
#else
	char *iptr = (char *)str;
#endif
	char obuf[20];
	char *optr = obuf;

	done_count = iconv(iconv_handle,&iptr,&ibuflen,&optr,&obuflen);          
	if (done_count!=(size_t)-1 && ibuflen==0) 
	{
		unsigned short b0 = (unsigned char)obuf[0];
		unsigned short b1 = (unsigned char)obuf[1];
#if BYTEORDER == 4321
		return b1 | (b0<<8);
#else
		return b0 | (b1<<8);
#endif
	}
	else {
		unsigned short b0 = '?';
		unsigned short b1 = 0;
#if BYTEORDER == 4321
		return b1 | (b0<<8);
#else
		return b0 | (b1<<8);
#endif
/*		return -1;*/
	}
}

static void release_iconv(void *data)
{
    iconv_close(*(iconv_t *)data);
}

/* this is used by code that reads xml using expat */
static int unknown_encoding_handler(void* encodingHandlerData,
							 const XML_Char *name,
                             XML_Encoding *info)
{
	int i;

	*(iconv_t *)encodingHandlerData = iconv_open(unicode_name,name);
	if (*(iconv_t *)encodingHandlerData == (iconv_t)-1)
	return 0;

	info->data = encodingHandlerData;
	info->convert = convert_charset;
	info->release = release_iconv;
	for(i=0;i<0x80;++i) info->map[i] = i;
	for(;i<=0xFF;++i) info->map[i] = -2;

	return 1;
}

static int expanddblen(parser_t *p, const int len)
{
	void *tmp;
	tmp = sb_realloc(p->db, p->allocatedsize + len);
	if (tmp == NULL) {
		return -1;
	}
	p->db = tmp;
	p->allocatedsize += len;
	return 1;
}

static int expanddb(parser_t *p)
{
	return expanddblen(p, EXPAND_DB_SIZE);
}
