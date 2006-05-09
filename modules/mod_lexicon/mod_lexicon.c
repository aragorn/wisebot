/* $Id$ */
#include <stdlib.h> /* atoi(3) */
#include <fcntl.h> /* O_CREAT,O_WRONLY */
#include <string.h> /* strerror(3) */
#include <errno.h>
#include "common_core.h"
#include "ipc.h"
#include "memory.h"
#include "lexicon_index.h"
#include "mod_lexicon.h"

typedef struct _word_db_set_t {
	int set;

	int set_lexicon_file;
	char lexicon_file[MAX_PATH_LEN];

	int set_lock_id;
	int lock_id;
} word_db_set_t;

static word_db_set_t* word_db_set = NULL;
static int current_word_db_set = -1;

// open을 singleton으로 구현하기 위한 것
static word_db_t* g_word_db[MAX_WORD_DB_SET];
static int g_word_db_ref[MAX_WORD_DB_SET];

char type_string[3][10]={"error","FIXED","VARIABLE"}; 

static int init_word_db(lexicon_t *word_db);
static int load_block(lexicon_t *word_db, int flag, int type, int block_idx);
static int alloc_word_db(lexicon_t *word_db, int *mmap_attr);

static int open_word_db(word_db_t** word_db, int opt);
static int close_word_db(word_db_t *word_db);
static int get_new_wordid(word_db_t *word_db, word_t *word);
static int get_wordid(word_db_t *word_db, word_t *word );
int get_word_by_wordid(word_db_t *word_db, word_t *word);

#define ACQUIRE_LOCK() \
	if ( db->lock_ref_count == 0 && acquire_lock( db->lock_id ) != SUCCESS ) return FAIL; \
	else db->lock_ref_count++;
#define RELEASE_LOCK() \
	if ( db->lock_ref_count <= 0 ) warn( "invalid lock_ref_count[%d]", db->lock_ref_count ); \
	else if ( db->lock_ref_count == 1 && release_lock( db->lock_id ) != SUCCESS ) return FAIL; \
	else db->lock_ref_count--;


/***********************************************************************/

static int init() {
	ipc_t lock;
	int i, ret;

	if ( word_db_set == NULL ) return SUCCESS;

	lock.type = IPC_TYPE_SEM;
	lock.pid  = SYS5_LEXICON;

	for ( i = 0; i < MAX_WORD_DB_SET; i++ ) {
		g_word_db[i] = NULL;

		if ( !word_db_set[i].set ) continue;

		if ( !word_db_set[i].set_lexicon_file ) {
			warn("LexiconFile [WordDbSet:%d] is not set", i);
			continue;
		}

		/* lock.pathname should not be a directory, but a normal file. */
		lock.pathname = word_db_set[i].lexicon_file;

		ret = get_sem(&lock);
		if ( ret != SUCCESS ) return FAIL;

		word_db_set[i].lock_id = lock.id;
		word_db_set[i].set_lock_id = 1;
	}

	return SUCCESS;
}

/* ############################################################################
 * lexicon database functions (sync & load )
 * ########################################################################## */

static int get_block_path(char *dest, lexicon_t *word_db, int type, int block_idx)
{
	return sprintf(dest, "%s.%c%03d",
				word_db->path, (type == BLOCK_TYPE_FIXED) ? 'f' : 'v', block_idx);
}

