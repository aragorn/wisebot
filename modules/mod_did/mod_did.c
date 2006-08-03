/* $Id$ */
#include "common_core.h"
#include <stdlib.h> /* atoi(3) */
#include <string.h> /* memset(3) */
#include <errno.h>
#include "ipc.h"
#include "md5.h"
#include "memory.h"
#include "hash.h"
#include "mod_api/did.h"

#define BLOCK_DATA_SIZE       16000000
#define DID_MAX_BLOCK_NUM     256

typedef struct {
    uint8_t  block_idx  :8;  /* max 255 blocks */
    uint32_t position   :24; /* block size of 16MB */
}__attribute__((packed)) did_offset_t;

typedef struct {
    uint32_t used_bytes;
    char     data[BLOCK_DATA_SIZE];
} block_t;

typedef struct {
    uint32_t last_did;
    int      alloc_block_num;
} did_db_shared_t;

typedef struct _did_db_custom_t {
    char             path[MAX_PATH_LEN];
    void             *hash;
    block_t          *block[DID_MAX_BLOCK_NUM];
    did_db_shared_t  *shared;
	int              lock_id;
} did_db_custom_t;

typedef struct _did_db_set_t {
	int set;

	int set_did_file;
	char did_file[MAX_FILE_LEN];

	// init()에서 초기화
	int set_lock_id;
	int lock_id;
} did_db_set_t;

static did_db_set_t* did_set = NULL;
static int current_did_set = -1;

// open을 singleton으로 구현하기 위한 것
static did_db_t* singleton_did_db[MAX_DID_SET];
static int singleton_did_db_ref[MAX_DID_SET];

#define HASH_SIZE   (sizeof(hash_shareddata_t))

static int block_load(did_db_custom_t *did_db, int block_idx);
static int block_unload(did_db_custom_t *did_db, int block_idx);
static int block_sync(did_db_custom_t *did_db, int block_idx);
static int did_hash_open(did_db_custom_t *did_db);
static int did_hash_sync(did_db_custom_t *did_db);
static int did_hash_close(did_db_custom_t *did_db);

#define ACQUIRE_LOCK() \
	if ( acquire_lock( db->lock_id ) != SUCCESS ) return FAIL;
#define RELEASE_LOCK() \
	if ( release_lock( db->lock_id ) != SUCCESS ) return FAIL;

/****** DB 관련 함수 ******/
static int init_did_db(did_db_custom_t *did_db)
{
	int ret;
	
	did_db->shared->last_did=0;
	did_db->shared->alloc_block_num=0;

	ret = block_load(did_db, 0);
	if (ret != SUCCESS) return FAIL;
	did_db->shared->alloc_block_num++;

	return SUCCESS;
}

static int load_did_db(did_db_custom_t *did_db)
{
	int i, ret;
	
	info("read did_db: last_did[%u], alloc_block_num [%d]",
			did_db->shared->last_did, did_db->shared->alloc_block_num);
	
	for ( i=0 ; i< did_db->shared->alloc_block_num ; i++) {
		ret = block_load(did_db, i);
		if (ret != SUCCESS) return FAIL;
	}
	return SUCCESS;
}

static int alloc_shared_did_db(did_db_custom_t *did_db, int *mmap_attr)
{
	ipc_t mmap;

    mmap.type        = IPC_TYPE_MMAP;
    mmap.pathname    = did_db->path;
    mmap.size        = sizeof(did_db_shared_t);
	
	if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
		crit("error while allocating mmap for did_db");
		return FAIL;
	}

	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );
	
	did_db->shared = mmap.addr;
	*mmap_attr = mmap.attr;

	debug("did_db->shared [%p]",did_db->shared);
	
	return SUCCESS;
}

static int free_shared_did_db(did_db_custom_t *did_db)
{
	int ret;

	ret = free_mmap(did_db->shared, sizeof(did_db_shared_t));
	if ( ret == SUCCESS ) did_db->shared = NULL;

	return ret;
}

