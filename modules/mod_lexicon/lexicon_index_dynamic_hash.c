/* $Id$ */
#include "common_core.h"
#include <string.h> /* strlen(3),memcpy(3) */
#include <errno.h>
#include "ipc.h"
#include "hash.h"
#include "md5.h"
#include "memory.h"
#include "lexicon_index.h"
#include "mod_lexicon.h"
	
//#define HASH_SIZE   (HASH_MEM_BLOCK_SIZE * 100 + sizeof(hash_shareddata_t))
#define HASH_SIZE   (sizeof(hash_shareddata_t))

static void hash_func(hash_t *hash, void *key, uint8_t *hashkey);
static int hash_keycmp_func(hash_t *hash, void *key1, void *key2);
static void hash_lock_func(uint32_t n);
static void hash_unlock_func(uint32_t n);
static void *hash_alloc_data_func(hash_t *hash, int data_idx, int size);
static void hash_free_func(void *buf, int size);

/* ############################################################################
 * hash callback functions
 * ########################################################################## */

	//
	//ret = hash_add(word_db->hash, &(offset), (uint8_t*)(wordid));
	//
static void hash_func(hash_t *hash, void *key, uint8_t *hashkey) {
    int len;
    char *str;
    MD5_CTX context;
    unsigned char digest[16];
    lexicon_t *word_db = (lexicon_t *)(hash->parent);

    str = offset2ptr(word_db, (word_offset_t *)key, BLOCK_TYPE_VARIABLE);
    len = strlen(str);

	
    MD5Init( &context );
    MD5Update( &context, str, len );
    MD5Final( digest, &context );

    memcpy(hashkey, digest, HASH_HASHKEY_LEN);

/*  CRIT("hash key for [%s] is %x%x%x%x", 
       str, hashkey[0], hashkey[1], hashkey[2], hashkey[3]); */
    return;
}

int total_alloc_size;
int current_used_size;

static int hash_keycmp_func(hash_t *hash, void *key1, void *key2)
{
    int n;
    char *str1, *str2;
    lexicon_t *word_db = (lexicon_t *)(hash->parent);

    if (key1 == NULL && key2 == NULL) {
        warn("key1 == key2 == NULL");
        return 0;
    } else if (key1 == NULL || key2 == NULL) {
        DEBUG("key1 = %p, key2 = %p", key1, key2);
        return 1;
    }
    str1 = offset2ptr(word_db, (word_offset_t *)key1, BLOCK_TYPE_VARIABLE);
    str2 = offset2ptr(word_db, (word_offset_t *)key2, BLOCK_TYPE_VARIABLE);

    n = strcmp(str1, str2);
    //DEBUG("strcmp(str1[%s][%p], str2[%s][%p]) is %d", str1, str1, str2, str2, n);
    return n;
}


static void hash_lock_func(uint32_t n) { }
static void hash_unlock_func(uint32_t n) { }
static void *hash_alloc_data_func(hash_t *hash, int data_idx, int size) 
{
	ipc_t mmap;
	char hash_data_path[MAX_PATH_LEN];

	sprintf(hash_data_path, "%s.hash_data.%03d", hash->path, data_idx);
	debug("hash [%d]th data path is %s", data_idx, hash_data_path); 
	
    mmap.type        = IPC_TYPE_MMAP;
    mmap.pathname    = hash_data_path;
    mmap.size        = size;
	
	if (alloc_mmap(&mmap, 0) != SUCCESS) {
		crit("error while allocating mmap for [%d]th hash data", data_idx);
		return NULL;
	}

	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );

	DEBUG("allocated memory ptr[%p]", mmap.addr);		
    return mmap.addr;
}

// not need because use shared memeory  
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
	char hash_data_path[MAX_PATH_LEN];

	sprintf(hash_data_path, "%s.hash_data.%03d", hash->path, data_idx);

	if ( sync_mmap( data, data_size ) != SUCCESS ) {
		error("error while write[%s, %d]: %s",hash_data_path, data_idx, strerror(errno));
		return FAIL;
	}

	INFO("save to [%s, %d] %d bytes", hash_data_path, data_idx, data_size);
	return SUCCESS;
}

static int alloc_word_db_hash(hash_t *hash, int *mmap_attr)
{
    ipc_t mmap;
    char hash_path[MAX_PATH_LEN];
    lexicon_t *word_db = (lexicon_t *)hash->parent;

    /* allocate memory for hash index data  */
    sprintf(hash_path, "%s.hash", word_db->path);

    mmap.type        = IPC_TYPE_MMAP;
    mmap.pathname    = hash_path;
    mmap.size        = HASH_SIZE;

	total_alloc_size = mmap.size;
	current_used_size = 0;

	if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
        crit("error while allocating mmap for word_db");
        return FAIL;
    }

	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );

    hash->shared  = mmap.addr;
	*mmap_attr = mmap.attr;

    DEBUG("hash[%p]->shared [%p]", hash, (hash->shared));

    return SUCCESS;
}

