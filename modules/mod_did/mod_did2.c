/* $Id$ */
#include <stdlib.h> /* atoi(3) */
#include <string.h> /* memset(3) */
#include <errno.h>
#include "common_core.h"
#include "ipc.h"
#include "memory.h"
#include "mod_api/did.h"
#include "mod_api/hash.h"

typedef struct {
    uint32_t last_did;
} did_db_shared_t;

typedef struct _did_db_custom_t {
	api_hash_t* hash;
    did_db_shared_t  *shared;
	int lock_id;
} did_db_custom_t;

typedef struct _did_db_set_t {
	int set;

	int set_hash_set;
	int hash_set;

	int set_shared_file;
	char shared_file[MAX_PATH_LEN];

	// init() 에서 초기화
	int set_lock_id;
	int lock_id;
} did_db_set_t;

static did_db_set_t* did_set = NULL;
static int current_did_set = -1;

#define ACQUIRE_LOCK() \
	if ( acquire_lock( db->lock_id ) != SUCCESS ) return FAIL;
#define RELEASE_LOCK() \
	if ( release_lock( db->lock_id ) != SUCCESS ) return FAIL;

static int alloc_shared_did_db(did_db_custom_t *did_db, char* shared_file, int *mmap_attr)
{
	ipc_t mmap;

    mmap.type        = IPC_TYPE_MMAP;
    mmap.pathname    = shared_file;
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
	did_db_custom_t *db = NULL;

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

	if ( !did_set[opt].set_hash_set ) {
		error("HashSet is not set [DidSet:%d]. see config", opt);
		return FAIL;
	}

	if ( !did_set[opt].set_shared_file ) {
		error("SharedFile is not set [DidSet:%d]. see config", opt);
		return FAIL;
	}

	if ( !did_set[opt].set_lock_id ) {
		error("invalid lock_id [DidSet:%d]. maybe failed from init()", opt);
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

	ret = alloc_shared_did_db( db, did_set[opt].shared_file, &mmap_attr );
	if ( ret != SUCCESS ) {
		error("alloc_shared_did_db failed");
		goto error;
	}

	ret = sb_run_hash_open( &db->hash, did_set[opt].hash_set );
	if (ret != SUCCESS) {
		error("hash[opt:%d] open failed", did_set[opt].hash_set);
		goto error;
	}

	db->lock_id = did_set[opt].lock_id;
	
	(*did_db)->set = opt;
	(*did_db)->db = (void*) db;
	return SUCCESS;

error:
	if ( db ) {
		if ( db->hash ) sb_run_hash_close( db->hash );
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
	int ret;
	did_db_custom_t* db;

	if ( did_set == NULL || !did_set[did_db->set].set )
		return DECLINE;
	db = (did_db_custom_t*) did_db->db;

	ret = sync_mmap( db->shared, sizeof(did_db_shared_t) );
	if ( ret != SUCCESS ) {
		error("write did_db->shared failed");
		return FAIL;
	}

	ret = sb_run_hash_sync( db->hash );
	if ( ret != SUCCESS ) {
		error("hash sync failed");
		return FAIL;
	}
	
	return SUCCESS;
}

static int close_did_db(did_db_t* did_db)
{
	did_db_custom_t* db;

	if ( did_set == NULL || !did_set[did_db->set].set )
		return DECLINE;
	db = (did_db_custom_t*) did_db->db;

	if ( sb_run_hash_close( db->hash ) != SUCCESS )
		error("hash close failed");

	if ( free_shared_did_db( db ) != SUCCESS )
		error("did shared free failed");

	sb_free( did_db->db );
	sb_free( did_db );

	return SUCCESS;
}

static int get_new_doc_id(did_db_t* did_db, char* pKey, uint32_t* docid, uint32_t* olddocid)
{
	did_db_custom_t* db;
	int ret;
	hash_data_t key, _docid, _olddocid;

	if ( did_set == NULL || !did_set[did_db->set].set )
		return DECLINE;
	db = (did_db_custom_t*) did_db->db;

	ACQUIRE_LOCK();
	*docid = ++(db->shared->last_did);
	RELEASE_LOCK();

	if ( *docid == 0 ) {
		db->shared->last_did--;
		return DOCID_OVERFLOW;
	}

	key.data = pKey;
	key.size = strlen(pKey)+1; // NULL도 같이 저장하자.
	key.data_len = key.size;

	_docid.data = docid;
	_docid.size = sizeof(uint32_t);
	_docid.data_len = sizeof(uint32_t);
	_docid.partial_op = 0;

	_olddocid.data = olddocid;
	_olddocid.data_len = sizeof(uint32_t);
	_olddocid.partial_op = 0;

	ret = sb_run_hash_put( db->hash, &key, &_docid, HASH_OVERWRITE, &_olddocid );
	if ( ret != SUCCESS ) {
		error("hash put failed. key[%s], docid[%u]", pKey, *docid);
		return FAIL;
	}

	if ( _olddocid.size > 0 ) return DOCID_OLD_REGISTERED;
	else return DOCID_NEW_REGISTERED;
}

static int get_doc_id(did_db_t* did_db, char* pKey, uint32_t* docid)
{
	did_db_custom_t* db;
	int ret;
	hash_data_t key, _docid;

	if ( did_set == NULL || !did_set[did_db->set].set )
		return DECLINE;
	db = (did_db_custom_t*) did_db->db;

	key.data = pKey;
	key.size = strlen(pKey)+1;
	key.data_len = key.size;
	
	_docid.data = docid;
	_docid.size = sizeof(uint32_t);
	_docid.data_len = sizeof(uint32_t);
	_docid.partial_op = 0;

	ret = sb_run_hash_get( db->hash, &key, &_docid );
	if ( ret == SUCCESS ) return DOCID_OLD_REGISTERED;
	else if ( ret == HASH_KEY_NOTEXISTS ) {
		*docid = 0;
		return DOCID_NOT_REGISTERED;
	}
	else return FAIL;
}

static uint32_t get_last_doc_id(did_db_t* did_db)
{
	did_db_custom_t* db;

	if ( did_set == NULL || !did_set[did_db->set].set )
		return DECLINE;
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
		if ( !did_set[i].set ) continue;

		if ( !did_set[i].set_shared_file ) {
			warn("SharedFile [DidSet:%d] is not set", i);
			continue;
		}

		lock.pathname = did_set[i].shared_file;

		ret = get_sem( &lock );
		if ( ret != SUCCESS ) return FAIL;

		did_set[i].lock_id = lock.id;
		did_set[i].set_lock_id = 1;
	}

	return SUCCESS;
}

/*****************************************************
 *                   config stuff
 *****************************************************/

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

static void get_hash_set(configValue v)
{
	if ( did_set == NULL || current_did_set < 0 ) {
		error("first, set DidSet");
		return;
	}

	did_set[current_did_set].hash_set = atoi( v.argument[0] );
	did_set[current_did_set].set_hash_set = 1;
}

static void get_shared_file(configValue v)
{
	if ( did_set == NULL || current_did_set < 0 ) {
		error("first, set DidSet");
		return;
	}

	strncpy( did_set[current_did_set].shared_file, v.argument[0], MAX_PATH_LEN-1 );
	did_set[current_did_set].set_shared_file = 1;
}

static config_t config[] = {
	CONFIG_GET("DidSet", get_did_set, 1, "DidSet {number}"),
	CONFIG_GET("HashSet", get_hash_set, 1, "did using HashSet {number}"),
	CONFIG_GET("SharedFile", get_shared_file, 1, "full filename of did_db_custom_t.shared"),
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

module did2_module=
{
    STANDARD_MODULE_STUFF,
    config,                 /* config */
    NULL,                   /* registry */
    init,                   /* initialize function */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
