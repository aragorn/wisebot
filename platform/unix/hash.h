/* $Id$ */
#ifndef	_HASH_H_
#define _HASH_H_

#include <softbot.h>
#include <inttypes.h>

#define HASH_SLOT_NUM		128 // origin 128
#define HASH_MAX_DEPTH		22	// susia increase this 18 -> 20	
#define HASH_MAX_BUCKET		(1 << HASH_MAX_DEPTH)	// 2^18 : 262xxx
#define HASH_ORGKEY_LEN		4			// 저장할 key 의 크기(byte)
#define HASH_HASHKEY_LEN	4			// 사용할 hashkey 의 크기
#define HASH_DATA_LEN		4			// 저장할 data의 크기

#define HASH_DELETELIST_SIZE	7

// 한번에 할당할 block의 크기
#define HASH_BUCKET_PER_BLOCK	10000 // 1000
#define HASH_MEM_BLOCK_SIZE		sizeof(bucket_t)*HASH_BUCKET_PER_BLOCK

// 최대로 할당할 block의 수
#define HASH_MEM_BLOCK_NUM	(HASH_MAX_BUCKET/HASH_BUCKET_PER_BLOCK) // XXX: why +1? 

//
#define HASH_COLLISION		-210
#define HASH_DELETED		211
#define HASH_OVERFLOW		-212

#define HASH_SORTED			1
#define HASH_UNSORTED		0
#define HASH_SORTING		(-1)

// slot
typedef struct {
	uint8_t		key[HASH_ORGKEY_LEN];	// key
	uint8_t		hashkey[HASH_HASHKEY_LEN];	// hash key
	uint8_t		data[HASH_DATA_LEN];	// data
	uint8_t		flag;					// delete flag, 기타 용도로도 사용가능.
} hslot_t;

// bucket
typedef struct {
	hslot_t		aSlot[HASH_SLOT_NUM];
	int16_t		nSlotCnt;
	int8_t		nDepth;
	int8_t		sorted;
} bucket_t;

// shared hash data
typedef struct {
	int 		magic; /* magic number */
	int8_t		attached; /* number of attachment */
	void		*data;   /* allocated memory offset for alloc_func */
	uint32_t	arr_of_mem_block_idx[HASH_MAX_BUCKET];
	int8_t		nDepth;
	int8_t		sorted;
	int32_t		nSlot;
	int32_t		nMem_block;
	uint32_t	nBucket;
	uint32_t	deletelist[HASH_DELETELIST_SIZE][2];
	uint8_t		nDeleted;
} hash_shareddata_t;

// hashtable
typedef struct hash_t hash_t;
struct hash_t {
	void				*parent;
	char				*path;
	hash_shareddata_t*	shared;
	void				(*hash_func)(hash_t *hash, void *key, uint8_t *hashkey);
	int					(*keycmp_func)(hash_t *hash, void *key1, void *key2);
	void				(*lock_func)(uint32_t bucket_num);
	void				(*unlock_func)(uint32_t bucket_num);
	int	(*load_data_func)(hash_t *hash, int data_idx, void *data, int data_size);
	int (*save_data_func)(hash_t *hash, int data_idx, void *data, int data_size);
	void*				(*alloc_data_func)(hash_t *hash, int data_idx, int size);
	void				(*free_func)(void*, int size);
	// XXX FIXME : obsolete until change structure to dynamic allocation 
	void*				mem_block[HASH_MEM_BLOCK_NUM];
};

SB_DECLARE(int) hash_open(hash_t *hash, int8_t init_depth, int8_t init_sortflag);
SB_DECLARE(int) hash_attach(hash_t *hash);
SB_DECLARE(int) hash_sync(hash_t *hash);
SB_DECLARE(int) hash_close(hash_t *hash);

SB_DECLARE(int) hash_add(hash_t *hash, void *key, uint8_t *data);
SB_DECLARE(int) hash_delete(hash_t *hash, void *key);
SB_DECLARE(int) hash_update(hash_t *hash, void *key, uint8_t *data);
SB_DECLARE(int) hash_search(hash_t *hash, void *key, uint8_t *data);
SB_DECLARE(void) print_bucket(hash_t *hash, int bucket_idx);
SB_DECLARE(void) buffer_bucket(hash_t *hash,int bucket_idx, char *result);
SB_DECLARE(void) print_hashstatus(hash_t *hash);

#endif // _HASH_H_
