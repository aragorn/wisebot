#ifndef _TRIEDIC_H
#define _TRIEDIC_H 1

#include "trie.h"
#include "manage_dict.h"

int triedic_init(dic_t *dic);
int triedic_open(dic_t *dic);
void triedic_close(dic_t *dic);
int triedic_search(dic_t *dic, const char *keyword, void *buf, int bufsize);

#endif
