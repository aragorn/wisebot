/* $Id$ */
#include "common_util.h"
/* moved to common_core.h due to precompiled header 
#include <stdio.h>
#include <string.h>
#include "log_error.h"
#include "hash.h"
*/

/// XXX ??? alloc 함수만 앞에 * 가 없이 불리운다 확인 필.

static uint32_t	getPrefix(uint8_t key[HASH_HASHKEY_LEN], int8_t depth);
static bucket_t *getBucket(hash_t *hash, uint32_t block_idx);
static int hashkeycmp(char *key1, char *key2, int len);

static int doubleTable(hash_t *hash);
static int partitionBucket(hash_t *hash, uint32_t bucket_idx);

static int32_t	findSlot(hash_t *hash, bucket_t *bucket, void *key, uint8_t *hashkey);
static int32_t	deleteSlot(hash_t *hash, uint32_t bucket_idx, uint32_t slotnum);

SB_DECLARE(int) hash_open(hash_t *hash, int8_t init_depth, int8_t init_sortflag)
{
	uint32_t ninit_bucket,i,j;
	bucket_t *bucket;
	void *tmpblock;
	int ret;

	info("HASH_MAX_BUCKET %d",HASH_MAX_BUCKET);
	info("HASH_BUCKET_PER_BLOCK %d", HASH_BUCKET_PER_BLOCK);
	info("HASH_MEM_BLOCK_NUM %d", HASH_MEM_BLOCK_NUM);
	info("HASH_MEM_BLOCK_SIZE %d", (int)HASH_MEM_BLOCK_SIZE);
	info("sizeof(bucket_t) [%d] sizeof(slot_t)[%d]", (int)sizeof(bucket_t), (int)sizeof(hslot_t));
	memset(hash->mem_block, 0, sizeof(void*)*HASH_MEM_BLOCK_NUM);

	
	if( init_depth > HASH_MAX_DEPTH ) {
		error("init_depth[%d] > HASH_MAX_DEPTH[%d]", init_depth, HASH_MAX_DEPTH);
		return FAIL;
	}

	if( hash->shared == NULL ) {
		error("hash->shared == NULL");
		return FAIL;
	}
	
	info("hash->shared->magic == [%d]",hash->shared->magic);
	
	ninit_bucket = 1 << init_depth;
	if(ninit_bucket > HASH_MAX_BUCKET){
		error("ninit_bucket[%d] > HASH_MAX_BUCKET[%d]", ninit_bucket, HASH_MAX_BUCKET);
		return FAIL;
	}
	
	if (hash->shared->magic == 0) {
		info(" data file not reading ");
		hash->shared->nDepth = init_depth; 
		hash->shared->sorted = init_sortflag; 
		hash->shared->nBucket = 0;
		hash->shared->nSlot = 0; 
		hash->shared->nDeleted = 0;
		hash->shared->nMem_block = ninit_bucket / HASH_BUCKET_PER_BLOCK +1;
	}
	
	info("hash->shared->nMem_block [%d]", hash->shared->nMem_block);
	info("hash->shared->nSlot [%d]", hash->shared->nSlot );

	// memory alloc and data loading
	for(i=0 ; i<hash->shared->nMem_block ; i++){
		// memory alloc
		tmpblock = (hash->alloc_data_func)(hash, i, HASH_MEM_BLOCK_SIZE);
		if (tmpblock == NULL) {
			for(j=0 ; j<i ; j++) {
				if (hash->mem_block[i])
					(hash->free_func)((void*)hash->mem_block[i], HASH_MEM_BLOCK_SIZE);
				hash->mem_block[i]=NULL;
			}
			return FAIL;
		}
		hash->mem_block[i] = tmpblock;
		info("hash->mem_block[%d] = [%p] [%p] ", i, tmpblock, hash->mem_block[i]);
		// data loading 
		if (hash->shared->magic == 1) {
			ret = (hash->load_data_func)(hash, i, hash->mem_block[i], HASH_MEM_BLOCK_SIZE);
			if (ret != SUCCESS) return FAIL;
		}
	}

	info("ninit_bucket %d", ninit_bucket);
	if (hash->shared->magic == 0) {
		for(i=0 ; i<ninit_bucket ; i++){
			hash->shared->arr_of_mem_block_idx[i] = i;
			bucket = getBucket(hash,i);
/*			info("bucket [%d] [%p] ", i ,bucket);*/
			if (bucket == NULL) {
				error("getBucket(%p,%d) returned NULL", hash, i);
				return FAIL;
			}
			hash->shared->nBucket++;
			bucket->nSlotCnt = 0;
			bucket->nDepth = init_depth;
			bucket->sorted = init_sortflag;
		}
	}

	hash->shared->magic = 1;
	return SUCCESS;
}

