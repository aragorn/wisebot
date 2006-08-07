#ifndef _DICT_CACHE_H
#define _DICT_CACHE_H 1

// XXX: temporary
#define STRING_KEY_SIZE			128

typedef struct _dict_cache_t dict_cache_t;

typedef struct _dict_cache_slot_t {
	char key[STRING_KEY_SIZE];
	void *data;
	int size;
	int occupied;
	int score;
} dict_cache_slot_t;

dict_cache_t *dict_cache_create(int size, int nblock, int nslot);
void dict_cache_destroy(dict_cache_t *this);
dict_cache_slot_t *dict_cache_lookup(dict_cache_t *this, char *key, int keylen);
int dict_cache_advice(dict_cache_t *this, char *key, int keylen, void *data, int datasize);

#endif
