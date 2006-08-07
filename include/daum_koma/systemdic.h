#ifndef _SYSTEMDIC_H
#define _SYSTEMDIC_H 1

#include "trie.h"
#include "manage_dict.h"
/* 시스템 사전에서 사용하는 INDEX PARAMETER 의 크기 */
#define		SDIC_INDEX_SIZE		6

typedef struct _systemdic_t systemdic_t;
struct _systemdic_t {
	trie_t *trie;

	int first_char_addr[MAX_DICT_FIRSTS];
	int tags_count;
	TAG_INFO *ptag_info;
};

int	systemdic_init(dic_t *dic);
int	systemdic_open(dic_t *dic);
void systemdic_close(dic_t *dic);
int search_sdict(dic_t *dic, const char *zooword, void *outputbuf, int outputbufsize);
int insert_sdict(dic_t *dic, const char *zooword, void *data, int datasize);

#endif