SB_DECLARE(int) hash_attach(hash_t *hash)
{
	int i, j;
	void *tmpblock;
		
	if( hash->shared == NULL ) {
		error("hash->shared == NULL");
		return FAIL;
	}
	
	info("hash->shared->nMem_block [%d]", hash->shared->nMem_block);
	memset(hash->mem_block, 0, sizeof(void*)*HASH_MEM_BLOCK_NUM);

	for(i=0 ; i<hash->shared->nMem_block ; i++){
		// memory alloc
		tmpblock = (hash->alloc_data_func)(hash, i, HASH_MEM_BLOCK_SIZE);
		if (tmpblock == NULL) {
			for(j=0 ; j<i ; j++) {
				if (hash->mem_block[i])
					(hash->free_func)((void*)hash->mem_block[i], HASH_MEM_BLOCK_SIZE);
				hash->mem_block[i] = NULL;
			}
			return FAIL;
		}
		hash->mem_block[i] = tmpblock;
	}

	info("hash attach success!");
	return SUCCESS;
}

SB_DECLARE(int) hash_sync(hash_t *hash)
{
	int i, ret;
	print_hashstatus(hash);

	info("hash[%s] save [%d] data file", hash->path, hash->shared->nMem_block); 
	
	// save and free memory data
	for ( i=0 ; i<hash->shared->nMem_block ; i++ ) {
		if ( hash->mem_block[i] == NULL )
			hash->mem_block[i] = ((hash->alloc_data_func))(hash, i, HASH_MEM_BLOCK_SIZE);

		ret = ( (hash->save_data_func )( hash, i, hash->mem_block[i], HASH_MEM_BLOCK_SIZE));
		if (ret != SUCCESS) return FAIL;
	}
	
	info("hash[%s] saved", hash->path);
	return SUCCESS;
}

SB_DECLARE(int) hash_close(hash_t *hash)
{
	int i, ret;
	print_hashstatus(hash);

	info("hash[%s] closing...", hash->path);

	ret = hash_sync(hash);
	if (ret != SUCCESS) return FAIL;

	info("hash close [%d] data file", hash->shared->nMem_block); 
	
	// save and free memory data
	for ( i=0 ; i<hash->shared->nMem_block ; i++ ) {
		if ( hash->mem_block[i] == NULL ) continue;

		(*(hash->free_func))((void*)hash->mem_block[i], HASH_MEM_BLOCK_SIZE);
		hash->mem_block[i] = NULL;
	}
	
	info("hash[%s] closed", hash->path);
	return SUCCESS;
}

