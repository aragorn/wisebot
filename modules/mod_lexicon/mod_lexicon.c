//TODO : load block interface

/* 
 * $Id$
 * 
 * mod_lexicon.c
 */
#include "mod_lexicon.h"
#include "lexicon_index.h"
#include "truncation.h"

word_db_t gWordDB;	/* lexicon database instance */
static int  lexicon_lock = -1;
static int  lock_ref_count = 0;
static char mLexiconDataPath[MAX_FILE_LEN] = "dat/lexicon/lexicon";
static int  m_lexicon_truncation_support   = TRUNCATION_NO; 
char type_string[3][10]={"error","FIXED","VARIABLE"}; 

static int init_word_db(word_db_t *word_db);
static int load_block(word_db_t *word_db, int flag, int type, int block_idx);
static int alloc_word_db(word_db_t *word_db, int *mmap_attr);
static int open_word_db(word_db_t *word_db, char *path, int flag);
static int close_word_db(word_db_t *word_db);

static int get_new_wordid(word_db_t *word_db, word_t *word, uint32_t did);
static int get_wordid(word_db_t *word_db, word_t *word );
int get_word_by_wordid(word_db_t *word_db, word_t *word);

#define ACQUIRE_LOCK() \
	if ( lock_ref_count == 0 && acquire_lock( lexicon_lock ) != SUCCESS ) return FAIL; \
	else lock_ref_count++;
#define RELEASE_LOCK() \
	if ( lock_ref_count <= 0 ) warn( "invalid lock_ref_count[%d]", lock_ref_count ); \
	else if ( lock_ref_count == 1 && release_lock( lexicon_lock ) != SUCCESS ) return FAIL; \
	else lock_ref_count--;


/***********************************************************************/

static int init() {
	ipc_t lock;
	int ret;

	// 이거는 왜 리턴값 검사 안하냐?
	sb_run_open_word_db(&gWordDB, mLexiconDataPath, O_RDWR|O_CREAT);

	lock.type = IPC_TYPE_SEM;
	lock.pid  = SYS5_LEXICON;
	/* lock.pathname should not be a directory, but a normal file. */
	lock.pathname = mLexiconDataPath;

	ret = get_sem(&lock);
	if ( ret != SUCCESS ) return FAIL;

	lexicon_lock = lock.id;

	return SUCCESS;
}

/* ############################################################################
 * lexicon database functions (sync & load )
 * ########################################################################## */

static int get_block_path(char *dest, word_db_t *word_db, int type, int block_idx)
{
	return sprintf(dest, "%s.%c%03d",
				word_db->path, (type == BLOCK_TYPE_FIXED) ? 'f' : 'v', block_idx);
}

static int load_block(word_db_t *word_db, int flag, int type, int block_idx)
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

static int unload_block(word_db_t *word_db, int type, int block_idx)
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

static int save_block(word_db_t *word_db, int type, int block_idx)
{
	char block_path[MAX_PATH_LEN];
	word_block_t *block;

	get_block_path(block_path, word_db, type, block_idx);

	/* TODO magic number check */

	if ( type == BLOCK_TYPE_FIXED )
		 block = word_db->fixed_block[block_idx];
	else block = word_db->variable_block[block_idx];

	if ( sync_mmap( block, sizeof(word_block_t) ) != SUCCESS ) {
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

static int init_word_db(word_db_t *word_db)
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

static int alloc_word_db(word_db_t *word_db, int *mmap_attr)
{
	ipc_t mmap;

	mmap.type		= IPC_TYPE_MMAP;
	mmap.pathname	= word_db->path;
	mmap.size		= sizeof(word_db_shared_t);


	if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
		crit("error while allocating mmap for word_db [%s]", word_db->path);
		return FAIL;
	}

	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );

	word_db->shared  = mmap.addr;
	*mmap_attr = mmap.attr;
	
	return SUCCESS;
}

