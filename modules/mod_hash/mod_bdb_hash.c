#include "mod_api/hash.h"
#include <db.h>
#include <errno.h>

/*************************************************************
 * Berkeley DB 를 사용해서 hash 구현
 *************************************************************/

typedef struct _bdb_hash_t {
	int opened; // open이후 ~ close이전 1. 아니면 0
	DB_ENV* dbenvp;
	DB* dbp;
	DBTYPE type;
} bdb_hash_t;

typedef struct _bdb_hash_set_t {
	int set;

	int set_path;
	char path[MAX_PATH_LEN];

	int set_filename;
	char filename[MAX_PATH_LEN];
} bdb_hash_set_t;

#define MAX_HASH_SET (10)
bdb_hash_set_t* hash_set = NULL;
int current_hash_set = -1;

static int bdb_hash_create(void** hash);
static int bdb_hash_destroy(void* hash);
static int bdb_hash_open(void* hash, int opt);
static int bdb_hash_sync(void* hash);
static int bdb_hash_close(void* hash);
static int bdb_hash_put(void* hash, hash_data_t* key, hash_data_t* value,
		int opt, hash_data_t* old_value);
static int bdb_hash_get(void* hash, hash_data_t* key, hash_data_t* value);
static int bdb_hash_count(void* hash, int* count);

static int bdb_hash_init()
{
	info("%s", DB_VERSION_STRING);

	if ( DB_VERSION_MAJOR < 4 ) {
		error("Berkeley DB version is too low. At least 4.x.x is needed.");
		return FAIL;
	}

	return SUCCESS;
}

static int bdb_hash_create(void** hash)
{
	bdb_hash_t* bdb_hash = sb_malloc( sizeof(bdb_hash_t) );

	*hash = NULL;

	if ( bdb_hash == NULL ) {
		error("hash create (malloc) failed: %s", strerror(errno));
		return FAIL;
	}
	*hash = (void*) bdb_hash;
	return SUCCESS;
}

static int bdb_hash_destroy(void* hash)
{
	bdb_hash_t* bdb_hash = (bdb_hash_t*) hash;

	if ( bdb_hash->opened ) bdb_hash_close( hash );
	sb_free( bdb_hash );

	return SUCCESS;
}

static int bdb_hash_open(void* hash, int opt)
{
	int ret;
	bdb_hash_t* bdb_hash = (bdb_hash_t*) hash;
	char abs_path[MAX_PATH_LEN], abs_file[MAX_PATH_LEN];
	char* path, *filename;

	if ( hash_set == NULL ) {
		error("hash_set is NULL. you must set HashSet in config file");
		return FAIL;
	}

	if ( opt >= MAX_HASH_SET || opt < 0 ) {
		error("opt[%d] is invalid. MAX_HASH_SET[%d]", opt, MAX_HASH_SET);
		return FAIL;
	}

	if ( !hash_set[opt].set ) {
		error("HashSet[opt:%d] is not defined", opt);
		return FAIL;
	}

	if ( !hash_set[opt].set_path ) {
		error("HashPath is not set [HashSet:%d]. see config", opt);
		return FAIL;
	}

	if ( !hash_set[opt].set_filename ) {
		error("HashFile is not set. [HashSet:%d]. see confnig", opt);
		return FAIL;
	}

	path = hash_set[opt].path;
	filename = hash_set[opt].filename;

	if ( bdb_hash->opened )
		warn("already opened? - dbenvp[%p], dbp[%p]", bdb_hash->dbenvp, bdb_hash->dbp);

	// 절대경로 만들어내기
	if ( path[0] == '/' ) strncpy( abs_path, path, sizeof(abs_path) );
	else {
		snprintf( abs_path, sizeof(abs_path), "%s/%s", gSoftBotRoot, path );
	}

	snprintf( abs_file, sizeof(abs_file), "%s/%s", abs_path, filename );

	// no fail
	db_env_create( &bdb_hash->dbenvp, 0 );

	ret = bdb_hash->dbenvp->open( bdb_hash->dbenvp, abs_path,
			DB_INIT_LOCK|DB_INIT_MPOOL|DB_CREATE, 0 );
	if ( ret != 0 ) {
		error("DB_ENV->open() failed: %d, %s", ret, strerror(ret));
		goto fail;
	}

	ret = db_create( &bdb_hash->dbp, bdb_hash->dbenvp, 0 );
	if ( ret != 0 ) {
		error("db_create failed. ret:%d, EINVAL[%d]", ret, EINVAL);
		goto fail;
	}

	ret = bdb_hash->dbp->open( bdb_hash->dbp, NULL, abs_file, NULL, DB_HASH, DB_CREATE, 0 );
	if ( ret != 0 ) {
		error("DB->open() failed: %d, %s", ret, strerror(ret));
		goto fail;
	}

	ret = bdb_hash->dbp->get_type( bdb_hash->dbp, &bdb_hash->type );
	if ( ret != 0 ) {
		error("DB->get_type() failed: %d, %s", ret, strerror(ret));
		goto fail;
	}

	bdb_hash->opened = 1;
	return SUCCESS;

fail:
	if ( bdb_hash->dbp ) {
		bdb_hash->dbp->close( bdb_hash->dbp, 0 );
		bdb_hash->dbp = NULL;
	}

	if ( bdb_hash->dbenvp ) {
		bdb_hash->dbenvp->close( bdb_hash->dbenvp, 0 );
		bdb_hash->dbenvp = NULL;
	}

	return FAIL;
}