SB_DECLARE(int) hash_add(hash_t *hash, void *key, uint8_t *data)
{
	int32_t i;
	int32_t bucket_idx;
	uint8_t hashkey[HASH_HASHKEY_LEN];
	bucket_t *bucket;
	hash_shareddata_t *shared = hash->shared;

	(*(hash->hash_func))(hash,key,hashkey);
	bucket_idx = getPrefix(hashkey,shared->nDepth);
	bucket = getBucket(hash,shared->arr_of_mem_block_idx[bucket_idx]);
	if (bucket == NULL) return FAIL;

	debug("bucket_idx [%d] mem_block_idx [%u]  bucket_cnt [%d]", 
			bucket_idx, shared->arr_of_mem_block_idx[bucket_idx] , bucket->nSlotCnt);
	
	(*(hash->lock_func))(bucket_idx);
	i = findSlot(hash,bucket,key,hashkey);

	if (i >= HASH_SLOT_NUM)
			CRIT("i>=HASH_SLOT_NUM %d: LINE:%d", i, __LINE__);
	if (i >= 0) {
		if(!memcmp(bucket->aSlot[i].data,data,HASH_DATA_LEN)){
			if(bucket->aSlot[i].flag == HASH_DELETED)
				bucket->aSlot[i].flag = 0;
			return SUCCESS;
		}
		memcpy(data,bucket->aSlot[i].data,HASH_DATA_LEN);
		return HASH_COLLISION;
	}
	
	if (bucket->nSlotCnt == HASH_SLOT_NUM) {
		if(bucket->nDepth == shared->nDepth) {
			if(doubleTable(hash) != SUCCESS) return HASH_OVERFLOW;
			if(partitionBucket(hash,bucket_idx) != SUCCESS) return HASH_OVERFLOW;
			return hash_add(hash,key,data);	
		} else if (bucket->nDepth < shared->nDepth) {
			if(partitionBucket(hash,bucket_idx) != SUCCESS) return HASH_OVERFLOW;
			return hash_add(hash,key,data);
		} else {
			error("depth error bucket->nDepth[%d] > shared->nDepth[%d]",
					bucket->nDepth, shared->nDepth);
			return FAIL;
		}
	} else if (bucket->nSlotCnt > HASH_SLOT_NUM) {
		crit("hash bucket overflow");
		return HASH_OVERFLOW;
	}

	debug("bucket->sorted [%d] bucket->nSlotCnt [%d]", bucket->sorted, bucket->nSlotCnt);
	if(bucket->sorted == HASH_SORTED)
	{
		int start = 0, middle, finish;
		if(bucket->nSlotCnt > 0) 
		{
			finish = bucket->nSlotCnt - 1;

			while(finish > start) 
			{
				middle = (start + finish) / 2;
				if (middle >= HASH_SLOT_NUM)
					CRIT("middle>=HASH_SLOT_NUM %d: LINE:%d", middle, __LINE__);
				if(hashkeycmp((bucket->aSlot[middle]).hashkey,hashkey,HASH_HASHKEY_LEN) > 0) {
					finish = middle;
				} 
				else {
					start = middle + 1;
				}

			}
			if(hashkeycmp((bucket->aSlot[start]).hashkey,hashkey,HASH_HASHKEY_LEN) < 0) {
					start++;
			}
		}
		
		i = start;
		
		if(i < bucket->nSlotCnt) {
			memmove(bucket->aSlot + (i+1), bucket->aSlot + i,
					sizeof(hslot_t) * (bucket->nSlotCnt - i));
		}
	}
	else if(bucket->sorted == HASH_UNSORTED){
		i = bucket->nSlotCnt;
	}
	else{
		return FAIL;
	}

	if (i >= HASH_SLOT_NUM)
			CRIT("i>=HASH_SLOT_NUM %d: LINE:%d", i, __LINE__);
	memcpy(bucket->aSlot[i].key,key,HASH_ORGKEY_LEN);
	memcpy(bucket->aSlot[i].hashkey,hashkey,HASH_HASHKEY_LEN);
	memcpy(bucket->aSlot[i].data,data,HASH_DATA_LEN);
	bucket->aSlot[i].flag = 0;
	bucket->nSlotCnt++;
	(*(hash->unlock_func))(bucket_idx);

/*	info("insert at [%d]" ,i);*/
	
/*	for (i=0 ; i< bucket->nSlotCnt; i++) { */
/*	info("bucket [%p] [%d] : key[%2x%2x%2x%2x] hashkey[%2x%2x%2x%2x] value[%d]",*/
/*			bucket, i,*/
/*			bucket->aSlot[i].key[0],bucket->aSlot[i].key[1],*/
/*			bucket->aSlot[i].key[2],bucket->aSlot[i].key[3],*/
/*		    bucket->aSlot[i].hashkey[0],bucket->aSlot[i].hashkey[1],*/
/*		    bucket->aSlot[i].hashkey[2],bucket->aSlot[i].hashkey[3],*/
/*		    *((uint8_t*)(&bucket->aSlot[i].data)));*/
/*	}*/

	shared->nSlot++;

	return SUCCESS;
}