static int load_block(lexicon_t *word_db, int flag, int type, int block_idx)
{
	ipc_t mmap;
	char block_path[MAX_PATH_LEN];
	word_block_t *block;

	if (block_idx >= MAX_BLOCK_NUM) {
		CRIT("block_idx overflow : %d", block_idx);
		return FAIL;
	}
	
	get_block_path(block_path, word_db, type, block_idx);

	mmap.type		= IPC_TYPE_MMAP;
	mmap.pathname	= block_path;
	mmap.size		= sizeof(word_block_t);

	if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
		crit("error while allocating mmap for block[%s, %d]", block_path, block_idx);
		return FAIL;
	}

	block = mmap.addr;

	if ( type == BLOCK_TYPE_FIXED )
		word_db->fixed_block[block_idx] = block;
	else
		word_db->variable_block[block_idx] = block;

	/* TODO magic number check */
	/* TODO if block->used_bytes == 0 then read data from file */

	if ( mmap.attr == MMAP_CREATED ) {	
		block->used_bytes = 0;
		INFO("created %d bytes from file[%s], idx[%d]", mmap.size, block_path, block_idx);
	} 
	else if ( mmap.attr == MMAP_ATTACHED ) {
		INFO("shared attached offset[%d], idx[%d]", block->used_bytes, block_idx);	
	} 
	else {
		error("unknown mmap.attr [%d]", mmap.attr);
		return FAIL;
	}

	return SUCCESS;
}

static int unload_block(lexicon_t *word_db, int type, int block_idx)
{
	int ret;
	char block_path[MAX_PATH_LEN];

	if ( type == BLOCK_TYPE_FIXED ) {
		if ( word_db->fixed_block[block_idx] == NULL ) return SUCCESS;
		ret = free_mmap(word_db->fixed_block[block_idx], sizeof(word_block_t));
		if ( ret == SUCCESS ) word_db->fixed_block[block_idx] = NULL;
	}
	else {
		if ( word_db->variable_block[block_idx] == NULL ) return SUCCESS;
		ret = free_mmap(word_db->variable_block[block_idx], sizeof(word_block_t));
		if ( ret == SUCCESS ) word_db->variable_block[block_idx] = NULL;
	}

	get_block_path(block_path, word_db, type, block_idx);
	if ( ret != SUCCESS ) error("unload block[%d][%s] failed", block_idx, block_path);
	else info("block[%d][%s] unloaded", block_idx, block_path);

	return ret;
}

static int save_block(lexicon_t *word_db, int type, int block_idx)
{
	char block_path[MAX_PATH_LEN];
	word_block_t *block;

	get_block_path(block_path, word_db, type, block_idx);

	/* TODO magic number check */

	if ( type == BLOCK_TYPE_FIXED )
		 block = word_db->fixed_block[block_idx];
	else block = word_db->variable_block[block_idx];

	// block == NULL 일 수 있다... 그냥 넘어갈까.. 아차피 mmap...
	if ( block && sync_mmap( block, sizeof(word_block_t) ) != SUCCESS ) {
		error("error while writing block[%s, %d]: %s",
				block_path, block_idx, strerror(errno));
		return FAIL;
	}
	
	info("save block[%s,%d] successed", block_path, block_idx);

	return SUCCESS;
}


/* ############################################################################
 * lexicon database functions (alloc)
 * ########################################################################## */

static int init_word_db(lexicon_t *word_db)
{
	int r;

	word_db->shared->last_wordid = 0;
	word_db->shared->alloc_fixed_block_num = 1;
	word_db->shared->alloc_variable_block_num = 1;

	r = load_block(word_db, O_WRONLY|O_CREAT|O_TRUNC, BLOCK_TYPE_FIXED, 0);
	if ( r != SUCCESS ) return FAIL;
	r = load_block(word_db, O_WRONLY|O_CREAT|O_TRUNC, BLOCK_TYPE_VARIABLE, 0);
	if ( r != SUCCESS ) return FAIL;

	return SUCCESS;
}

static int alloc_word_db(lexicon_t *word_db, int *mmap_attr)
{
	ipc_t mmap;

	mmap.type		= IPC_TYPE_MMAP;
	mmap.pathname	= word_db->path;
	mmap.size		= sizeof(lexicon_shared_t);


	if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
		crit("error while allocating mmap for word_db [%s]", word_db->path);
		return FAIL;
	}

	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );

	word_db->shared  = mmap.addr;
	*mmap_attr = mmap.attr;
	
	return SUCCESS;
}

static int free_word_db(lexicon_t *word_db)
{
	int ret;

	ret = free_mmap(word_db->shared, sizeof(lexicon_shared_t));
	if ( ret == SUCCESS ) word_db->shared = NULL;

	return ret;
}

