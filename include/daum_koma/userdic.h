#ifndef _USERDIC_H
#define _USERDIC_H 1

#include "manage_dict.h"

int udict_init(dic_t *dic);
int open_udict(dic_t *dic);
void close_udict(dic_t *dic);
int search_udict(dic_t *dic, const char *zooword, void *outbuf, int bufsize);

#endif