SB_DECLARE(int) hash_delete(hash_t *hash, void *key)
{
	int32_t i;
	int32_t bucket_idx,slotnum;
	uint8_t hashkey[HASH_HASHKEY_LEN];
	bucket_t *bucket;
	hash_shareddata_t *shrdat;

	shrdat = hash->shared;
	(*(hash->hash_func))(hash,key,hashkey);
	bucket_idx = getPrefix(hashkey,shrdat->nDepth);
	bucket = getBucket(hash,shrdat->arr_of_mem_block_idx[bucket_idx]);

	(*(hash->lock_func))(bucket_idx);
	slotnum = findSlot(hash,bucket,key,hashkey);
	if(slotnum < 0) return FAIL;

	bucket->aSlot[slotnum].flag = HASH_DELETED;

	if(shrdat->nDeleted >= HASH_DELETELIST_SIZE){
		for(i=0 ; i<HASH_DELETELIST_SIZE ; i++)
			deleteSlot(hash,shrdat->deletelist[i][0],shrdat->deletelist[i][1]);
		shrdat->nDeleted = 0;
	}

	shrdat->deletelist[shrdat->nDeleted][0] = bucket_idx;
	shrdat->deletelist[shrdat->nDeleted][1] = slotnum;
	shrdat->nDeleted++;
	(*(hash->unlock_func))(bucket_idx);

	return SUCCESS;
}

SB_DECLARE(int) hash_update(hash_t *hash, void *key, uint8_t *data)
{
	int32_t bucket_idx,slotnum;
	uint8_t hashkey[HASH_HASHKEY_LEN];
	bucket_t *bucket;
	hash_shareddata_t *shrdat;

	shrdat = hash->shared;
	(*(hash->hash_func))(hash,key,hashkey);
	bucket_idx = getPrefix(hashkey,shrdat->nDepth);
	bucket = getBucket(hash,shrdat->arr_of_mem_block_idx[bucket_idx]);

	(*(hash->lock_func))(bucket_idx);
	slotnum = findSlot(hash,bucket,key,hashkey);
	if(slotnum < 0) return FAIL;

	memcpy(bucket->aSlot[slotnum].data,data,HASH_DATA_LEN);
	(*(hash->unlock_func))(bucket_idx);

	return SUCCESS;
}

SB_DECLARE(int) hash_search(hash_t *hash, void *key, uint8_t *data)
{
	int32_t bucket_idx,slotnum;
	uint8_t hashkey[HASH_HASHKEY_LEN];
	bucket_t *bucket;
	hash_shareddata_t *shrdat;

	shrdat = hash->shared;
	(*(hash->hash_func))(hash,key,hashkey);
	bucket_idx = getPrefix(hashkey,shrdat->nDepth);
	bucket = getBucket(hash,shrdat->arr_of_mem_block_idx[bucket_idx]);

	(*(hash->lock_func))(bucket_idx);
	slotnum = findSlot(hash,bucket,key,hashkey);
	if(slotnum < 0) return FAIL;
	(*(hash->unlock_func))(bucket_idx);

	memcpy(data,bucket->aSlot[slotnum].data,HASH_DATA_LEN);

	return SUCCESS;
}