static int free_word_db_hash(hash_t *hash)
{
	int ret;

	ret = free_mmap( hash->shared, HASH_SIZE );
	if ( ret == SUCCESS ) hash->shared = NULL;

	return ret;
}

/* ***********************************************************************
 * lexicon index api
 * ***********************************************************************/

int lexicon_index_open( lexicon_t *word_db )
{
    hash_t *hash;
    char hash_path[MAX_PATH_LEN];
	int mmap_attr;

	
    info("lexicon uses dynamic hash for indexing");
    
    sprintf(hash_path, "%s.hash", word_db->path);

    /* allocate memory for hash handler */
    hash = (hash_t *)sb_malloc(sizeof(hash_t));
    word_db->hash = hash;

    hash->parent      = word_db;
	hash->path 		  = word_db->path;
    hash->hash_func   = hash_func;
    hash->keycmp_func = hash_keycmp_func;
    hash->lock_func   = hash_lock_func;
    hash->unlock_func = hash_unlock_func;
	hash->load_data_func   = hash_load_data_func;
	hash->save_data_func   = hash_save_data_func;
    hash->alloc_data_func  = hash_alloc_data_func;
    hash->free_func   	   = hash_free_func;

    if ( alloc_word_db_hash(hash, &mmap_attr) != SUCCESS ) {
        error("cannot allocate memory for word_db hash index");
        return FAIL;
    }
	
	// check shared memory status 
	if (mmap_attr == MMAP_CREATED) {
		INFO("hash init!");
		hash_open(hash, 0, 1);
	} 
	else if (mmap_attr == MMAP_ATTACHED) {
		INFO("hash attached");
		hash_attach(hash);
	} 
	else {
		error("unknown mmap.atrr [%d]", mmap_attr);
		return FAIL;
	}

    DEBUG("hash->parent->shared [%p]", ((lexicon_t *)(hash->parent))->shared);
	return SUCCESS;
}

int lexicon_index_sync  ( lexicon_t *word_db )
{
    char hash_path[MAX_PATH_LEN];
    int ret;

    sprintf(hash_path, "%s.hash", word_db->path);
    DEBUG("hasy sync [%s]", hash_path);

	if ( sync_mmap( ((hash_t*)word_db->hash)->shared, HASH_SIZE ) != SUCCESS ) {
		error("sync failed");
        return FAIL;
    }

	ret = hash_sync((hash_t *)word_db->hash);
	if (ret != SUCCESS) return FAIL;

    DEBUG("sync hash success");
	return SUCCESS;
}

int lexicon_index_close ( lexicon_t *word_db )
{
	int ret;

	info("ivt lexicon db closing...");
	// lexicon close에서 이미 했을 것이다.
/*	ret = lexicon_index_sync(word_db);
	if (ret != SUCCESS) {
		error("lexicon index dynamic hash sync fail %d",ret);
	}*/

	ret = free_word_db_hash(word_db->hash);
	if (ret != SUCCESS) {
		error("lexicon index dynamic hash free fail %d",ret);
	}

	sb_free(word_db->hash);
	word_db->hash = NULL;

	info("ivt lexicon db closed");
	return ret;
}

int lexicon_index_put   ( lexicon_t *word_db, char* string, uint32_t* wordid, word_offset_t *offset)
{
	int len, ret;

	len = strlen(string);
	//DEBUG("lexicon index put word[%s]:id(%u),len(%d)",string, *wordid, len);
	
	ret = new_vb_offset(word_db, offset);
	if (ret != SUCCESS) return ret;
	
	ret = block_write(word_db, offset, BLOCK_TYPE_VARIABLE, string, len+1);
	if (ret != len+1) return ret;

	ret = hash_add(word_db->hash, offset, (uint8_t*)(wordid));
	//DEBUG("hash add ret %d",ret);
	
	switch (ret) {
	case SUCCESS:  /* if hash add success. increase variable block offset*/
		ret = increase_block_offset(word_db, len+1, BLOCK_TYPE_VARIABLE);
		if (ret != SUCCESS) return ret;
		return WORD_NEW_REGISTERED;	
	case HASH_COLLISION:
		return WORD_OLD_REGISTERED;
	default:
		error("hash add return [%d]",ret);
		return FAIL;
	}
}

int lexicon_index_get   ( lexicon_t *word_db, char* string, uint32_t* wordid)
{
	int len, ret;
	word_offset_t offset;

	len = strlen(string);

	ret = new_vb_offset(word_db, &(offset));
	if (ret != SUCCESS) return ret;

    ret = block_write(word_db, &(offset), BLOCK_TYPE_VARIABLE, string, len+1);
    if (ret != len+1) return ret;

	if (hash_search(word_db->hash, &(offset), (uint8_t*)(wordid)) == SUCCESS) {
		CRIT("search wordid %u", *wordid);
		return WORD_OLD_REGISTERED;
	}else{
		return WORD_NOT_REGISTERED;	
	}
}

int lexicon_index_del   ( lexicon_t *word_db, char* string)
{
	warn("lexicon index del need fill");
	return SUCCESS;
}