static int bdb_hash_sync(void* hash)
{
	bdb_hash_t* bdb_hash = (bdb_hash_t*) hash;
	int ret;

	ret = bdb_hash->dbp->sync( bdb_hash->dbp, 0 );
	if ( ret != 0 ) {
		error("DB->sync() failed. %d, %s", ret, strerror(ret));
		return FAIL;
	}

	return SUCCESS;
}

static int bdb_hash_close(void* hash)
{
	bdb_hash_t* bdb_hash = (bdb_hash_t*) hash;

	if ( !bdb_hash->opened ) {
		warn("hash is not opened");
		return SUCCESS;
	}

	bdb_hash->dbp->close( bdb_hash->dbp, 0 );
	bdb_hash->dbenvp->close( bdb_hash->dbenvp, 0 );

	memset( bdb_hash, 0, sizeof(bdb_hash_t) );

	return SUCCESS;
}

// db 에는 NULL 문자 저장하지 않음. 하지만 key, value에는 꼭 있어야 함
static int bdb_hash_put(void* hash, hash_data_t* key, hash_data_t* value,
		int opt, hash_data_t* old_value)
{
	bdb_hash_t* bdb_hash = (bdb_hash_t*) hash;
	DBT key_dbt, value_dbt;
	int ret;

	key_dbt.data = key->data;
	key_dbt.size = key->size;
	key_dbt.ulen = key->data_len;
	key_dbt.flags = DB_DBT_USERMEM;

	value_dbt.data = value->data;
	value_dbt.size = value->size;
	value_dbt.ulen = value->data_len;

	if ( value->partial_op ) {
		value_dbt.dlen = value->partial_len;
		value_dbt.doff = value->partial_off;
		value_dbt.flags = DB_DBT_USERMEM|DB_DBT_PARTIAL;
	}
	else value_dbt.flags = DB_DBT_USERMEM;

	// 덮어쓰기가 괜찮고 이전 value를 따로 저장할 필요가 없으면...
	if ( (opt & HASH_OVERWRITE) && !old_value ) goto overwrite;

	ret = bdb_hash->dbp->put( bdb_hash->dbp, NULL, &key_dbt, &value_dbt, DB_NOOVERWRITE );
	if ( ret == 0 ) return SUCCESS;
	else if ( ret != DB_KEYEXIST ) {
		error("DB->put() failed: %d, %s", ret, strerror(ret));
		return FAIL;
	}
	else if ( !old_value ) return HASH_KEY_EXISTS;

	ret = bdb_hash_get( hash, key, old_value );
	if ( ret == HASH_BUFFER_SMALL ) {
		error("bdb_hash_get failed with HASH_BUFFER_SMALL");
		return HASH_BUFFER_SMALL;
	}
	else if ( ret != SUCCESS ) {
		error("get old_value failed: ret[%d], %s", ret, strerror(ret));
		return FAIL;
	}

	if ( !(opt & HASH_OVERWRITE) ) return HASH_KEY_EXISTS;

overwrite:
	ret = bdb_hash->dbp->put( bdb_hash->dbp, NULL, &key_dbt, &value_dbt, 0 );
	if ( ret != 0 ) {
		error("DB->put() failed: %d, %s", ret, strerror(ret));
		return FAIL;
	}
	
	return SUCCESS;
}