static uint32_t	getPrefix(uint8_t key[HASH_HASHKEY_LEN], int8_t depth)
{
    uint32_t divisor, remainder;
	uint32_t prefix;
	uint32_t i;

	divisor = depth / 8;
	remainder = depth % 8;

	prefix = 0;

	for(i=0; i<divisor; i++) prefix = (prefix << 8) + key[i];

	if (remainder)
		prefix = (prefix << remainder) + (key[i] >> ( 8 - remainder ));

	return prefix;
}

static bucket_t *getBucket(hash_t *hash, uint32_t block_idx)
{
	int block, offset;

	block = block_idx / HASH_BUCKET_PER_BLOCK;
	if(block >= HASH_MEM_BLOCK_NUM) 
	{
		CRIT("block[%d] >= HASH_MEM_BLOCK_NUM[%d]", block, HASH_MEM_BLOCK_NUM);
		return NULL;
	}

	offset = block_idx % HASH_BUCKET_PER_BLOCK;
	debug("hash->shared[%p], offset[%d], mem_block[block %d] = %p, ",
			hash->shared, offset, block, hash->mem_block[block]);

	if (hash->mem_block[block] == NULL) {
		hash->mem_block[block] = ((hash->alloc_data_func))(hash, block, HASH_MEM_BLOCK_SIZE);
		if (hash->mem_block[block] == NULL) {
			crit(" block idx[%d] hash->mem_block[%d] == NULL)",block_idx, block);
			return NULL;
		}
/*		crit("getBucket [%d] is null memory block", block_idx);*/
/*		return NULL;*/
	}
	
	return hash->mem_block[block] + (offset * sizeof(bucket_t));
}

static int hashkeycmp(char *key1, char *key2, int len)
{
	return memcmp(key1, key2, len);
}

static int doubleTable(hash_t *hash)
{
	uint8_t depth;
	int32_t i;

	hash->shared->nDepth++;
	depth = hash->shared->nDepth;

	info("table depth (%d)", depth); // XXX: obsolete 

	if(depth > HASH_HASHKEY_LEN*8 || depth > HASH_MAX_DEPTH) {
		debug("depth (%d), HASH_HASHKEY_LEN*8 (%d), HASH_MAX_DEPTH (%d)", 
				depth, HASH_HASHKEY_LEN*8, HASH_MAX_DEPTH);
		return FAIL;
	}
		
	i = (1 << depth) - 1;
	for(; i>=0 ; i--)
		hash->shared->arr_of_mem_block_idx[i] = hash->shared->arr_of_mem_block_idx[i/2];

	return SUCCESS;
}

static int partitionBucket(hash_t *hash, uint32_t bucket_idx)
{
	int32_t i;
	int32_t backup;
	int8_t depth,cdepth;
	void *tmpblock;
	bucket_t *srcbucket, *newbucket;
	hash_shareddata_t *shrdat = hash->shared;

	
	if(shrdat->sorted != HASH_SORTED) {
		CRIT("hash not sorted");
		return FAIL;
	}

	srcbucket = getBucket(hash,shrdat->arr_of_mem_block_idx[bucket_idx]);
	cdepth = shrdat->nDepth - srcbucket->nDepth;

	if (cdepth <= 0) {
		CRIT("cdepth is %d shrdat->nDepth[%d] - srcbucket->nDepth[%d]",
				cdepth, shrdat->nDepth, srcbucket->nDepth);
		return FAIL;
	}
	
	if (!(shrdat->nBucket  % HASH_BUCKET_PER_BLOCK)) {
		tmpblock = ((hash->alloc_data_func))(hash, shrdat->nMem_block, HASH_MEM_BLOCK_SIZE);
		if(tmpblock == NULL) {
			crit("tmpblock is NULL");
			return FAIL;
		}
		if(shrdat->nMem_block >= HASH_MEM_BLOCK_NUM)
			CRIT("nMem_block is overflow : %d LINE :%d",shrdat->nMem_block, __LINE__);

		hash->mem_block[shrdat->nMem_block] = tmpblock;
		CRIT("mem_block[%d]_size : %p LINE :%d ", 
			  shrdat->nMem_block , hash->mem_block[shrdat->nMem_block], __LINE__);
		shrdat->nMem_block++;
	}

	newbucket = getBucket(hash,shrdat->nBucket);
	if (newbucket == NULL) {
		CRIT("newbucket null");
		return FAIL;
	}

	shrdat->nBucket++;
	
	newbucket->sorted = shrdat->sorted;
	depth = srcbucket->nDepth;

	for(i=0 ; i<srcbucket->nSlotCnt ; i++){
		if(getPrefix(srcbucket->aSlot[i].hashkey,depth+1) % 2) break;
	}

	memcpy(newbucket->aSlot,srcbucket->aSlot+i,
			sizeof(hslot_t)*(srcbucket->nSlotCnt-i));
	newbucket->nSlotCnt = srcbucket->nSlotCnt - i;
	srcbucket->nSlotCnt = i;
	srcbucket->nDepth++;
	newbucket->nDepth = srcbucket->nDepth;
	
	
	backup = shrdat->arr_of_mem_block_idx[bucket_idx];
	
	for (i=0 ; i<(1 << shrdat->nDepth) ; i++) {
		if(shrdat->arr_of_mem_block_idx[i] == backup){
			if((i >> (cdepth - 1)) % 2) {
				shrdat->arr_of_mem_block_idx[i] = shrdat->nBucket - 1;
			}
		}
	}
	
	
	return SUCCESS;
}

