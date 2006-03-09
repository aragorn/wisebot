/* $Id$ */
#ifndef UTIL_STRING_H
#define UTIL_STRING_H 1

#include "apr.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_lib.h"

char* sb_escape_html(apr_pool_t *p, const char *s);
int   sb_is_url(const char *u);
void  sb_str_tolower(char *str);


#endif