// db 에는 NULL문자가 없으므로 붙여서 return
static int bdb_hash_get(void* hash, hash_data_t* key, hash_data_t* value)
{
	bdb_hash_t* bdb_hash = (bdb_hash_t*) hash;
	DBT key_dbt, value_dbt;
	int ret;

	key_dbt.data = key->data;
	key_dbt.size = key->size;
	key_dbt.ulen = key->data_len;
	key_dbt.flags = DB_DBT_USERMEM;

	value_dbt.data = value->data;
	value_dbt.ulen = value->data_len;

	if ( value->partial_op ) {
		value_dbt.dlen = value->partial_len;
		value_dbt.doff = value->partial_off;
		value_dbt.flags = DB_DBT_USERMEM|DB_DBT_PARTIAL;
	}
	else value_dbt.flags = DB_DBT_USERMEM;

	ret = bdb_hash->dbp->get( bdb_hash->dbp, NULL, &key_dbt, &value_dbt, 0);
	if ( ret != 0 ) {
		value->size = 0;

		if ( ret == DB_NOTFOUND ) return HASH_KEY_NOTEXISTS;
		if ( ret == DB_BUFFER_SMALL ) {
			error("insufficient buffer. needed[%d], your buffer[%d]",
					value_dbt.size, value_dbt.ulen);

			value->size = value_dbt.size;
			return HASH_BUFFER_SMALL;
		}
		else error("DB->get() failed: %d, %s", ret, strerror(ret));

		return FAIL;
	}

	value->size = value_dbt.size;
	return SUCCESS;
}

static int bdb_hash_count(void* hash, int* count)
{
	bdb_hash_t* bdb_hash = (bdb_hash_t*) hash;
	void* sp;
	int ret;

	// dbp->stat() 이 multi process인 상황에서도 믿을 수 있는지 확인해야 한다.
	ret = bdb_hash->dbp->stat( bdb_hash->dbp, NULL, &sp, 0 );
	if ( ret != 0 ) {
		error("DB->stat() failed: %d, %s", ret, strerror(ret));
		return FAIL;
	}

	switch (bdb_hash->type) {
		case DB_BTREE:
		case DB_RECNO:
			*count = (int) ((DB_BTREE_STAT*)sp)->bt_nkeys;
			break;
		case DB_HASH:
			*count = (int) ((DB_HASH_STAT*)sp)->hash_nkeys;
			break;
		case DB_QUEUE:
			*count = (int) ((DB_QUEUE_STAT*)sp)->qs_nkeys;
			break;
		default:
			error("unknown type: %d", (int) bdb_hash->type);
			free( sp );
			return FAIL;
	}

	free( sp );
	return SUCCESS;
}

/***********************************************
 *              config functions
 ***********************************************/

static void get_hash_set(configValue v)
{
	int value = atoi( v.argument[0] );
	static bdb_hash_set_t local_hash_set[MAX_HASH_SET];

	if ( value < 0  || value >= MAX_HASH_SET ) {
		error("Invalid HashSet value[%s], MAX_HASH_SET[%d]",
				v.argument[0], MAX_HASH_SET);
		return;
	}

	if ( hash_set == NULL ) {
		memset( local_hash_set, 0, sizeof(local_hash_set) );
		hash_set = local_hash_set;
	}

	current_hash_set = value;
	hash_set[value].set = 1;
}

static void get_hash_path(configValue v)
{
	if ( hash_set == NULL || current_hash_set < 0 ) {
		error("first, set HashSet");
		return;
	}

	strncpy( hash_set[current_hash_set].path, v.argument[0], MAX_PATH_LEN );
	hash_set[current_hash_set].set_path = 1;
}

static void get_hash_file(configValue v)
{
	if ( hash_set == NULL || current_hash_set < 0 ) {
		error("first, set HashSet");
		return;
	}

	strncpy( hash_set[current_hash_set].filename, v.argument[0], MAX_PATH_LEN );
	hash_set[current_hash_set].set_filename = 1;
}

static config_t config[] = {
	CONFIG_GET("HashSet", get_hash_set, 1, "Hash Set 0~..."),
	CONFIG_GET("HashPath", get_hash_path, 1, "Hash DB path. ex> HashPath dat/lexicon"),
	CONFIG_GET("HashFile", get_hash_file, 1, "Hash DB filename. ex> HashFile word.db"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_hash_create(bdb_hash_create, NULL, NULL, HOOK_MIDDLE);
	sb_hook_hash_destroy(bdb_hash_destroy, NULL, NULL, HOOK_MIDDLE);
	sb_hook_hash_open(bdb_hash_open, NULL, NULL, HOOK_MIDDLE);
	sb_hook_hash_sync(bdb_hash_sync, NULL, NULL, HOOK_MIDDLE);
	sb_hook_hash_close(bdb_hash_close, NULL, NULL, HOOK_MIDDLE);
	sb_hook_hash_put(bdb_hash_put, NULL, NULL, HOOK_MIDDLE);
	sb_hook_hash_get(bdb_hash_get, NULL, NULL, HOOK_MIDDLE);
	sb_hook_hash_count(bdb_hash_count, NULL, NULL, HOOK_MIDDLE);
}

module bdb_hash_module = {
	STANDARD_MODULE_STUFF,
	config,                  /* config */
	NULL,                    /* registry */
	bdb_hash_init,           /* initialize */
	NULL,                    /* child_main */
	NULL,                    /* scoreboard */
	register_hooks,          /* register hook api */
};

/**********************************************************
 *                      test  stuff
 **********************************************************/

#include "mod_mp/mod_mp.h"

static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(1) };