static int open_did_db(did_db_t** did_db, int opt)
{
	int ret, mmap_attr;
	did_db_custom_t* db = NULL;

	if ( did_set == NULL ) {
		warn("did_set is NULL. you must set DidSet in config file");
		return DECLINE;
	}

	if ( opt >= MAX_DID_SET || opt < 0 ) {
		error("opt[%d] is invalid. MAX_DID_SET[%d]", opt, MAX_DID_SET);
		return FAIL;
	}

	if ( !did_set[opt].set ) {
		warn("DidSet[opt:%d] is not defined", opt);
		return DECLINE;
	}

	// 다른 module에서 이미 열었던 건데.. 그것을 return한다.
	if ( singleton_did_db[opt] != NULL ) {
		*did_db = singleton_did_db[opt];
		singleton_did_db_ref[opt]++;

		info("reopened did db[set:%d, ref:%d]", opt, singleton_did_db_ref[opt]);
		return SUCCESS;
	}

	if ( !did_set[opt].set_did_file ) {
		error("DidFile is not set [DidSet:%d]. see config", opt);
		return FAIL;
	}

	if ( !did_set[opt].set_lock_id ) {
		error("invalid lock_id. maybe failed from init()");
		return FAIL;
	}

	*did_db = (did_db_t*) sb_calloc(1, sizeof(did_db_t));
	if ( *did_db == NULL ) {
		error("sb_calloc failed: %s", strerror(errno));
		goto error;
	}

	db = (did_db_custom_t*) sb_calloc(1, sizeof(did_db_custom_t));
	if ( db == NULL ) {
		error("sb_calloc failed: %s", strerror(errno));
		goto error;
	}

	strncpy(db->path, did_set[opt].did_file, MAX_PATH_LEN-1);
	db->lock_id = did_set[opt].lock_id;

	if (alloc_shared_did_db(db, &mmap_attr) != SUCCESS)
		goto error;
	
	if (mmap_attr == MMAP_CREATED) {
		ret = init_did_db(db);
		if (ret != SUCCESS) {
			error("db init failed");
			goto error;
		}
	} 
	else if (mmap_attr == MMAP_ATTACHED) {
		ret = load_did_db(db);
		if (ret != SUCCESS) {
			error("db load failed");
			goto error;
		}
	} 
	else {
		error("unknow mmap_attr [%d]", mmap_attr);
		goto error;
	}

	ret = did_hash_open(db);
	if (ret != SUCCESS) goto error;

	(*did_db)->set = opt;
	(*did_db)->db = (void*) db;

	singleton_did_db[opt] = *did_db;
	singleton_did_db_ref[opt] = 1;

	return SUCCESS;

error:
	if ( db ) {
		if ( db->hash ) did_hash_close( db );
		if ( db->shared ) free_shared_did_db( db );
		sb_free( db );
	}
	if ( *did_db ) {
		sb_free( *did_db );
		*did_db = NULL;
	}
	return FAIL;
}

static int sync_did_db(did_db_t* did_db)
{
	int i, ret;
	did_db_custom_t* db;

	if ( did_set == NULL || !did_set[did_db->set].set )
		return DECLINE;
	db = (did_db_custom_t*) did_db->db;

	info("did_db: last_did [%u] alloc_block[%d]",
			db->shared->last_did, db->shared->alloc_block_num);
	
	for (i=0; i< db->shared->alloc_block_num; i++) {
		ret = block_sync(db, i);
		if(ret != SUCCESS) return FAIL;
	}

	ret = sync_mmap( db->shared, sizeof(did_db_shared_t) );
	if ( ret != SUCCESS ) {
		error("write db->shared failed");
		return FAIL;
	}

	ret = did_hash_sync(db);
	if (ret!= SUCCESS) return FAIL;
	
	return SUCCESS;
}

static int close_did_db(did_db_t* did_db)
{
	int i, set;
	did_db_custom_t* db;

	if ( did_set == NULL || !did_set[did_db->set].set )
		return DECLINE;
	db = (did_db_custom_t*) did_db->db;
	set = did_db->set;

	// 아직 reference count가 남아있으면 close하지 말아야 한다.
	singleton_did_db_ref[set]--;
	if ( singleton_did_db_ref[set] ) {
		info("did db[set:%d, ref:%d] is not closing now",
				set, singleton_did_db_ref[set]);
		return SUCCESS;
	}

	if (sync_did_db(did_db) == FAIL) return FAIL;

	if (did_hash_close(db) == FAIL)
		error("did hash close failed");

	for (i=0; i< db->shared->alloc_block_num; i++) {
		block_unload(db, i);
	}

	if (free_shared_did_db(db) == FAIL)
		error("did shared free failed");

	sb_free(did_db->db);
	sb_free(did_db);

	singleton_did_db[set] = NULL;

	return SUCCESS;
}

