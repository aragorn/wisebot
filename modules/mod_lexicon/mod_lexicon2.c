/* 
 * $Id$
 * 
 * mod_lexicon.c
 */

#include <stdlib.h> /* atoi(3) */
#include <string.h> /* strerror(3) */
#include <errno.h>
#include "common_core.h"
#include "ipc.h"
#include "memory.h"
#include "mod_api/lexicon.h"
#include "mod_api/hash.h"

typedef struct {
	uint32_t last_wordid;
} lexicon_shared_t;

typedef struct {
	lexicon_shared_t *shared;
	api_hash_t* wordtoid_hash;
	api_hash_t* idtoword_hash; // 없어도 된다...
} lexicon_t;

typedef struct _word_db_set_t {
	int set;

	int set_wordtoid_hash_set;
	int wordtoid_hash_set;

	int set_idtoword_hash_set;
	int idtoword_hash_set;

	int set_shared_file;
	char shared_file[MAX_PATH_LEN];
} word_db_set_t;

static word_db_set_t* word_db_set = NULL;
static int current_word_db_set = -1;

// open을 singleton으로 구현하기 위한 것
static word_db_t* singleton_word_db[MAX_WORD_DB_SET];
static int singleton_word_db_ref[MAX_WORD_DB_SET];

static int init()
{
	int i;

	for ( i = 0; i < MAX_WORD_DB_SET; i++ ) {
		singleton_word_db[i] = NULL;
	}

	return SUCCESS;
}

/* ############################################################################
 * lexicon database functions (alloc)
 * ########################################################################## */

static int alloc_shared_word_db(lexicon_t *lexicon, char* shared_file, int *mmap_attr)
{
	ipc_t mmap;

	mmap.type		= IPC_TYPE_MMAP;
	mmap.pathname	= shared_file;
	mmap.size		= sizeof(lexicon_shared_t);

	if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
		crit("error while allocating mmap for word_db [%s]", shared_file);
		return FAIL;
	}

	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );

	lexicon->shared  = mmap.addr;
	*mmap_attr = mmap.attr;
	
	return SUCCESS;
}

static int free_shared_word_db(lexicon_t *lexicon)
{
	int ret;

	ret = free_mmap(lexicon->shared, sizeof(lexicon_shared_t));
	if ( ret == SUCCESS ) lexicon->shared = NULL;

	return ret;
}

/* ############################################################################
 * api functions
 * ########################################################################## */

static int open_word_db(word_db_t** word_db, int opt)
{
	int ret, mmap_attr;
	lexicon_t *db = NULL;

	if ( word_db_set == NULL ) {
		warn("word_db_set is NULL. you must set WordDbSet in config file");
		return DECLINE;
	}

	if ( opt >= MAX_WORD_DB_SET || opt < 0 ) {
		error("opt[%d] is invalid. MAX_WORD_DB_SET[%d]", opt, MAX_WORD_DB_SET);
		return FAIL;
	}

	if ( !word_db_set[opt].set ) {
		warn("WordDbSet[opt:%d] is not defined", opt);
		return DECLINE;
	}

	// 다른 module에서 이미 열었던 건데.. 그것을 return한다.
	if ( singleton_word_db[opt] != NULL ) {
		*word_db = singleton_word_db[opt];
		singleton_word_db_ref[opt]++;

		info("reopened word db[set:%d, ref:%d]", opt, singleton_word_db_ref[opt]);
		return SUCCESS;
	}

	if ( !word_db_set[opt].set_wordtoid_hash_set ) {
		error("WordToIdHashSet is not set [WordDbSet:%d]. see config", opt);
		return FAIL;
	}
	
	if ( !word_db_set[opt].set_shared_file ) {
		error("SharedFile is not set [WordDbSet:%d]. see config", opt);
		return FAIL;
	}

	*word_db = (word_db_t*) sb_calloc(1, sizeof(word_db_t));
	if ( *word_db == NULL ) {
		error("sb_calloc failed: %s", strerror(errno));
		goto error;
	}
	
	db = (lexicon_t*) sb_calloc(1, sizeof(lexicon_t));
	if ( db == NULL ) {
		error("sb_calloc failed: %s", strerror(errno));
		goto error;
	}
	
	ret = alloc_shared_word_db(db, word_db_set[opt].shared_file, &mmap_attr);
	if ( ret != SUCCESS ) {
		error("alloc_shared_word_db failed");
		return FAIL;
	}

	ret = sb_run_hash_open( &db->wordtoid_hash, word_db_set[opt].wordtoid_hash_set );
	if ( ret != SUCCESS ) {
		error("word->id hash[opt:%d] open failed", word_db_set[opt].wordtoid_hash_set);
		goto error;
	}

	if ( word_db_set[opt].set_idtoword_hash_set ) {
		ret = sb_run_hash_open( &db->idtoword_hash, word_db_set[opt].idtoword_hash_set );
		if ( ret != SUCCESS ) {
			error("id->word hash[opt:%d] open failed", word_db_set[opt].idtoword_hash_set);
			goto error;
		}
	}

	(*word_db)->set = opt;
	(*word_db)->db = (void*) db;

	singleton_word_db[opt] = *word_db;
	singleton_word_db_ref[opt] = 1;

	return SUCCESS;

error:
	if ( db ) {
		if ( db->idtoword_hash ) sb_run_hash_close( db->idtoword_hash );
		if ( db->wordtoid_hash ) sb_run_hash_close( db->wordtoid_hash );
		if ( db->shared ) free_shared_word_db( db );
		sb_free( db );
	}
	if ( *word_db ) {
		sb_free( *word_db );
		*word_db = NULL;
	}
	return FAIL;
}


