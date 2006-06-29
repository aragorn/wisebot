/* $Id$ */
#include "common_core.h"
#include "xmlparser.h"

HOOK_STRUCT(
	HOOK_LINK(xmlparser_parselen)
	HOOK_LINK(xmlparser_free_parser)
	HOOK_LINK(xmlparser_retrieve_field)
/*
	HOOK_LINK(xmlparser_parse)
	HOOK_LINK(xmlparser_parser_create)
	HOOK_LINK(xmlparser_loaddom)
	HOOK_LINK(xmlparser_loaddom2)
	HOOK_LINK(xmlparser_savedom)
	HOOK_LINK(xmlparser_savedom2)
	HOOK_LINK(xmlparser_get_domsize)
	HOOK_LINK(xmlparser_retrieve_attr)*/
)

SB_IMPLEMENT_HOOK_RUN_FIRST(void*, xmlparser_parselen, \
	(const char *charset, const char *xmltext, const int len), \
	(charset, xmltext, len), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, xmlparser_retrieve_field, \
	(void *p, const char *query, char** field_value, int* field_length),
	(p, query, field_value, field_length), DECLINE)
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(xmlparser_free_parser,(void* p), (p))
/*
SB_IMPLEMENT_HOOK_RUN_FIRST(parser_t *,xmlparser_parse, \
	(const char *charset, const char *xmltext),(charset,xmltext),(parser_t *)1)

SB_IMPLEMENT_HOOK_RUN_FIRST(parser_t *,xmlparser_parser_create, \
	(const char *charset), (charset),(parser_t *)1)
SB_IMPLEMENT_HOOK_RUN_FIRST(parser_t *,xmlparser_loaddom, \
	(void *data, int *len), (data,len),(parser_t *)1)
SB_IMPLEMENT_HOOK_RUN_FIRST(parser_t *,xmlparser_loaddom2, \
	(void *data, int *len), (data,len),(parser_t *)1)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,xmlparser_savedom, \
	(parser_t *p, void *buf, int len), (p,buf,len),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,xmlparser_savedom2, \
	(parser_t *p, void **buf1, int *len1, void **buf2, int *len2, \
	void **buf3, int *len3, void **buf4, int *len4, void **buf5, int *len5),\
	(p,buf1,len1,buf2,len2,buf3,len3,buf4,len4,buf5,len5),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,xmlparser_get_domsize, \
	(parser_t *p), (p),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(attribute_t *,xmlparser_retrieve_attr, \
	(parser_t *p, const char *query),(p,query),(attribute_t *)1)
*/