/* ############################################################################
 * lexicon database functions (set & get)
 * ########################################################################## */

/*
int variable_block_set_string(char string[], internal_word_t *internal_word)
{
	int block_num;
	int len=0, ret=0;

	//XXX : len = strlen(string) 하는것이 더 좋은방법인가?
	
	len = internal_word->word_attr.length;
	DEBUG("word [%s]'s len is (%d)", string, len);
	if (len <= 0) {
		return FAIL;
	} else if (MAX_WORD_LEN <= len) {
		string[MAX_WORD_LEN-1] = '\0';
		len = internal_word->word_attr.length = MAX_WORD_LEN-1;
	}
	
	block_num = word_db.shared->alloc_variable_block_num-1;
	if (block_num == -1) block_num = 0;
	DEBUG("set block_num(%d)",block_num);
		
	if (word_db.variable_block[block_num] == NULL) { 
		ret = alloc_variable_block(block_num);
		if (ret != SUCCESS) return ret;
	}

	if ((VARIABLE_BLOCK_MAX_OFFSET - word_db.variable_block[block_num]->used_bytes) < len) {
		
		ret = sync_variable_block(block_num);
		if (ret != SUCCESS) return ret;
 
		block_num++;
		CRIT("variable block increase [%d]",block_num);
		
		ret = alloc_variable_block(block_num);
		if (ret != SUCCESS) return ret;

		word_db.shared->alloc_variable_block_num++;	
		word_db.variable_block[block_num]->used_bytes = 0;
	}
	
	string[len] = '\0';
	
	memcpy(word_db.variable_block[block_num]->data + word_db.variable_block[block_num]->used_bytes,
		   string,len+1);

	internal_word->word_offset.block_num = block_num;
	internal_word->word_offset.offset = word_db.variable_block[block_num]->used_bytes;
	word_db.variable_block[block_num]->used_bytes += len+1; 

	DEBUG("block[%d], offset(%d)",block_num , word_db.variable_block[block_num]->used_bytes);
	
	return SUCCESS;
}
*/

int block_write(lexicon_t *word_db, word_offset_t *word_offset, int type,
						void *data, int size)
{
	word_block_t *block;

	if ( type == BLOCK_TYPE_FIXED ) {
		block = word_db->fixed_block[word_offset->block_idx]; 
	} 
	else if ( type == BLOCK_TYPE_VARIABLE ) {
		block = word_db->variable_block[word_offset->block_idx];
	} else {
        error("unknown type [%d]",type);
	    return FAIL;
	}
	
	if (BLOCK_DATA_SIZE < word_offset->offset + size ) {
		error("memory encroachment ");
		return FAIL; 
	}

	memcpy(((char *)block->data) + word_offset->offset, data, size); // ~~~~~~

	return size;
}

int new_vb_offset(lexicon_t *word_db, word_offset_t *word_offset)
{
	int r;
	int block_idx, offset;

	block_idx = word_db->shared->alloc_variable_block_num -1;
	if (word_db->variable_block[block_idx] == NULL) {
		r = load_block(word_db, O_WRONLY|O_CREAT, BLOCK_TYPE_VARIABLE, block_idx);
		if (r != SUCCESS) return FAIL;
	}
	offset = word_db->variable_block[block_idx]->used_bytes;

	if (offset + MAX_WORD_LEN >= BLOCK_DATA_SIZE - 1) {
		offset = 0;
		r = load_block(word_db, O_WRONLY|O_CREAT, BLOCK_TYPE_VARIABLE, block_idx+1);
		if ( r != SUCCESS ) return FAIL;
		if ( block_idx > MAX_BLOCK_NUM - 1) return FAIL;
	
		block_idx++;
		word_db->shared->alloc_variable_block_num++;
	}

	word_offset->block_idx = block_idx;
	word_offset->offset = offset;

	return SUCCESS;
}