/****** block 관련 함수 ******/
static int get_block_path(char *dest, char *base_path, int block_idx)
{
	return  sprintf(dest, "%s.%03d", base_path, block_idx);
}

//	ret = load_block(did_db, block_idx);
static int block_load(did_db_custom_t *did_db, int block_idx)
{
	char block_path[MAX_PATH_LEN];
	ipc_t mmap;
	block_t *block;
		
	if (block_idx >= DID_MAX_BLOCK_NUM) {
		CRIT("block idx overflow: %d", block_idx);
		return FAIL;
	}

	get_block_path(block_path, did_db->path, block_idx);
	
    mmap.type        = IPC_TYPE_MMAP;
    mmap.pathname    = block_path;
    mmap.size        = sizeof(block_t);

	if ( alloc_mmap( &mmap, 0 ) != SUCCESS ) {
		error("error while allocation mmap for block[%s]", block_path);
		return FAIL;
	}

	did_db->block[block_idx] = mmap.addr;	
	block = did_db->block[block_idx];

	if (mmap.attr == MMAP_CREATED) {
		block->used_bytes = 0;
		info("read %d bytes from [%s] used_bytes[%u]",
					mmap.size ,block_path, block->used_bytes); 
	} else if (mmap.attr == MMAP_ATTACHED) {
		info("shared attached block[%d] used_bytes [%d]", 
								block_idx, block->used_bytes);
	} else {
		error("unknow mmap.attr [%d]", mmap.attr);
		return FAIL;
	}
	
	return SUCCESS;	
}

static int block_unload(did_db_custom_t *did_db, int block_idx)
{
	int ret;
	char block_path[MAX_PATH_LEN];

	if ( did_db->block[block_idx] == NULL ) return SUCCESS;

	get_block_path(block_path, did_db->path, block_idx);

	ret = free_mmap(did_db->block[block_idx], sizeof(block_t));
	if ( ret == SUCCESS ) {
		did_db->block[block_idx] = NULL;
		info("block[%d][%s] unloaded", block_idx, block_path);
	}
	else {
		error("unload block[%d][%s] failed", block_idx, block_path);
	}

	return ret;
}

// ret = block_sync(did_db, i);
static int block_sync(did_db_custom_t *did_db, int block_idx)
{
	int ret;
	block_t *block;
	char block_path[MAX_PATH_LEN];
	
	get_block_path(block_path, did_db->path, block_idx);

	block = did_db->block[block_idx];
	if ( block == NULL ) {
		ret = block_load(did_db, block_idx);
		if ( ret != SUCCESS ) return FAIL;
		block = did_db->block[block_idx];
	}

	ret = sync_mmap(block, sizeof(block_t));
	if ( ret != SUCCESS ) {
		error("error while writing block[%s]: %s", block_path, strerror(errno));
		return FAIL;
	}

	info("write %d bytes to block[%s] used_bytes[%u]",
			(int)sizeof(block_t), block_path, block->used_bytes);
	
	return SUCCESS;	
}


// ret = block_assign_offset( did_db , offset, len+1 );
// XXX: need to be locked !!!
static int block_assign_offset( did_db_custom_t *did_db , did_offset_t* offset, int len )
{
	int ret;
	int block_idx;
	uint32_t used_bytes;

	block_idx = did_db->shared->alloc_block_num-1;
	debug("block_idx is %d",block_idx);
	
	if (did_db->block[block_idx] == NULL) {
		ret = block_load(did_db, block_idx);
		if (ret != SUCCESS) return FAIL;
	}
	
	used_bytes = did_db->block[block_idx]->used_bytes;
	debug("used_bytes is %d", used_bytes);
	
	if (used_bytes + len >= BLOCK_DATA_SIZE-1) {
		block_idx++;

		ret = block_load(did_db, block_idx);
		if (ret != SUCCESS) return FAIL;

		used_bytes = 0;
		did_db->shared->alloc_block_num++;
	}

	offset->block_idx 	= block_idx; 
	offset->position    = used_bytes;

	debug("block_idx[%d], position[%d]",block_idx, used_bytes);
	
	return SUCCESS;		
}

