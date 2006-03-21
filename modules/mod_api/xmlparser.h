/* $Id$ */
#ifndef XMLPARSER_H
#define XMLPARSER_H 1

#include "mod_xmlparser/parser.h"

SB_DECLARE_HOOK(parser_t *,xmlparser_parselen, \
	(const char *charset, const char *xmltext, const int len))
SB_DECLARE_HOOK(parser_t *,xmlparser_parser_create,(const char *charset))
SB_DECLARE_HOOK(parser_t *,xmlparser_parse, \
	(const char *charset, const char *xmltext))
SB_DECLARE_HOOK(void,xmlparser_free_parser,(parser_t *p))

SB_DECLARE_HOOK(parser_t *,xmlparser_loaddom,(void *data, int *len))
SB_DECLARE_HOOK(parser_t *,xmlparser_loaddom2,(void *data, int *len))
SB_DECLARE_HOOK(int,xmlparser_savedom,(parser_t *p, void *buf, int len))
SB_DECLARE_HOOK(int,xmlparser_savedom2, \
	(parser_t *p, void **buf1, int *len1, void **buf2, int *len2, \
	void **buf3, int *len3, void **buf4, int *len4, void **buf5, int *len5))
SB_DECLARE_HOOK(int,xmlparser_get_domsize,(parser_t *p))

SB_DECLARE_HOOK(field_t *,xmlparser_retrieve_field,(parser_t *p, char *query))
SB_DECLARE_HOOK(attribute_t *,xmlparser_retrieve_attr,(parser_t *p, const char *query))

#endif