static int sync_word_db(word_db_t* word_db)
{
	lexicon_t* db;

	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;

	info("saving word db[set:%d]...", word_db->set);

	if ( sync_mmap( db->shared, sizeof(lexicon_shared_t) ) != SUCCESS ) {
		error("write word_db->shared failed");
		return FAIL;
	}

	if ( sb_run_hash_sync( db->wordtoid_hash ) != SUCCESS ) {
		error("word->id hash sync failed");
		return FAIL;
	}
	
	if ( !db->idtoword_hash && sb_run_hash_sync( db->idtoword_hash ) != SUCCESS ) {
		error("id->word hash sync failed");
		return FAIL;
	}
	
	info("word db[set:%d] saved", word_db->set);
	return SUCCESS;
}

static int close_word_db(word_db_t* word_db)
{	
	lexicon_t* db;
	int set;

	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;
	set = word_db->set;

	info("word db[set:%d] closing...", set);

	// 아직 reference count가 남아있으면 close하지 말아야 한다.
	singleton_word_db_ref[set]--;
	if ( singleton_word_db_ref[set] ) {
		info("word db[set:%d, ref:%d] is not closing now",
				set, singleton_word_db_ref[set]);
		return SUCCESS;
	}

	if ( sb_run_hash_close( db->wordtoid_hash ) != SUCCESS )
		error("word->id hash close failed");

	if ( db->idtoword_hash && sb_run_hash_close( db->idtoword_hash ) != SUCCESS )
		error("id->word hash close failed");

	if ( free_shared_word_db( db ) != SUCCESS )
		error("word db shared free failed");

	info("word db[set:%d] closed", word_db->set);

	sb_free( word_db->db );
	sb_free( word_db );

	singleton_word_db[set] = NULL;

	return SUCCESS;
}

/***********************************************************
 * word db에 단어 추가하는 녀석은 indexer 혼자 뿐이므로
 * write lock 에 목메지 않아도 된다.
 * 일단 lock 관련해서는 무시...
 ***********************************************************/
static int get_new_wordid( word_db_t* word_db, word_t *word )
{
	lexicon_t* db;
	hash_data_t wordid, string, oldwordid;
	int len, ret;
	
	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;

	len = strlen(word->string);
	if ( len <=0 )  return WORD_NOT_REGISTERED;

	word->id = db->shared->last_wordid + 1;

	wordid.data = &word->id;
	wordid.size = (int) sizeof(word->id);
	wordid.data_len = (int) sizeof(word->id);
	wordid.partial_op = 0;

	string.data = word->string;
	string.size = len+1; // NULL 포함
	string.data_len = sizeof(word->string);
	string.partial_op = 0;

	oldwordid.data = &word->id; // wordid와 같지만 잘 동작할 것이다...?
	oldwordid.data_len = (int) sizeof(word->id);
	oldwordid.partial_op = 0;

	ret = sb_run_hash_put( db->wordtoid_hash, &string, &wordid, 0, &oldwordid );
	if ( ret == HASH_KEY_EXISTS ) return WORD_OLD_REGISTERED;
	else if ( ret != SUCCESS ) {
		error("write to hash (word->id) failed. word[%s]", word->string);
		return FAIL;
	}

	db->shared->last_wordid++;

	// id->word hash는 없으면 여기서 끝
	if ( !db->idtoword_hash ) return WORD_NEW_REGISTERED;

	ret = sb_run_hash_put( db->idtoword_hash, &wordid, &string, 0, NULL );
	if ( ret == HASH_KEY_EXISTS ) {
		// 위쪽에서는 잘 통과했는데 여기서 이럴 수는 없다....
		crit("that's problem");
		error("word[%s] is in invalid state. wordid in hash[%u]", word->string, word->id );
		return FAIL;
	}
	else if ( ret != SUCCESS ) {
		error("write to hash (id->word) failed. word[%s]", word->string);
		return FAIL;
	}

	return WORD_NEW_REGISTERED;
}


