/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "handler_util.h"
#include "mod_httpd/http_util.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//--------------------------------------------------------------//
//  *   custom function 
//--------------------------------------------------------------//
char *replace_newline_to_space(char *str) {
    char *ch;
    ch = str;
    while ( (ch = strchr(ch, '\n')) != NULL ) {
        *ch = ' ';
    }
    return str;
}

/*
 * 하위 4bit에 node_id를 push
 * push 된 node_id를 리턴
 */
uint32_t push_node_id(uint32_t node_id, uint32_t this_node_id)
{   
    // 상위 4bit에 내용이 있으면 더이상 depth를 늘릴수 없음.
    if((node_id >> 28) > 0) {
        error("depth overflower[0x%0X]", node_id);
        return 0;
    }
    
    node_id = node_id << 4;
    node_id |= this_node_id;
    
    return node_id;
}

/*
 * 하위 4bit를 pop한다.
 * pop 된 node_id를 리턴.
 */
uint32_t pop_node_id(uint32_t node_id)
{   
    return (node_id >> 4);
}

// 하위 4bit의 node_id 알아내기
uint32_t get_node_id(uint32_t node_id)
{   
    return node_id & 0x0f;
}

/*
 * 검색 연산자의 &, +는 url syntax 로 적용되는 항목으로
 * escape 대상에서 제외되므로 특별하게 변경하여 준다.
 * URL reserved characters
 * $ & + , / : ; = ? @
 */
char* escape_operator(apr_pool_t *p, const char *path)
{
    char *copy = apr_palloc(p, 3 * strlen(path) + 3);
    const unsigned char *s = (const unsigned char *)path;
    unsigned char *d = (unsigned char *)copy;
    unsigned c;

    while ((c = *s)) {
    if (c == '&') {
        *d++ = '%';
        *d++ = '2';
        *d++ = '6';
    } else if( c == '+') {
        *d++ = '%';
        *d++ = '2';
        *d++ = 'B';
    }
    else {
        *d++ = c;
    }
    ++s;
    }
    *d = '\0';
    return copy;
}

int def_atoi(const char *s, int def)
{       
    if (s)
        return atoi(s);
    return def;
}     

int hex(unsigned char h)
{
    if (isdigit(h))
        return h-'0';
    else
        return toupper(h)-'A'+10;
}

void decodencpy(unsigned char *dst, unsigned char *src, int n)
{
    register int x, y;

    x = y = 0;
    while(src[x]) {
        if (src[x] == '+')
            dst[y] = ' ';
        else if (src[x] == '%' && n - x >= 2
                && isxdigit((int)(src[x+1])) && isxdigit((int)(src[x+2]))) {
            dst[y] = (hex(src[x+1]) << 4) + hex(src[x+2]);
            x += 2;
        }
        else
            dst[y] = src[x];
        x++;
        y++;
        if (x >= n)
            break;
    }
    if (y < n)
        dst[y] = 0;
}

int equals_content_type(request_rec *r, const char* ct)
{
    const char *req_type = apr_table_get(r->headers_in, "Content-Type");

    if ( ct == NULL || req_type == NULL) {
        return FAIL;
    }

    if ( strncmp(req_type, ct, strlen(ct)) != 0 ){
        return FAIL;
    }

    return SUCCESS;
}

char* get_ipaddress(apr_pool_t* pool)
{
	static char hostname[APRMAXHOSTLEN+1] = {0};
	static char ipaddr[SHORT_STRING_SIZE+1] = {0};
	static char* tmp_ipaddr = NULL;
	apr_sockaddr_t *sa = NULL;

	if(ipaddr[0] != '\0') return ipaddr;

	if(apr_gethostname(hostname, APRMAXHOSTLEN, pool) != APR_SUCCESS) {
        error("can not get gethostname info");
        return NULL;
	}

	if(apr_sockaddr_info_get(&sa, hostname, APR_UNSPEC, 0, APR_IPV4_ADDR_OK, pool) != APR_SUCCESS) {
        error("can not get sockaddr info");
        return NULL;
	}

	if(apr_sockaddr_ip_get(&tmp_ipaddr, sa) != APR_SUCCESS) {
        error("can not get ip info");
        return NULL;
	}

	info("ip address[%s]", tmp_ipaddr);

	strncpy(ipaddr, tmp_ipaddr, SHORT_STRING_SIZE);

	return ipaddr;
}