//ret = block_write_data( did_db, offset, pKey, len+1);
static int block_write_data( did_db_custom_t *did_db, did_offset_t offset, const void *data, int size )
{
	block_t *block;

	debug(" data [%s] size[%d] block_idx[%d],position[%d]",
			(char *)data, size, offset.block_idx, offset.position);
	block = did_db->block[offset.block_idx];

	debug("block [%p]",block);

	
	if (BLOCK_DATA_SIZE < offset.position + size) {
		error("memory encroachment BLOCK_DATA_SIZE[%d] < position[%d]+size[%d]", 
			   							BLOCK_DATA_SIZE, offset.position, size);
		return FAIL;
	}
	
	debug("alloc num is %d",did_db->shared->alloc_block_num);
	debug("block->data [%p] ",block->data);
	
	memcpy(((char *)block->data)+offset.position, data, size); 
	debug("alloc num is %d",did_db->shared->alloc_block_num);
 	return SUCCESS;
}

//ret = block_offset_increase (did_db, len+1);
static int block_offset_increase (did_db_custom_t *did_db, did_offset_t offset, int size)
{
	block_t *block;

	block = did_db->block[offset.block_idx];
	if (block == NULL) return FAIL;

	block->used_bytes += size;
	if (BLOCK_DATA_SIZE < block->used_bytes) {
		error("block->used_bytes[%d] overflow", block->used_bytes);	
		return FAIL;
	}

	debug("offset.block_idx [%d]",offset.block_idx);
	
	return SUCCESS;
}

/****** hash 관련 함수 ******/
static void *offset2ptr(did_db_custom_t *did_db, did_offset_t *offset)
{	
	int ret;
	block_t *block;

	if (offset->block_idx >= did_db->shared->alloc_block_num) {
		error("offset->block_idx[%d] >= did_db->shared->alloc_block_num[%d]",
				offset->block_idx, did_db->shared->alloc_block_num);
		return NULL;
	}

	block = did_db->block[offset->block_idx];

	if (block == NULL) {
		ret = block_load(did_db, offset->block_idx);
		if (ret!=SUCCESS) return NULL;

		block = did_db->block[offset->block_idx];
	}
	
	SB_DEBUG_ASSERT(block!=NULL);
	return (void *)( ((char *)(block->data)) + offset->position );
}

static void hash_func(hash_t *hash, void *key, uint8_t *hashkey) {
    int len;
    char *str;
    MD5_CTX context;
    unsigned char digest[16];
    did_db_custom_t *did_db = (did_db_custom_t *)(hash->parent);

    str = offset2ptr(did_db, (did_offset_t *)key);
    len = strlen(str);

    MD5Init( &context );
    MD5Update( &context, str, len );
    MD5Final( digest, &context );

    memcpy(hashkey, digest, HASH_HASHKEY_LEN);

/*  CRIT("hash key for [%s] is %x%x%x%x",
       str, hashkey[0], hashkey[1], hashkey[2], hashkey[3]); */
    return;
}

static int hash_keycmp_func(hash_t *hash, void *key1, void *key2)
{
    int n;
    char *str1, *str2;
    did_db_custom_t *did_db = (did_db_custom_t *)(hash->parent);

    if (key1 == NULL && key2 == NULL) {
        warn("key1 == key2 == NULL");
        return 0;
    } else if (key1 == NULL || key2 == NULL) {
        debug("key1 = %p, key2 = %p", key1, key2);
        return 1;
    }
    str1 = (char *)offset2ptr(did_db, (did_offset_t *)key1);
	//INFO("line:%d - str1: %p", __LINE__, str1);
    str2 = (char *)offset2ptr(did_db, (did_offset_t *)key2);
	//INFO("line:%d - str1: %p, str2: %p", __LINE__, str1, str2);

    n = strcmp(str1, str2);
	debug(" key1->block_idx: %u, key1->position: %u, key2->block_idx: %u, key2->position: %u",
			((did_offset_t *)key1)->block_idx, ((did_offset_t *)key1)->position, 
			((did_offset_t *)key2)->block_idx, ((did_offset_t *)key2)->position);
    debug("strcmp(str1[%s][%p], str2[%s][%p]) is %d", str1, str1, str2, str2, n);
    return n;
}