static int32_t findSlot(hash_t *hash, bucket_t *bucket, void *key, uint8_t hashkey[HASH_HASHKEY_LEN]){
	int startPos,finPos,midPos=100,fPos,bPos;
	int flag;

	if (bucket->nSlotCnt == 0 )
		return -1;

	if(bucket->sorted == HASH_SORTED)
	{			// if hashtable was sorted
		startPos = 0;
		finPos = bucket->nSlotCnt - 1;

		while(startPos <= finPos)
		{
			midPos = (startPos + finPos) / 2;

			flag = hashkeycmp(bucket->aSlot[midPos].hashkey,hashkey,HASH_HASHKEY_LEN);

			if(flag<0)
					startPos = midPos+1;
			else if(flag>0) 
					finPos = midPos;
			else
				break;

			if (startPos == midPos && midPos == finPos ) return -1;
		}
	}
	else if(bucket->sorted == HASH_UNSORTED){	// if hashtable was unsorted
		for(midPos=0 ; midPos<bucket->nSlotCnt ; midPos++)
			if(!hashkeycmp(bucket->aSlot[midPos].hashkey,hashkey,HASH_HASHKEY_LEN))
				return midPos;
		return -1;
	}
	else{										// if hashtable is in sorting
		return -2;
	}

	fPos = midPos;
	bPos = fPos - 1;
	do{
/*		info("bucket [%p] fPos is %d",bucket, fPos);*/
/*		info("aslot[fpos].key[%2x%2x%2x%2x]",*/
/*			  bucket->aSlot[fPos].key[0], bucket->aSlot[fPos].key[1],*/
/*			  bucket->aSlot[fPos].key[2], bucket->aSlot[fPos].key[3]);*/
		
		if( !(*(hash->keycmp_func))(hash,bucket->aSlot[fPos].key,key) )
			return fPos;
		fPos++;
		if(fPos >= bucket->nSlotCnt) break;
	}while( !memcmp(bucket->aSlot[fPos].hashkey,hashkey,HASH_HASHKEY_LEN) );

	while( bPos >= 0 &&
			!memcmp(bucket->aSlot[bPos].hashkey,hashkey,HASH_HASHKEY_LEN) ){
		if( !(*(hash->keycmp_func))(hash,(void*)bucket->aSlot[bPos].key,key) )
			return bPos;
		bPos--;
	}

	return -3;
}