static int new_fb_offset(lexicon_t *word_db, uint32_t wordid , word_offset_t *word_offset)
{
	int r;
	int block_idx, offset;

	block_idx = (int)((wordid - 1)/ NUM_WORDS_PER_BLOCK);
	offset = ((wordid-1) % NUM_WORDS_PER_BLOCK) * sizeof(internal_word_t);
	
	if (word_db->fixed_block[block_idx] == NULL) {
		r = load_block(word_db, O_WRONLY|O_CREAT, BLOCK_TYPE_FIXED, block_idx);
		if ( r != SUCCESS ) return FAIL;
		if ( block_idx > MAX_BLOCK_NUM -1 ) return FAIL;
		word_db->shared->alloc_fixed_block_num = block_idx + 1;
	}

	word_offset->block_idx = block_idx;
	word_offset->offset = offset;
	
	word_db->fixed_block[block_idx]->used_bytes = offset + sizeof(internal_word_t);
	
	return SUCCESS;
}

void *offset2ptr(lexicon_t *word_db, word_offset_t *word_offset, int type)
{
	int r;
	word_block_t *block;


/*	DEBUG("block[%d], offset[%d], type[%s]",*/
/*			word_offset->block_idx, word_offset->offset, type_string[type]);*/

	if ( type == BLOCK_TYPE_FIXED ) { /* BLOCK_TYPE_FIXED */
		if ( word_offset->block_idx >= word_db->shared->alloc_fixed_block_num ) {
			error("word_offset->block_idx[%d] >= word_db->shared->alloc_fixed_block_num[%d]",
				   word_offset->block_idx, word_db->shared->alloc_fixed_block_num);	
			return NULL;
		}
		
		block = word_db->fixed_block[word_offset->block_idx];
/*		DEBUG("block[block_idx %d] = [%p]", word_offset->block_idx, block);*/

		if ( block == NULL ) {
			r = load_block(word_db, O_RDWR|O_CREAT, BLOCK_TYPE_FIXED, word_offset->block_idx);
			if ( r != SUCCESS ) return NULL;
			block = word_db->fixed_block[word_offset->block_idx];
		}

	} else { /* BLOCK_TYPE_VARIABLE */
/*		DEBUG("word_offset->block_idx[%d], word_db->shared->alloc_variable_block_num[%d]",*/
/*				word_offset->block_idx, word_db->shared->alloc_variable_block_num);*/
		if ( word_offset->block_idx >= word_db->shared->alloc_variable_block_num )
			return NULL;

		block = word_db->variable_block[word_offset->block_idx];
/*		DEBUG("block[block_idx %d] = [%p]", word_offset->block_idx, block);*/

		if ( block == NULL ) {
			r = load_block(word_db, O_RDWR|O_CREAT, BLOCK_TYPE_VARIABLE, word_offset->block_idx);
			if ( r != SUCCESS ) return NULL;
			block = word_db->variable_block[word_offset->block_idx];
		}
	}

	return (void *)(((char *)(block->data)) + word_offset->offset);
}

static internal_word_t *get_internal_word_ptr_in_fb(lexicon_t *word_db, uint32_t wordid)
{
	word_offset_t word_offset;
	internal_word_t *internal_word;

	word_offset.block_idx = (int)((wordid - 1)/ NUM_WORDS_PER_BLOCK);
	word_offset.offset = ((wordid-1) % NUM_WORDS_PER_BLOCK) * sizeof(internal_word_t);

	//DEBUG("block_idx %d, offset %d", word_offset.block_idx, word_offset.offset);
	internal_word = offset2ptr(word_db, &word_offset, BLOCK_TYPE_FIXED);
	if (internal_word == NULL) {
		error(" internal_word null");	
		return NULL;
	}
	/* DEBUG(" wordid[%u] internal_word[%p]->df[%d]",
			wordid, internal_word, internal_word->word_attr.df); */
	return internal_word;
}


