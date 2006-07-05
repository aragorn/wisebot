#ifndef __HANDLER_UTIL_H__
#define __HANDLER_UTIL_H__

#include "auto_config.h"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "mod_httpd/mod_httpd.h"
#include "apr_pools.h"

char *replace_newline_to_space(char *str); 
uint32_t push_node_id(uint32_t node_id, uint32_t this_node_id);
uint32_t pop_node_id(uint32_t node_id);
uint32_t get_node_id(uint32_t node_id);
char* escape_operator(apr_pool_t *p, const char *path);
int def_atoi(const char *s, int def);
int hex(unsigned char h);
void decodencpy(unsigned char *dst, unsigned char *src, int n);
int equals_content_type(request_rec *r, const char* ct);

#endif