static int get_wordid(word_db_t* word_db, word_t *word )
{
	lexicon_t* db;
	int len, ret;
	hash_data_t wordid, string;

	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;

	len = strlen(word->string);
	if ( len <= 0 )  return WORD_NOT_REGISTERED;

	string.data = word->string;
	string.size = len+1;
	string.data_len = sizeof(word->string);

	wordid.data = &word->id;
	wordid.data_len = sizeof(word->id);
	wordid.partial_op = 0;

	ret = sb_run_hash_get( db->wordtoid_hash, &string, &wordid );
	if ( ret == HASH_KEY_NOTEXISTS ) return WORD_NOT_REGISTERED;
	else if ( ret != SUCCESS ) {
		error("hash_get failed[%d]. word[%s]", ret, word->string);
		return FAIL;
	}
	else return WORD_OLD_REGISTERED;
}

static int get_word_by_wordid(word_db_t* word_db, word_t *word )
{
	lexicon_t* db;
	int ret;
	hash_data_t wordid, string;

	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;

	if ( !db->idtoword_hash ) {
		warn("id->word hash is not exists. see config");
		return DECLINE; // 이렇게 해도 괜찮겠지?
	}

	// id check
	if ( word->id <= 0 || db->shared->last_wordid < word->id ) {
		warn("not proper wordid [%u]", word->id);
		return WORD_NOT_REGISTERED;
	}

	wordid.data = &word->id;
	wordid.size = sizeof(word->id);
	wordid.data_len = sizeof(word->id);

	string.data = word->string;
	string.data_len = sizeof(word->string);
	string.partial_op = 0;

	ret = sb_run_hash_get( db->idtoword_hash, &wordid, &string );
	if ( ret == HASH_KEY_NOTEXISTS ) return WORD_NOT_REGISTERED;
	else if ( ret != SUCCESS ) {
		error("hash_get failed[%d]. wordid[%u]", ret, word->id);
		return FAIL;
	}
	else return WORD_OLD_REGISTERED;
}

static int get_num_of_wordid(word_db_t* word_db, uint32_t *num_of_word )
{
	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;

	*num_of_word = ((lexicon_t*)word_db->db)->shared->last_wordid;

	return SUCCESS;
}


/* ###########################################################################
 * frame & configuration stuff here
 * ########################################################################## */

static void get_word_db_set(configValue v)
{
	static word_db_set_t local_word_db_set[MAX_WORD_DB_SET];
	int value = atoi( v.argument[0] );

	if ( value < 0 || value >= MAX_WORD_DB_SET ) {
		error("Invalid WordDbSet value[%s], MAX_WORD_DB_SET[%d]",
				v.argument[0], MAX_WORD_DB_SET);
		return;
	}

	if ( word_db_set == NULL ) {
		memset( local_word_db_set, 0, sizeof(local_word_db_set) );
		word_db_set = local_word_db_set;
	}

	current_word_db_set = value;
	word_db_set[value].set = 1;
}

static void get_wordtoid_hash_set(configValue v)
{
	if ( word_db_set == NULL || current_word_db_set < 0 ) {
		error("first, set WordDbSet");
		return;
	}

	word_db_set[current_word_db_set].wordtoid_hash_set = atoi( v.argument[0] );
	word_db_set[current_word_db_set].set_wordtoid_hash_set = 1;
}

static void get_idtoword_hash_set(configValue v)
{
	if ( word_db_set == NULL || current_word_db_set < 0 ) {
		error("first, set WordDbSet");
		return;
	}

	word_db_set[current_word_db_set].idtoword_hash_set = atoi( v.argument[0] );
	word_db_set[current_word_db_set].set_idtoword_hash_set = 1;
}

static void get_shared_file(configValue v)
{
	if ( word_db_set == NULL || current_word_db_set < 0 ) {
		error("first, set WordDbSet");
		return;
	}

	strncpy( word_db_set[current_word_db_set].shared_file, v.argument[0], MAX_PATH_LEN );
	word_db_set[current_word_db_set].set_shared_file = 1;
}

static config_t config[] = {
	CONFIG_GET("WordDbSet", get_word_db_set, 1, "WordDbSet {number}"),
	CONFIG_GET("WordToIdHashSet", get_wordtoid_hash_set, 1, "hash for word -> id"),
	CONFIG_GET("IdToWordHashSet", get_idtoword_hash_set, 1, "hash for id -> word"),
	CONFIG_GET("SharedFile", get_shared_file, 1, "full filename of lexicon_t.shared"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_open_word_db		(open_word_db,		NULL, NULL, HOOK_MIDDLE);
	sb_hook_sync_word_db		(sync_word_db 	   ,NULL, NULL, HOOK_MIDDLE);
	sb_hook_close_word_db		(close_word_db,		NULL, NULL, HOOK_MIDDLE);

	sb_hook_get_new_wordid 		(get_new_wordid, 	NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_word 			(get_wordid, 		NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_word_by_wordid 	(get_word_by_wordid,NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_num_of_word		(get_num_of_wordid ,NULL, NULL, HOOK_MIDDLE);
}

module lexicon2_module=
{
	STANDARD_MODULE_STUFF,
    config,                 /* config */
    NULL,                   /* registry */
    init,                   /* initialize function */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