static int32_t	deleteSlot(hash_t *hash, uint32_t bucket_idx, uint32_t slotnum){
	bucket_t *bucket;

	bucket = getBucket(hash,hash->shared->arr_of_mem_block_idx[bucket_idx]);
	if(slotnum >= bucket->nSlotCnt) return FAIL;
	memmove(bucket->aSlot + slotnum, bucket->aSlot + slotnum + 1,
			sizeof(hslot_t) * (bucket->nSlotCnt - slotnum - 1));
	bucket->nSlotCnt--;

	return SUCCESS;
}

// for debug & optimize
SB_DECLARE(void) print_hashstatus(hash_t *hash) {
	hash_shareddata_t *shrdat = hash->shared;

	debug("table structure size : %d bytes.",
			(int)sizeof(hash_t)+(int)sizeof(hash_shareddata_t));
	debug("slot size : %d bytes.",(int)sizeof(hslot_t));
	debug("bucket size : %d bytes.",(int)sizeof(bucket_t));
	debug("max bucket : %d",HASH_MAX_BUCKET);
	debug("# of slot prer bucket : %d",HASH_SLOT_NUM);
	debug("final hashtable depth : %d",shrdat->nDepth);

	debug("stored slot : %d",shrdat->nSlot);
	debug("actually allocated bucket : %d",shrdat->nBucket);
	debug("slot / bucket : %.1f%% (%.1f/%d)",
			(float)shrdat->nSlot/shrdat->nBucket/HASH_SLOT_NUM*100,
			(float)shrdat->nSlot/shrdat->nBucket,HASH_SLOT_NUM);
	debug("total bucket size : %d bytes (%.1fMB)",
			shrdat->nBucket * (int)sizeof(bucket_t),
			(float)shrdat->nBucket * sizeof(bucket_t)/1024/1024);
}

SB_DECLARE(void) print_bucket(hash_t *hash,int bucket_idx){
	bucket_t *bucket;
	int i;
	
	CRIT("print [%d] bucket", bucket_idx);
	
	bucket = getBucket(hash,hash->shared->arr_of_mem_block_idx[bucket_idx]);
	for(i=0;i<bucket->nSlotCnt;i++) {
		info("key[%2x%2x%2x%2x] hashkey[%2x%2x%2x%2x] value[%d] prefix[%u]\n",
				bucket->aSlot[i].key[0],bucket->aSlot[i].key[1],
				bucket->aSlot[i].key[2],bucket->aSlot[i].key[3],
				bucket->aSlot[i].hashkey[0],bucket->aSlot[i].hashkey[1],
				bucket->aSlot[i].hashkey[2],bucket->aSlot[i].hashkey[3],
				*((int*)(&bucket->aSlot[i].data[0])),
				getPrefix(bucket->aSlot[i].hashkey,bucket->nDepth+1)
				);
	}
}

SB_DECLARE(void) buffer_bucket(hash_t *hash,int bucket_idx, char *result){
	bucket_t *bucket;
	int i;
	char tmpbuf[LONG_STRING_SIZE];
	
	CRIT("print [%d] bucket", bucket_idx);
	
	bucket = getBucket(hash,hash->shared->arr_of_mem_block_idx[bucket_idx]);
	for(i=0;i<bucket->nSlotCnt;i++) {
		sprintf(tmpbuf, "key[%2x%2x%2x%2x] hashkey[%2x%2x%2x%2x] value[%d] prefix[%u]\n",
				bucket->aSlot[i].key[0],bucket->aSlot[i].key[1],
				bucket->aSlot[i].key[2],bucket->aSlot[i].key[3],
				bucket->aSlot[i].hashkey[0],bucket->aSlot[i].hashkey[1],
				bucket->aSlot[i].hashkey[2],bucket->aSlot[i].hashkey[3],
				*((int*)(&bucket->aSlot[i].data[0])),
				getPrefix(bucket->aSlot[i].hashkey,bucket->nDepth+1)
				);
		strcat(result, tmpbuf);
		printf("tmpbuf:%s\n", tmpbuf);
	}
}