static void _do_nothing(int sig)
{
	return;
}

static void _shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	scoreboard->shutdown++;
}

static void _graceful_shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	scoreboard->shutdown++;
}

static int test_main(slot_t *slot)
{
	void *hash;
	char key_string[100], value_string[100], old_value_string[100];
	hash_data_t key, value, old_value;
	int i, ret;

	slot->state = SLOT_PROCESS;

	if ( sb_run_hash_create( &hash ) != SUCCESS ) {
		error("hash_create failed");
		goto error;
	}

	if ( sb_run_hash_open( hash, 0 ) != SUCCESS ) {
		error("hash_open failed");
		goto error;
	}

	key.data = key_string;
	key.data_len = sizeof(key_string);

	value.data = value_string;
	value.data_len = sizeof(value_string);
	value.partial_op = 0;

	old_value.data = old_value_string;
	old_value.data_len = sizeof(old_value_string);
	old_value.partial_op = 0;

	for ( i = 0; i < 10; i++ ) {
		snprintf( key_string, sizeof(key_string), "key%d", i );
		snprintf( value_string, sizeof(value_string), "value%d", i );

		key.size = strlen( key_string );
		value.size = strlen( value_string );

		ret = sb_run_hash_put( hash, &key, &value, (i%2)?HASH_OVERWRITE:0,
				&old_value );
		//		NULL, 0 );
		if ( ret == HASH_KEY_EXISTS ) error("i[%d] key %s is already exists - %s",
				i, key_string, old_value_string);
	//	if ( ret == HASH_KEY_EXISTS ) error("i[%d] key %s is already exists", i, key_string);
		if ( ret != SUCCESS ) error("i[%d] %d returned", i, ret);
	}

	value.partial_op = 1;
	value.partial_off = 3;
	value.partial_len = 2;
	ret = sb_run_hash_put( hash, &key, &value, HASH_OVERWRITE, NULL );
	if ( ret != SUCCESS ) error("last test failed");

	value.partial_op = 0;

	if ( sb_run_hash_sync( hash ) != SUCCESS ) {
		error("hash_sync failed");
		goto error;
	}

	for ( i = 0; i < 11; i++ ) {
		memset(value.data, 0, sizeof(value_string));
		snprintf( key_string, sizeof(key_string), "key%d", i );
		key.size = strlen( key_string );

		ret = sb_run_hash_get( hash, &key, &value );
		if ( ret == SUCCESS ) info("read %d: %s", i, value_string);
		else error("read %d failed: %d", i, ret);
	}

	if ( sb_run_hash_count( hash, &i ) != SUCCESS ) {
		error("hash_count failed");
		goto error;
	}
	info("hash count is %d", i);

	if ( sb_run_hash_close( hash ) != SUCCESS ) {
		error("hash_close failed");
		goto error;
	}

	if ( sb_run_hash_destroy( hash ) != SUCCESS ) {
		error("hash_destroy failed");
		goto error;
	}

	slot->state = SLOT_FINISH;
	return EXIT_SUCCESS;

error:
	sleep(5);
	slot->state = SLOT_RESTART;
	return -1;
}

static int test_module_main(slot_t *slot)
{
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);
	scoreboard->size = 1;
	
	sb_init_scoreboard(scoreboard);
	sb_spawn_processes(scoreboard, "bdb_hash test module", test_main);

	scoreboard->period = 2;
	sb_monitor_processes(scoreboard);

	return 0;
}

module bdb_hash_test_module = {
	STANDARD_MODULE_STUFF,
	NULL,                   /* config */
	NULL,                   /* registry */
	NULL,                   /* initialize */
	test_module_main,       /* child_main */
	scoreboard,             /* scoreboard */
	NULL,                   /* register hook api */
};

