#ifndef _HASH_H
#define _HASH_H 1

typedef struct _hash_t hash_t;
struct _hash_t {
	int dirsize;             	/* current directory array size */
	int datasize;               /* data size */
	int dataperbucket;        	/* data number per bucket */
	int usedsize;               /* used bucket number */

	void **dirs;
	int nbuckets;
};


int hash_open(dic_t *dic);
void hash_close(dic_t *dic);
int hash_search(dic_t *dic, const char *key, void *outbuf, int bufsize);
int hash_insert(dic_t *dic, const char *key, void *data, int datasize);
//int hash_update(dic_t *dic, const char *key, void *data, int datasize);
//int hash_delete(dic_t *dic, const char *key);

#endif