static void hash_lock_func(uint32_t n) { }
static void hash_unlock_func(uint32_t n) { }
static void *hash_alloc_data_func(hash_t *hash, int data_idx, int size)
{
    ipc_t mmap;
    char hash_data_path[MAX_PATH_LEN];

    sprintf(hash_data_path, "%s.hash_data.%03d", hash->path, data_idx);

    mmap.type        = IPC_TYPE_MMAP;
    mmap.pathname    = hash_data_path;
    mmap.size        = size;

	if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
		crit("error while allocating mmap for %s", hash_data_path);
		return NULL;
	}

	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );

    debug("allocated memory ptr[%p]", mmap.addr);
    return mmap.addr;
}

static void hash_free_func(void *buf, int size)
{
	free_mmap(buf, size);
}


static int hash_load_data_func(hash_t *hash, int data_idx, void *data, int data_size)
{
	// mmap을 썼으니 할 일이 없다.
    return SUCCESS;
}


static int hash_save_data_func(hash_t *hash, int data_idx, void *data, int data_size)
{
    int ret;
    char hash_data_path[MAX_PATH_LEN];

    sprintf(hash_data_path, "%s.hash_data.%03d", hash->path, data_idx);

	ret = sync_mmap(data, data_size);
	if ( ret != SUCCESS ) {
		error("error while write %s: %s", hash_data_path, strerror(errno));
		return FAIL;
	}

    info("save to [%s] %d bytes", hash_data_path, data_size);
    return SUCCESS;
}

static int alloc_did_db_hash(hash_t *hash, int *mmap_attr)
{
    ipc_t mmap;
    char hash_path[MAX_PATH_LEN];
    did_db_custom_t *did_db = (did_db_custom_t *)hash->parent;

    /* allocate memory for hash index data  */
    sprintf(hash_path, "%s.hash", did_db->path);

    mmap.type        = IPC_TYPE_MMAP;
    mmap.pathname    = hash_path;
    mmap.size        = HASH_SIZE;

	if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
		crit("error while allocating mmap for did_db hash");
		return FAIL;
	}

	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );

    hash->shared  = mmap.addr;
    *mmap_attr = mmap.attr;

    debug("hash[%p]->shared [%p]", hash, (hash->shared));
    return SUCCESS;
}

static int free_did_db_hash(hash_t *hash)
{
	int ret;

	ret = free_mmap(hash->shared, HASH_SIZE);
	if (ret == SUCCESS) hash->shared = NULL;

	return ret;
}

static int did_hash_open(did_db_custom_t *did_db)
{
    hash_t *hash;
    char hash_path[MAX_PATH_LEN];
    int mmap_attr;

    CRIT("did use dynamic hash");

    sprintf(hash_path, "%s.hash", did_db->path);

    /* allocate memory for hash handler */
    hash = (hash_t *)sb_malloc(sizeof(hash_t));
    did_db->hash = hash;

    hash->parent      = did_db;
    hash->path        = did_db->path;
    hash->hash_func   = hash_func;
    hash->keycmp_func = hash_keycmp_func;
    hash->lock_func   = hash_lock_func;
    hash->unlock_func = hash_unlock_func;
    hash->load_data_func   = hash_load_data_func;
    hash->save_data_func   = hash_save_data_func;
    hash->alloc_data_func  = hash_alloc_data_func;
    hash->free_func        = hash_free_func;

    if ( alloc_did_db_hash(hash, &mmap_attr) != SUCCESS ) {
        error("cannot allocate memory for did_db hash");
        return FAIL;
    }

    // check shared memory status
    if (mmap_attr == MMAP_CREATED) {
        info("hash init!");
        hash_open(hash, 0, 1);
    }
    else if (mmap_attr == MMAP_ATTACHED) {
        info("hash attached");
        hash_attach(hash);
    }
    else {
        error("unknown mmap.attr [%d]", mmap_attr);
        return FAIL;
    }

    debug("hash->parent->shared [%p]", ((did_db_custom_t *)(hash->parent))->shared);
    return SUCCESS;
}
	