int increase_block_offset(lexicon_t *word_db, int len, int type)
{
	int block_idx, ret;
	word_block_t *block;
	
	if ( type == BLOCK_TYPE_FIXED ) {
		block_idx 	= word_db->shared->alloc_fixed_block_num -1;
		block = word_db->fixed_block[block_idx];
	} else {
		block_idx 	= word_db->shared->alloc_variable_block_num -1;
		block = word_db->variable_block[block_idx];
	}

	if ( block == NULL ) {
        ret = load_block(word_db, O_RDWR|O_CREAT, type, block_idx);
	    if (ret != SUCCESS) return ret;

		return increase_block_offset(word_db, len, type);
	}

	//DEBUG("type[%s] block[block_idx %d][%p]->used_bytes[%d] += %d",
	//	   type_string[type], block_idx, block, block->used_bytes, len);

	block->used_bytes += len;
	
	if(BLOCK_DATA_SIZE < block->used_bytes) {
		error("increase block offset: [%s] block[%d] offset[%d]",
				type_string[type], block_idx, block->used_bytes);
		return FAIL;
	}
	
	return SUCCESS;
}


static int check_db_not_full(lexicon_t *word_db)
{
	/* check if fixed/variable blocks are full */
	/* sizeof(internal_word_t) * [3] : just for a little room */
	if ( word_db->shared->alloc_fixed_block_num == MAX_BLOCK_NUM
			&& word_db->fixed_block[MAX_BLOCK_NUM-1]->used_bytes
					> BLOCK_DATA_SIZE - sizeof(internal_word_t)*3 )
		return FAIL;
	
	if ( word_db->shared->alloc_variable_block_num == MAX_BLOCK_NUM
			&& word_db->variable_block[MAX_BLOCK_NUM-1]->used_bytes
					> BLOCK_DATA_SIZE - MAX_WORD_LEN*3 )
		return FAIL;

	return SUCCESS;
}



/* ############################################################################
 * api functions
 * ########################################################################## */