static int free_word_db(word_db_t *word_db)
{
	int ret;

	ret = free_mmap(word_db->shared, sizeof(word_db_shared_t));
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

int block_write(word_db_t *word_db, word_offset_t *word_offset, int type,
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

int new_vb_offset(word_db_t *word_db, word_offset_t *word_offset)
{
	int r;
	int block_idx, offset;

	block_idx = word_db->shared->alloc_variable_block_num -1;
	if (word_db->variable_block[block_idx] == NULL) {
		r = load_block(word_db, O_WRONLY|O_CREAT, BLOCK_TYPE_VARIABLE, block_idx);
		if (r != SUCCESS) return FAIL;
	}
	offset = word_db->variable_block[block_idx]->used_bytes;

	if (offset + MAX_WORD_LENGTH >= BLOCK_DATA_SIZE - 1) {
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

static int new_fb_offset(word_db_t *word_db, uint32_t wordid , word_offset_t *word_offset)
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

void *offset2ptr(word_db_t *word_db, word_offset_t *word_offset, int type)
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

static internal_word_t *get_internal_word_ptr_in_fb(word_db_t *word_db, uint32_t wordid)
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


int increase_block_offset(word_db_t *word_db, int len, int type)
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


static int check_db_not_full(word_db_t *word_db)
{
	/* check if fixed/variable blocks are full */
	/* sizeof(internal_word_t) * [3] : just for a little room */
	if ( word_db->shared->alloc_fixed_block_num == MAX_BLOCK_NUM
			&& word_db->fixed_block[MAX_BLOCK_NUM-1]->used_bytes
					> BLOCK_DATA_SIZE - sizeof(internal_word_t)*3 )
		return WORD_ID_OVERFLOW;
	
	if ( word_db->shared->alloc_variable_block_num == MAX_BLOCK_NUM
			&& word_db->variable_block[MAX_BLOCK_NUM-1]->used_bytes
					> BLOCK_DATA_SIZE - MAX_WORD_LENGTH*3 )
		return WORD_ID_OVERFLOW;

	return SUCCESS;
}



/* ############################################################################
 * api functions
 * ########################################################################## */

static word_db_t* get_global_word_db()
{
	return &gWordDB;
}

static int open_word_db(word_db_t *word_db, char *path, int flag)
{
	int i, r;
	int mmap_attr;
	
	memset(word_db, 0x00, sizeof(word_db_t));
		
	strncpy(word_db->path, path, MAX_PATH_LEN-1);
	if ( alloc_word_db(word_db, &mmap_attr) != SUCCESS ) return FAIL;

/* TODO magic number check */
	if ( mmap_attr == MMAP_CREATED) {
		r  = init_word_db(word_db);
		if ( r != SUCCESS ) {
			error("word_db init failed");
			return FAIL;
		}
	} 
	else if(mmap_attr == MMAP_ATTACHED) {
		INFO("word_db->shared exist attach wordid[%u], fixed_block_num[%d], variable_block_num[%d]",
				word_db->shared->last_wordid, 
				word_db->shared->alloc_fixed_block_num,
				word_db->shared->alloc_variable_block_num);

		for (i = 0; i < word_db->shared->alloc_fixed_block_num; i++) {
			r = load_block(word_db, flag, BLOCK_TYPE_FIXED, i);
			if ( r != SUCCESS ) return FAIL;
		}
		for (i = 0; i < word_db->shared->alloc_variable_block_num; i++) {
			r = load_block(word_db, flag, BLOCK_TYPE_VARIABLE, i);
			if ( r != SUCCESS ) return FAIL;
		}
	} 
	else {
		error(" unknown mmap_attr [%d]",mmap_attr);
		return FAIL;	
	}

	/* open word db index */
	r = lexicon_index_open(word_db);
	if (r != SUCCESS) return FAIL;
		
	if (m_lexicon_truncation_support == TRUNCATION_YES) {
		r = truncation_open(word_db);
		if (r != SUCCESS) return FAIL;
	}
	
	return SUCCESS;
}


static int sync_word_db(word_db_t *word_db)
{
	int i, r;

	INFO("last wordid (%u)", word_db->shared->last_wordid);
	DEBUG("alloc fixed block (%d) alloc variable block (%d)",
		  word_db->shared->alloc_fixed_block_num, 
		  word_db->shared->alloc_variable_block_num);

	info("saving word db[%s]...", word_db->path);

	for (i = 0; i < word_db->shared->alloc_fixed_block_num; i++) {
		r = save_block(word_db, BLOCK_TYPE_FIXED, i);
		if ( r != SUCCESS ) return FAIL;
	}

	for (i = 0; i < word_db->shared->alloc_variable_block_num; i++) {
		r = save_block(word_db, BLOCK_TYPE_VARIABLE, i);
		if ( r != SUCCESS ) return FAIL;
	}

	/* TODO magic number check */
	if ( sync_mmap( word_db->shared, sizeof(word_db_shared_t) ) != SUCCESS ) {
		error("cannot write word_db[%s], so create a new word db: %s",
				word_db->path, strerror(errno));
		return FAIL;
	}
	
	r = lexicon_index_sync(word_db);
	if ( r != SUCCESS ) return FAIL; 

	if (m_lexicon_truncation_support == TRUNCATION_YES) {
		r = truncation_sync(word_db);
		if (r != SUCCESS) return FAIL;
	}
	
	info("word db[%s] saved", word_db->path);
	return SUCCESS;
}

static int close_word_db(word_db_t *word_db)
{	
	int i, ret;

	info("word db[%s] closing...", word_db->path);

	// sync word_db
	if (sync_word_db( word_db ) != SUCCESS) return FAIL;

	// close truncation database if needed
	if (m_lexicon_truncation_support == TRUNCATION_YES) {
		truncation_close(word_db);
	}
	
	// lexicon index (dynamic hash , berkeley db ..) close
	ret = lexicon_index_close(word_db);
	if ( ret != SUCCESS ) error("lexicon index close failed");

	// free word_db
	for (i = 0; i < word_db->shared->alloc_fixed_block_num; i++) {
		unload_block(word_db, BLOCK_TYPE_FIXED, i);
	}

	for (i = 0; i < word_db->shared->alloc_variable_block_num; i++) {
		unload_block(word_db, BLOCK_TYPE_VARIABLE, i);
	}

	ret = free_word_db(word_db);

	info("word db[%s] closed", word_db->path);
	return ret;
}

static int get_new_wordid( word_db_t *word_db, word_t *word, uint32_t did )
{
	int len=0, r;
	uint32_t wordid;
	internal_word_t internal_word;
	word_offset_t fb_offset;	
	
	len = strlen(word->string);
	if ( len <=0 )  return WORD_NOT_EXIST;
	if ( len >= MAX_WORD_LENGTH ) {
		warn("length of word[%s] exceeds MAX_WORD_LENGTH[%d]", 
								word->string, MAX_WORD_LENGTH);
		len = MAX_WORD_LENGTH-1;
	}

	word->string[len] = '\0';
	//DEBUG("input word [%s]",word->string);	

/*	internal_word.word_attr.id = word_db->shared->last_wordid + 1;*/
/*	internal_word.word_attr.length = len;*/
	ACQUIRE_LOCK();
	wordid = word_db->shared->last_wordid + 1;

	// LEXICON INDEX PUT
	r = lexicon_index_put( word_db, word->string, &wordid , 
								&(internal_word.word_offset));
	if (r == LEXICON_INDEX_EXIST_WORD) {
		internal_word_t *word_ptr;

		//DEBUG("hash key exist  [%s]'s id [%u]", word->string, wordid);
		word_ptr = get_internal_word_ptr_in_fb(word_db, wordid);
		if ( word_ptr == NULL ) {
			RELEASE_LOCK();
			return FAIL;
		}

		if (did > word_ptr->did) {
			word_ptr->did = did;
			word_ptr->word_attr.df++;
		} else if (did < word_ptr->did) {
			RELEASE_LOCK();
			error("did decrease input did[%u] < last did[%u]; \
				   maybe, data from Indexer is wrong.", did, word_ptr->did);
			return FAIL;
		}
		
		word->word_attr = word_ptr->word_attr;
		word->id = wordid;
		RELEASE_LOCK();
		return WORD_OLD_REGISTERED;
		
	} else if (r == LEXICON_INDEX_NEW_WORD) {

		if (check_db_not_full(word_db) != SUCCESS) {
			RELEASE_LOCK();
			return FAIL;
		}
		
		word_db->shared->last_wordid++;
		if (word_db->shared->last_wordid != wordid)
				warn("word_db->shared->last_wordid[%u] != wordid[%u]",
								word_db->shared->last_wordid, wordid);
		
		wordid = word_db->shared->last_wordid;
		
		// fill internal_word_attribute (id and length fill before put)
		internal_word.did = did;
		internal_word.word_attr.df = 1;

        /* write fixed block */
        r = new_fb_offset(word_db,wordid, &fb_offset);
        if ( r != SUCCESS) {
			RELEASE_LOCK();
			return r;
		}

        r = block_write(word_db, &fb_offset, BLOCK_TYPE_FIXED,
                        &internal_word, sizeof(internal_word_t));
        if (r != sizeof(internal_word_t)) {
			RELEASE_LOCK();
			return r;
		}

//        r = increase_block_offset(word_db, sizeof(internal_word_t), BLOCK_TYPE_FIXED);
//        if (r != SUCCESS) return r;

		word->word_attr = internal_word.word_attr;
		word->id = wordid;

		if ( word->id <= 0 ) // XXX:obsolete
		{
				error("word id <= 0! %d",word->id);
				RELEASE_LOCK();
				abort();
		}
		
		if (m_lexicon_truncation_support == TRUNCATION_YES) {
			put_truncation_word( word_db, *word );
		}
		
		RELEASE_LOCK();
		return WORD_NEW_REGISTERED; 
	} else {
		RELEASE_LOCK();
		error("lexicon index put fail %d", r);
		return FAIL;
	}
}


static int get_wordid(word_db_t *word_db, word_t *word )
{
	int len=0, ret;
	uint32_t wordid;
	
	len = strlen(word->string);
	if ( len <= 0 )  return WORD_NOT_EXIST;
	if ( len >= MAX_WORD_LENGTH ) {
		warn("length of word[%s] exceeds MAX_WORD_LENGTH[%d]", word->string, MAX_WORD_LENGTH);
		len = MAX_WORD_LENGTH-1;
	}
	word->string[len] = '\0';
	//DEBUG("input word [%s]",word->string);	

	ACQUIRE_LOCK();
	ret = lexicon_index_get( word_db, word->string, &wordid );	

	debug("ret:%d\n", ret);
	if (ret == LEXICON_INDEX_EXIST_WORD) {
        	internal_word_t *word_ptr;
        	//DEBUG("search success  [%s]'s id [%u]", word->string, wordid);
       		word_ptr = get_internal_word_ptr_in_fb(word_db, wordid);
        	if ( word_ptr == NULL ) {
				RELEASE_LOCK();
				return FAIL;
			}
       		memcpy(&(word->word_attr), &(word_ptr->word_attr), sizeof(word_attr_t));
			RELEASE_LOCK();

			word->id = wordid;
        	return WORD_OLD_REGISTERED;
	
	} else if (ret == LEXICON_INDEX_NOT_EXIST_WORD) {
		RELEASE_LOCK();
		word->id = 0;
		return WORD_NOT_REGISTERED;
	} else {
		RELEASE_LOCK();
		error("lexicon index get return %d",ret);
		return FAIL;
	}
}

// use this function at truncation api 
int get_word_by_wordid(word_db_t *word_db, word_t *word )
{
	internal_word_t *internal_word;
	char *str;
	
	// id check
	if ( word->id <= 0 || word_db->shared->last_wordid < word->id ) {
		warn("not proper wordid [%u]", word->id);
		return WORD_ID_NOT_EXIST;	
	}

	ACQUIRE_LOCK();
	// get fixed data
	internal_word = get_internal_word_ptr_in_fb(word_db, word->id);
	if ( internal_word == NULL ) {
		RELEASE_LOCK();
		return WORD_FIXED_DB_FAIL;
	}
	//DEBUG("word id[%d], df[%d], vb block[%d], offset[%d]",
	//		word->id,
	//		internal_word->word_attr.df,
	//		internal_word->word_offset.block_idx,
	//		internal_word->word_offset.offset);
	memcpy(&(word->word_attr), &(internal_word->word_attr), sizeof(word_attr_t));

	// get word string from variable block
	str = offset2ptr(word_db, &(internal_word->word_offset), BLOCK_TYPE_VARIABLE);
	if ( str == NULL ) {
		RELEASE_LOCK();
		return WORD_VARIABLE_DB_FAIL;
	}
	//DEBUG("string [%s] len [%d]",str , strlen(str));
	memcpy(&(word->string), str, strlen(str)+1);

	//DEBUG("wordid(%u) -> word[%s]", word->id , word->string);
	RELEASE_LOCK();
	return SUCCESS;
}

static int get_num_of_wordid(word_db_t *word_db, uint32_t *num_of_word )
{
	*num_of_word = word_db->shared->last_wordid;
	return SUCCESS;
}


static int get_right_truncation( word_db_t *word_db, char *word, 
								 word_t ret_word[], int max_search_num)
{
	int ret;

	if (m_lexicon_truncation_support == TRUNCATION_NO) {
		warn("truncation search not support");
		return 0; 
	}

	ACQUIRE_LOCK();
	ret = truncation_search(word_db, word, ret_word, 
							  max_search_num, TYPE_RIGHT_TRUNCATION);	
	RELEASE_LOCK();

	return ret;
}

static int get_left_truncation( word_db_t *word_db, char *word, 
								word_t ret_word[], int max_search_num)
{
	int ret;

	if (m_lexicon_truncation_support == TRUNCATION_NO) {
		return 0;
		warn("truncation search not support");
	}

	ACQUIRE_LOCK();
	ret = truncation_search(word_db, word, ret_word, 
							 max_search_num, TYPE_LEFT_TRUNCATION);	
	RELEASE_LOCK();

	return ret;
}


static int print_hash_bucket(word_db_t *word_db, int bucket_idx)
{
	print_bucket((hash_t *)word_db->hash, bucket_idx);
	return SUCCESS;
}

static int print_hash_status(word_db_t *word_db)
{
	print_hashstatus((hash_t*)word_db->hash);
	return SUCCESS;
}


/* ###########################################################################
 * frame & configuration stuff here
 * ########################################################################## */

static void get_lexicon_data_path(configValue v)
{
	strncpy(mLexiconDataPath,v.argument[0],MAX_FILE_LEN);
	mLexiconDataPath[MAX_FILE_LEN-1] = '\0';
}

static void get_lexicon_truncation_support(configValue v)
{
	INFO("call this function!");
	if (strncmp("YES", v.argument[0], 3) == 0) {
		m_lexicon_truncation_support = TRUNCATION_YES; 
	} 
	else if (strncmp("NO", v.argument[0], 2) == 0) {
		m_lexicon_truncation_support = TRUNCATION_NO;
	}
	else {
		m_lexicon_truncation_support = TRUNCATION_NO;
		error("config value [%s] must YES/NO", v.argument[0]);
	}
}


static config_t config[] = {
	CONFIG_GET("LexiconData",get_lexicon_data_path,1,\
				"path of lexicon database file. ex) dat/lexicon/lexicon"),
	CONFIG_GET("TRUNCATION", get_lexicon_truncation_support,1,\
				"YES/NO truncation search support"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_get_global_word_db  (get_global_word_db,NULL, NULL, HOOK_MIDDLE);
	sb_hook_open_word_db		(open_word_db,		NULL, NULL, HOOK_MIDDLE);
	sb_hook_sync_word_db		(sync_word_db 	   ,NULL, NULL, HOOK_MIDDLE);
	sb_hook_close_word_db		(close_word_db,		NULL, NULL, HOOK_MIDDLE);

	sb_hook_get_new_word 		(get_new_wordid, 	NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_word 			(get_wordid, 		NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_word_by_wordid 	(get_word_by_wordid,NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_num_of_word		(get_num_of_wordid ,NULL, NULL, HOOK_MIDDLE);

	sb_hook_get_right_truncation_words (get_right_truncation,NULL, NULL, HOOK_MIDDLE);
	sb_hook_get_left_truncation_words (get_left_truncation,NULL, NULL, HOOK_MIDDLE);

	
	sb_hook_print_hash_bucket	(print_hash_bucket ,NULL, NULL, HOOK_MIDDLE);
	sb_hook_print_hash_status	(print_hash_status ,NULL, NULL, HOOK_MIDDLE);
	sb_hook_print_truncation_bucket	(print_truncation_bucket ,NULL, NULL, HOOK_MIDDLE);

//	sb_hook_clear_wordids 		(clean_lexicon	   ,NULL, NULL, HOOK_MIDDLE);
//	sb_hook_destroy_shared_memory(destroy_lexicon  ,NULL, NULL, HOOK_MIDDLE); 
//	sb_hook_print_hash_bucket   (print_hash_bucket ,NULL, NULL, HOOK_MIDDLE);
/*	sb_hook_synchronize_wordids (synchronize_wordids,NULL, NULL, HOOK_MIDDLE);*/
/*	sb_hook_synchronize_wordids (sync_word_db 	   ,NULL, NULL, HOOK_MIDDLE);*/
/*	sb_hook_synchronize_wordids (sync_lexicon_index,NULL, NULL, HOOK_MIDDLE);*/
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