static int did_hash_sync(did_db_custom_t *did_db)
{
    char hash_path[MAX_PATH_LEN];
    int ret;

    sprintf(hash_path, "%s.hash", did_db->path);
    info("hash[%s] saving...", hash_path);

	ret = sync_mmap( ((hash_t*)did_db->hash)->shared, HASH_SIZE );
	if ( ret != SUCCESS ) {
		error("error while writing did_db->hash");
		return FAIL;
	}

    ret = hash_sync((hash_t *)did_db->hash);
    if (ret != SUCCESS) return FAIL;

    debug("sync hash success");
    return SUCCESS;
}

static int did_hash_close(did_db_custom_t *did_db)
{
	char hash_path[MAX_PATH_LEN];

	info("did hash closing...");

	sprintf(hash_path, "%s.hash", did_db->path);
	debug("hash close[%s]", hash_path);

	hash_close((hash_t *)did_db->hash);

	free_did_db_hash(did_db->hash);
	sb_free(did_db->hash);
	did_db->hash = NULL;

	info("did hash closed");
	return SUCCESS;
}

static int did_hash_put( did_db_custom_t *did_db, char *pKey, uint32_t *docid, uint32_t *olddocid)
{
	int len, ret;
	did_offset_t offset;
	uint32_t tempid;

	tempid = *docid;

	len = strlen(pKey);
	if (len >MAX_DOCID_KEY_LEN) {
		len = MAX_DOCID_KEY_LEN;
	}

	pKey[len] = '\0';
	
	debug("docid [%u], pKey [%s] len [%d]",*docid, pKey, len);
	
	if (tempid == 0) return DOCID_OVERFLOW;
	
	// len +1: +1 is null char
	ret = block_assign_offset( did_db , &offset, len+1 );
	if (ret != SUCCESS) return ret;
	

	ret = block_write_data( did_db, offset, pKey, len+1);
	if (ret != SUCCESS) return ret;

			
	debug("offset.block_idx[%d], offset.position[%d]",
			offset.block_idx, offset.position);
	
	ret = hash_add( did_db->hash, &offset, (uint8_t*)(docid));

	debug("hash_add return [%d]",ret);
	*olddocid = *docid;
	
	switch (ret) 
	{
		case SUCCESS:
			ret = block_offset_increase (did_db, offset, len+1);
			if (ret != SUCCESS) return FAIL;
			return DOCID_NEW_REGISTERED;
		case HASH_COLLISION:
			ret = hash_update( did_db->hash, &offset, (uint8_t*)&(tempid));
			*docid = tempid;
			if (ret != SUCCESS) return FAIL;
			return DOCID_OLD_REGISTERED;
		default:
			error("hash add return[%d]", ret);
			return FAIL;
	}
}

static int did_hash_find(did_db_custom_t *did_db, char* pKey, uint32_t* docid)
{
	int ret, len;
	did_offset_t offset;

	len = strlen(pKey);
	if (len >MAX_DOCID_KEY_LEN) {
		len = MAX_DOCID_KEY_LEN;
	}
	pKey[len] = '\0';
	
	// len +1: +1 is null char
	ret = block_assign_offset( did_db , &offset, len+1 );
	if (ret != SUCCESS) return ret;
	
	ret = block_write_data( did_db, offset, pKey, len+1 );
	if (ret != SUCCESS) return ret;
	
	ret = hash_search(did_db->hash, &(offset), (uint8_t*)(docid));
	switch (ret)
	{
		case SUCCESS:
			return DOCID_OLD_REGISTERED;
		case FAIL:
			return DOCID_NOT_REGISTERED;	
		default:
			error("unknow ret [%d]", ret);
			return FAIL;
	}
}

