/* $Id$ */
#ifndef UTIL_H
#define UTIL_H 1

#if USE_APR==1
#include "apr.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_lib.h"
#endif

#if USE_APR==1
SB_DECLARE(char *) sb_escape_html(apr_pool_t *p, const char *s);
SB_DECLARE(int)  sb_is_url(const char *u);
SB_DECLARE(void) sb_str_tolower(char *str);
SB_DECLARE(char *) ap_getword_white(apr_pool_t *p, const char **line);
#endif

SB_DECLARE(const char *) sb_get_server_version(void);
SB_DECLARE(int) sb_server_root_relative(char *buf, const char *filename);
SB_DECLARE(char *) sb_strbin(uint32_t number, int size);

#endif
