/* lexicon index berkeley db 
 * 
 *  $Id$
 */
// TODO : fill blank function
// 
#include <sys/types.h>
#include <limits.h>
#include <db.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "lexicon_index.h"
#include "mod_lexicon.h"

#define DATABASE	"access.db"
#define FILE_CREAT_MODE	(0600)

#define DB_CACHE_SIZE	(64*1024)
#define DB_ENV_FLAGS    (DB_CREATE|DB_INIT_MPOOL|DB_USE_ENVIRON)


int lexicon_index_open( word_db_t *word_db )
{
	char db_path[MAX_PATH_LEN];
	DB *dbp=NULL;
/*    char *db_config[] = {*/
/*        "DB_DATA_DIR /home/4/susia/temp/db/database",*/
/*        NULL*/
/*    };*/
    DB_ENV *dbenv=NULL;

	
    CRIT("lexicon index use berkeley database!");	
    
    sprintf(db_path, "%s/%s",gSoftBotRoot,"dat/lexicon/");
	debug("data base path [%s]", db_path);

    if ((dbenv = (DB_ENV *)sb_calloc(sizeof(DB_ENV), 1)) == NULL) {
        fprintf(stderr, "%s\n",strerror(errno));
        return FAIL;
    }
    dbenv->db_errfile = stderr;
    dbenv->db_errpfx = "lexicon.c";
    dbenv->mp_size = 32 * 1024;

    if( (errno = db_appinit(db_path,NULL , dbenv, DB_ENV_FLAGS)) !=0 ) {
        error("db: db_appinit fail %s",strerror(errno));
        return FAIL;
    }

    if ((errno = db_open(DATABASE, DB_BTREE, DB_CREATE, 0664, NULL, NULL, &dbp)) != 0) {
        error("db: %s: %s\n", DATABASE, strerror(errno));
        return FAIL;
    }

	word_db->hash = dbp;
	
	return SUCCESS;
}

int lexicon_index_sync  ( word_db_t *word_db )
{
	int ret;
	DB *dbp;

	dbp = word_db->hash;	

	ret = dbp->sync(dbp, 0);
    if (ret == 0) {
        INFO("db: database sync.\n");
    } else {
        error("db->sync error %s\n",strerror(ret));
    }
	return SUCCESS;
}

int lexicon_index_close ( word_db_t *word_db )
{
	int ret;
	DB *dbp;
	
	dbp = word_db->hash;

    ret = dbp->close(dbp, 0);
    if (ret == 0) {
        info("db: database closed.\n");
    } else {
        error("db->close error %s\n",strerror(ret));
    }
	return SUCCESS;
}

int lexicon_index_put   ( word_db_t *word_db, char* string, uint32_t* wordid)
{
	DB *dbp;
	DBT key, data;
	int ret , len;
	word_offset_t offset;
	
	dbp = (DB *)word_db->hash;

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	
	key.data = string;
	key.size = (strlen(string)+1);
	
	data.data = wordid;
	data.size = sizeof(uint32_t);

	debug("data.data [%u], key.data[%s]",*(uint32_t*)(data.data), (char *)key.data);

	ret = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE);
	switch (ret) {
	case 0: // success
		debug("word[%s] not exist. id [%u] registered.", 
			   (char *)key.data, *(uint32_t *)data.data);
	
#if 1	
		ret = new_vb_offset(word_db, &offset);
		if (ret != SUCCESS) return ret;
	
		/* write variable block */
		len = strlen(string); 
		// XXX: +1 means adding null character.
		ret = block_write(word_db, &(offset), BLOCK_TYPE_VARIABLE, string, len+1);
		if (ret != (len+1)) return ret;

		ret = increase_block_offset(word_db, len+1, BLOCK_TYPE_VARIABLE);
		if (ret != SUCCESS) return ret;
		
#endif	
		return LEXICON_INDEX_NEW_WORD;
	case DB_KEYEXIST:
		debug("word[%s] exist id[%u] will get again !!!",
			  (char *)key.data, *(uint32_t *)data.data);
		ret = dbp->get(dbp, NULL, &key, &data, 0);
		if (ret !=0) {
			error("word [%s] not put. but not found by dbp->get", (char *)key.data);
			return FAIL;
		}
		debug("word[%s], again get id [%u]", (char *)key.data, *(uint32_t *)data.data);	
		*wordid = *(uint32_t *)data.data;
		if (*wordid <= 0) abort(); 
		return LEXICON_INDEX_EXIST_WORD;
	default:	
		return FAIL;
	}
}

int lexicon_index_get   ( word_db_t *word_db, char* string, uint32_t* wordid)
{
	DB *dbp;
	DBT key, data;
	int ret;

	dbp =  (DB *)word_db->hash;

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	
	key.data = string;
	key.size = (strlen(string)+1);
	
	ret = dbp->get(dbp, NULL, &key, &data, 0);
	switch (ret) {
	case 0: //success
		debug("db: get word[%s] id[%u]", (char *)key.data, *(uint32_t*)data.data);
		*wordid = *(uint32_t *)data.data;
		return LEXICON_INDEX_EXIST_WORD;
	case DB_NOTFOUND:
		debug("db: get word[%s] not exist in database", (char *)key.data);
		return LEXICON_INDEX_NOT_EXIST_WORD;
	default:
		error("db: get fail %d",ret);
		return FAIL;
	}
}

int lexicon_index_del   ( word_db_t *word_db, char* string)
{
	warn("lexicon index del need fill");
	return SUCCESS;
}