/****** API function ******/ 
static int get_new_doc_id(did_db_t* did_db, char *pKey, uint32_t* docid, uint32_t* olddocid)
{
	int ret;
	did_db_custom_t* db;

	if ( did_set == NULL || !did_set[did_db->set].set )
		return DECLINE;
	db = (did_db_custom_t*) did_db->db;

	ACQUIRE_LOCK();
	*docid = db->shared->last_did+1;
	
	ret = did_hash_put(db, pKey, docid, olddocid);
	switch (ret) {
		case DOCID_NEW_REGISTERED:
			db->shared->last_did++;
			break;

		case DOCID_OLD_REGISTERED:
			db->shared->last_did++;
			break;

		default:
			break;;
	}
	RELEASE_LOCK();

	return ret;
}

static int get_doc_id(did_db_t* did_db, char *pKey, uint32_t *docid)
{
	int ret;
	did_db_custom_t* db;

	if ( did_db == NULL ) return FAIL;
	if ( did_set == NULL || !did_set[did_db->set].set )
		return DECLINE;
	db = (did_db_custom_t*) did_db->db;

	ACQUIRE_LOCK();
	ret = did_hash_find(db, pKey, docid);
	RELEASE_LOCK();

	if (ret == DOCID_OLD_REGISTERED) {
		return DOCID_OLD_REGISTERED;
	} else if (ret == DOCID_NOT_REGISTERED) {
		*docid = 0;
		return DOCID_NOT_REGISTERED;
	} else {
		return FAIL;
	}
}

static uint32_t get_last_doc_id(did_db_t* did_db)
{
	did_db_custom_t* db;

	if ( did_set == NULL || !did_set[did_db->set].set )
		return (uint32_t)-1;
	db = (did_db_custom_t*) did_db->db;

	// no lock?
	return db->shared->last_did;
}

/******  module stuff *******/
static int init() 
{
	ipc_t lock;
	int i, ret;

	if ( did_set == NULL ) return SUCCESS;

	lock.type = IPC_TYPE_SEM;
	lock.pid  = SYS5_DOCID;

	for ( i = 0; i < MAX_DID_SET; i++ ) {
		singleton_did_db[i] = NULL;

		if ( !did_set[i].set ) continue;

		if ( !did_set[i].set_did_file ) {
			warn("SharedFile [DidSet:%d] is not set", i);
			continue;
		}

		lock.pathname = did_set[i].did_file;

		ret = get_sem( &lock );
		if ( ret != SUCCESS ) return FAIL;

		did_set[i].lock_id = lock.id;
		did_set[i].set_lock_id = 1;
	}

	return SUCCESS;
}

static void get_did_set(configValue v)
{
	static did_db_set_t local_did_set[MAX_DID_SET];
	int value = atoi( v.argument[0] );

	if ( value < 0 || value >= MAX_DID_SET ) {
		error("Invalid DidSet value[%s], MAX_DID_SET[%d]",
				v.argument[0], MAX_DID_SET);
		return;
	}

	if ( did_set == NULL ) {
		memset( local_did_set, 0, sizeof(local_did_set) );
		did_set = local_did_set;
	}

	current_did_set = value;
	did_set[value].set = 1;
}

static void get_did_file(configValue v)
{
	if ( did_set == NULL || current_did_set < 0 ) {
		error("first, set DidSet");
		return;
	}


	strncpy( did_set[current_did_set].did_file, v.argument[0], MAX_FILE_LEN-1);
	did_set[current_did_set].set_did_file = 1;
}

static config_t config[] = {
	CONFIG_GET("DidSet", get_did_set, 1, "DidSet {number}"),
	CONFIG_GET("DidFile", get_did_file, 1, "ex> DidFile dat/did/did.db"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_open_did_db		(open_did_db,		NULL, NULL, HOOK_MIDDLE);
	sb_hook_sync_did_db		(sync_did_db,		NULL, NULL, HOOK_MIDDLE);
	sb_hook_close_did_db	(close_did_db,		NULL, NULL, HOOK_MIDDLE);
	
	sb_hook_get_new_docid	(get_new_doc_id, 	NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_docid		(get_doc_id, 		NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_last_docid	(get_last_doc_id, 	NULL, NULL, HOOK_MIDDLE);
}

module did_module=
{
    STANDARD_MODULE_STUFF,
    config,                 /* config */
    NULL,                   /* registry */
    init,                   /* initialize function */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