static int open_word_db(word_db_t** word_db, int opt)
{
	lexicon_t* db = NULL;
	int i, r;
	int mmap_attr;

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
	if ( g_word_db[opt] != NULL ) {
		*word_db = g_word_db[opt];
		g_word_db_ref[opt]++;

		info("reopened word db[set:%d, ref:%d]", opt, g_word_db_ref[opt]);
		return SUCCESS;
	}

	if ( !word_db_set[opt].set_lexicon_file ) {
		error("LexiconFile is not set [WordDbSet:%d]. see config", opt);
		return FAIL;
	}

	if ( !word_db_set[opt].set_lock_id ) {
		error("invalid lock_id, maybe failed from init()");
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
	
	strncpy(db->path, word_db_set[opt].lexicon_file, MAX_PATH_LEN-1);
	if ( alloc_word_db(db, &mmap_attr) != SUCCESS ) goto error;

	/* TODO magic number check */
	if ( mmap_attr == MMAP_CREATED) {
		r  = init_word_db(db);
		if ( r != SUCCESS ) {
			error("word_db init failed");
			goto error;
		}
	} 
	else if(mmap_attr == MMAP_ATTACHED) {
		INFO("word_db->shared exist attach wordid[%u], fixed_block_num[%d], variable_block_num[%d]",
				db->shared->last_wordid, 
				db->shared->alloc_fixed_block_num,
				db->shared->alloc_variable_block_num);

		for (i = 0; i < db->shared->alloc_fixed_block_num; i++) {
			r = load_block(db, (O_RDWR|O_CREAT), BLOCK_TYPE_FIXED, i);
			if ( r != SUCCESS ) goto error;
		}
		for (i = 0; i < db->shared->alloc_variable_block_num; i++) {
			r = load_block(db, (O_RDWR|O_CREAT), BLOCK_TYPE_VARIABLE, i);
			if ( r != SUCCESS ) goto error;
		}
	} 
	else {
		error(" unknown mmap_attr [%d]",mmap_attr);
		return FAIL;	
	}

	/* open word db index */
	r = lexicon_index_open(db);
	if (r != SUCCESS) goto error;

	db->lock_id = word_db_set[opt].lock_id;
	db->lock_ref_count = 0;

	(*word_db)->set = opt;
	(*word_db)->db = (void*) db;
		
	return SUCCESS;

error:
	if ( db ) {
		if ( db->shared ) free_word_db( db );
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
	int i, r;

	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;

	INFO("last wordid (%u)", db->shared->last_wordid);
	DEBUG("alloc fixed block (%d) alloc variable block (%d)",
		  db->shared->alloc_fixed_block_num, 
		  db->shared->alloc_variable_block_num);

	info("saving word db[%s]...", db->path);

	for (i = 0; i < db->shared->alloc_fixed_block_num; i++) {
		r = save_block(db, BLOCK_TYPE_FIXED, i);
		if ( r != SUCCESS ) return FAIL;
	}

	for (i = 0; i < db->shared->alloc_variable_block_num; i++) {
		r = save_block(db, BLOCK_TYPE_VARIABLE, i);
		if ( r != SUCCESS ) return FAIL;
	}

	/* TODO magic number check */
	if ( sync_mmap( db->shared, sizeof(lexicon_shared_t) ) != SUCCESS ) {
		error("cannot write word db[%s], so create a new word db: %s",
				db->path, strerror(errno));
		return FAIL;
	}
	
	r = lexicon_index_sync(db);
	if ( r != SUCCESS ) return FAIL; 

	info("word db[%s] saved", db->path);
	return SUCCESS;
}

static int close_word_db(word_db_t* word_db)
{	
	lexicon_t* db;
	int i, ret;

	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;

	info("word db[%s] closing...", db->path);

	// 아직 reference count가 남아있으면 close하지 말아야 한다.
	g_word_db_ref[word_db->set]--;
	if ( g_word_db_ref[word_db->set] ) {
		info("word db[set:%d, ref:%d] is not closing now",
				word_db->set, g_word_db_ref[word_db->set]);
		return SUCCESS;
	}

	// sync db
	if (sync_word_db( word_db ) != SUCCESS) return FAIL;

	// lexicon index (dynamic hash , berkeley db ..) close
	ret = lexicon_index_close(db);
	if ( ret != SUCCESS ) error("lexicon index close failed");

	// free db
	for (i = 0; i < db->shared->alloc_fixed_block_num; i++) {
		unload_block(db, BLOCK_TYPE_FIXED, i);
	}

	for (i = 0; i < db->shared->alloc_variable_block_num; i++) {
		unload_block(db, BLOCK_TYPE_VARIABLE, i);
	}

	ret = free_word_db(db);

	info("word db[%s] closed", db->path);

	sb_free( word_db->db );
	sb_free( word_db );

	return ret;
}

static int get_new_wordid( word_db_t* word_db, word_t *word )
{
	lexicon_t* db;
	int len=0, r;
	uint32_t wordid;
	internal_word_t internal_word;
	word_offset_t fb_offset;	
	
	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;

	len = strlen(word->string);
	if ( len <=0 )  return WORD_NOT_REGISTERED;
	if ( len >= MAX_WORD_LEN ) {
		warn("length of word[%s] exceeds MAX_WORD_LEN[%d]", 
								word->string, MAX_WORD_LEN);
		len = MAX_WORD_LEN-1;
	}

	word->string[len] = '\0';
	//DEBUG("input word [%s]",word->string);	

	ACQUIRE_LOCK();
	wordid = db->shared->last_wordid + 1;

	// LEXICON INDEX PUT
	r = lexicon_index_put( db, word->string, &wordid , 
								&(internal_word.word_offset));
	if (r == WORD_OLD_REGISTERED) {
		word->id = wordid;
		RELEASE_LOCK();
		return WORD_OLD_REGISTERED;
	} else if (r == WORD_NEW_REGISTERED) {

		if (check_db_not_full(db) != SUCCESS) {
			RELEASE_LOCK();
			return FAIL;
		}
		
		db->shared->last_wordid++;
		if (db->shared->last_wordid != wordid)
			warn("word_db->shared->last_wordid[%u] != wordid[%u]",
					db->shared->last_wordid, wordid);
		
		wordid = db->shared->last_wordid;
		
        /* write fixed block */
        r = new_fb_offset(db,wordid, &fb_offset);
        if ( r != SUCCESS) {
			RELEASE_LOCK();
			return r;
		}

        r = block_write(db, &fb_offset, BLOCK_TYPE_FIXED,
                        &internal_word, sizeof(internal_word_t));
        if (r != sizeof(internal_word_t)) {
			RELEASE_LOCK();
			return r;
		}

		word->id = wordid;

		RELEASE_LOCK();
		return WORD_NEW_REGISTERED; 
	} else {
		RELEASE_LOCK();
		error("lexicon index put fail %d", r);
		return FAIL;
	}
}


static int get_wordid(word_db_t* word_db, word_t *word )
{
	lexicon_t* db;
	int len=0, ret;
	uint32_t wordid;
	
	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;

	len = strlen(word->string);
	if ( len <= 0 )  return WORD_NOT_REGISTERED;
	if ( len >= MAX_WORD_LEN ) {
		warn("length of word[%s] exceeds MAX_WORD_LEN[%d]", word->string, MAX_WORD_LEN);
		len = MAX_WORD_LEN-1;
	}
	word->string[len] = '\0';
	//DEBUG("input word [%s]",word->string);	

	ACQUIRE_LOCK();
	ret = lexicon_index_get( db, word->string, &wordid );	
	RELEASE_LOCK();

	debug("ret:%d\n", ret);
	if (ret == WORD_OLD_REGISTERED) {
		word->id = wordid;
       	return WORD_OLD_REGISTERED;
	} else if (ret == WORD_NOT_REGISTERED) {
		word->id = 0;
		return WORD_NOT_REGISTERED;
	} else {
		error("lexicon index get return %d",ret);
		return FAIL;
	}
}

// use this function at truncation api 
int get_word_by_wordid(word_db_t* word_db, word_t *word )
{
	lexicon_t* db;
	internal_word_t *internal_word;
	char *str;
	
	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;
	db = (lexicon_t*) word_db->db;

	// id check
	if ( word->id <= 0 || db->shared->last_wordid < word->id ) {
		warn("not proper wordid [%u]", word->id);
		return WORD_NOT_REGISTERED;
	}

	ACQUIRE_LOCK();
	// get fixed data
	internal_word = get_internal_word_ptr_in_fb(db, word->id);
	if ( internal_word == NULL ) {
		RELEASE_LOCK();
		return FAIL;
	}

	// get word string from variable block
	str = offset2ptr(db, &(internal_word->word_offset), BLOCK_TYPE_VARIABLE);
	if ( str == NULL ) {
		RELEASE_LOCK();
		return FAIL;
	}
	//DEBUG("string [%s] len [%d]",str , strlen(str));
	memcpy(&(word->string), str, strlen(str)+1);

	//DEBUG("wordid(%u) -> word[%s]", word->id , word->string);
	RELEASE_LOCK();
	return SUCCESS;
}

static int get_num_of_wordid(word_db_t* word_db, uint32_t *num_of_word )
{
	if ( word_db_set == NULL || !word_db_set[word_db->set].set )
		return DECLINE;

	*num_of_word = ((lexicon_t*) word_db->db)->shared->last_wordid;
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

static void get_lexicon_file(configValue v)
{
	if ( word_db_set == NULL || current_word_db_set < 0 ) {
		error("first, set WordDbSet");
		return;
	}

	strncpy( word_db_set[current_word_db_set].lexicon_file, v.argument[0], MAX_PATH_LEN );
	word_db_set[current_word_db_set].set_lexicon_file = 1;
}

static config_t config[] = {
	CONFIG_GET("WordDbSet", get_word_db_set, 1, "WordDbSet {number}"),
	CONFIG_GET("LexiconFile",get_lexicon_file,1,\
				"file name of mmap file. ex) LexiconFile dat/lexicon/lexicon"),
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

module lexicon_module=
{
	STANDARD_MODULE_STUFF,
    config,                 /* config */
    NULL,                   /* registry */
    init,                   /* initialize function */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};
