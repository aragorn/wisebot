/* ****************************************************************************
 * $Id$
 *
 * right-truncation :  prefix 전방절단 검색
 * left-truncation  :  suffix 후방절단 검색
 * 
 * ************************************************************************* */
#include "mod_lexicon.h"
#include "truncation.h"

// functino proto type
static int add_mem_block				(truncation_db_t *truncation_db, int block_idx);
static int  get_bucket_idx				(const char *word, int type);
static truncation_bucket_t *get_bucket 	(truncation_db_t *truncation_db, int bucket_num);
static void get_word 					(word_db_t *word_db, uint32_t wordid, char str[]);
static int find_truncation_insert_slot 	(word_db_t *word_db, 
										truncation_bucket_t *bucket, 
										char *word, int type);
static int put_slot_to_bucket   		(truncation_bucket_t *bucket, int pos, 
										truncation_slot_t slot, int type);
static int attach_ext_bucket			(truncation_db_t *truncation_db, 
										truncation_bucket_t *bucket, int type);
static int put_word(word_db_t *word_db, word_t word, int type);
int print_truncation_bucket(word_db_t *word_db, int bucket_idx);

// alloc - load - save 
static void *alloc_data(char path[], int data_size)
{
	ipc_t mmap;

	INFO("alloc data path[%s] data_size[%d]", path, data_size);
	
    mmap.type        = IPC_TYPE_MMAP;
    mmap.pathname    = path;
    mmap.size        = data_size;

    if (alloc_mmap(&mmap, 0) != SUCCESS) {
        crit("error while allocating mmap for [%s]", path);
        return NULL;
    }
	
	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );
	
    INFO("allocated memory for[%s] mmap.addr[%p]",path, mmap.addr);
	return mmap.addr;
}

static int load_data(char path[], void *data, int data_size)
{
	// mmap쓰면 할 일 없다.
    return SUCCESS;
}

static int save_data(char path[], void *data, int data_size)
{
	if ( sync_mmap( data, data_size ) != SUCCESS ) {
		error( "error while write[%s], %s", path, strerror(errno));
		return FAIL;
	}
    INFO("save to [%s] [%d] bytes", path, data_size);
    return SUCCESS;
}


#define TRUN_KEY_LEN 	(3)

// right-truncation bucket_idx (  0 -  599)
// left-truncation  bucket_idx (600 - 1119)
static int get_bucket_idx(const char *word, int type)
{
	int num=0, len, i, loop;
	int Mod[] = { 5, 12 ,10 };
	int Mul[] = { 120, 10, 1};
	
	len = strlen(word);
	
	loop = len;
	
	if (len > TRUN_KEY_LEN)
		loop = TRUN_KEY_LEN;

	for (i=0; i<loop ; i++) {
		if (type == TYPE_RIGHT_TRUNCATION) {
			num = num + ((((unsigned char)word[i]) % Mod[i] ) * Mul[i]);
		} else if (type == TYPE_LEFT_TRUNCATION) {
			num = num + ((((unsigned char)word[len-i-1]) % Mod[i] ) * Mul[i]);
		}
	}
	
	num = num % MAX_WORD_BUCKET;

	if (type == TYPE_LEFT_TRUNCATION)
		num = num + MAX_WORD_BUCKET;
	
	return num;
}

static int get_hash_loop(char *word) 
{
	int len;

	len = strlen(word);
	if (len < 2) {
		error("|%s| is too short word", word);
		return FAIL;
	} else if (len == 2) { 
		return 10; 
	} else { 
		return 1; 
	}
}

static truncation_bucket_t *get_bucket(truncation_db_t *truncation_db, 
														int bucket_idx)
{
	int block_idx, offset;
	truncation_bucket_block_t *block;
	char data_path[MAX_PATH_LEN];
	
	block_idx = bucket_idx / MAX_BUCKET_NUM_IN_BLOCK;
	offset    = bucket_idx % MAX_BUCKET_NUM_IN_BLOCK;

	if ( block_idx >= truncation_db->shared->block_num) {
		warn(" block_idx [%d] , truncation_db->shared->block_num [%d]", 
				block_idx, truncation_db->shared->block_num);
		return NULL;
	}
	
	block = truncation_db->block[block_idx];
	if (block == NULL) {
		if ( add_mem_block( truncation_db, block_idx ) != SUCCESS ) return NULL;
		block = truncation_db->block[block_idx];
	}

	if (block == NULL) {
		error("cannot alloc data [%s]", data_path);
		return NULL;
	}

	return block->bucket + offset;
}

// get word by wordid (use get_word_by_wordid in mod_lexicon.c)
static void get_word(word_db_t *word_db, uint32_t wordid, char string[])
{
	int ret;
	word_t word;
	
	word.id = wordid;
	ret = get_word_by_wordid( word_db, &word);
	
	//DEBUG("get word : id [%u] string [%s]", wordid, word.string);
	strncpy(string, word.string, MAX_WORD_LENGTH);
	
	return;
}

// reverse strcmp
static int reverse_strcmp( const char *s1, const char *s2 )
{
	int len, len1, len2;
	int i, flag;
	
	len1 = strlen(s1);
	len2 = strlen(s2);

	len = len1 < len2 ? len1 : len2;

	//DEBUG("s1[%s], s2[%s], len1:%d len2:%d len:%d",
	//						s1, s2, len1 ,len2, len);
	
	for (i=1; i<=len; i++) {
		flag = s1[len1-i] - s2[len2-i];
		if (flag != 0) return flag;		
	}
	
	return len1 - len2; 
}

// reverse strncmp
static int reverse_strncmp( const char *s1, const char *s2, int length )
{
	int len, len1, len2;
	int i, flag;
	
	len1 = strlen(s1);
	len2 = strlen(s2);

	len = len1 < len2 ? len1 : len2;
	len = len < length ? len : length;

	//DEBUG("s1[%s], s2[%s], len1:%d len2:%d length:%d len:%d",
	//						s1, s2, len1 ,len2, length, len);
	
	for (i=1; i<=len; i++) {
		flag = s1[len1-i] - s2[len2-i];
		if (flag != 0) return flag;		
	}

	return len - length;
}

static int find_truncation_insert_slot(word_db_t *word_db, 
									   truncation_bucket_t *bucket, 
									   char *word, int type)
{
	int start, middle, finish;
	int flag=0;
	uint32_t middle_wordid;
	char str[MAX_WORD_LENGTH];
	
	start = 0;
	finish = bucket->slot_count - 1;

	if (finish == -1) return 0;
	
	do 
	{
		middle = (start + finish) /2;
		middle_wordid = bucket->slot[middle].wordid;
		get_word( word_db, middle_wordid, str );
		//DEBUG(" middle_wordid [%d] word [%s]", middle_wordid, str);
		
		if (type == TYPE_RIGHT_TRUNCATION) 
			flag = strcmp( str, word );
		else if (type == TYPE_LEFT_TRUNCATION) 
			flag = reverse_strcmp( str, word );	
		else return -1;

		if (flag < 0) {
			start = middle + 1;
		}
		else if (flag > 0) {
			finish = middle -1;
		}
		else {
			error ( "word [%s] already exist", word);
			error ( "str [%s], word [%s]", str, word);
			return -1;	
		}

	} while (start <= finish);

	if ( start<0 || MAX_SLOT_NUM_IN_BUCKET<start ) {
		error("postion illegal word [%s], position [%d]", word, start);
		return -1;
	}

	return start;
}


static int find_truncation_position(word_db_t *word_db, 
									truncation_bucket_t *bucket, 
									char *word, int type, int *startpos)		
{
	int start, middle, finish, flag, hit=0;
	uint32_t middle_wordid;
	char str[MAX_WORD_LENGTH];
	
	start = 0;
	finish = bucket->slot_count -1;
	
	//DEBUG("finish [%d]", finish);
	
	if (finish == -1) return 0;

	do
	{
		middle = (start + finish) /2;
		middle_wordid = bucket->slot[middle].wordid;
		get_word( word_db, middle_wordid, str );
		//DEBUG(" middle_wordid [%u] word [%s]", middle_wordid, str);

		if (type == TYPE_RIGHT_TRUNCATION) 
			flag = strncmp( str, word, strlen(word) );
		else if (type == TYPE_LEFT_TRUNCATION) 
			flag = reverse_strncmp( str, word, strlen(word) );	
		else {
			warn ("type irregal [%d]",type);
			return -1;
		}
		
		if (flag < 0) {
			start = middle + 1;
		}
		else if (flag > 0) {
			finish = middle - 1;
		}
		else {
			finish = middle;
			hit = 1;
			if (start == finish) break;
		}	

		//DEBUG("start[%d], middle[%d], finish[%d], flag[%d], hit[%d]",
		//		start, middle, finish, flag, hit);
	
	}while (start <= finish);

	if (start <0 || MAX_SLOT_NUM_IN_BUCKET < start || hit == 0) {
		return FAIL;
	}
	
	// 절단검색 AB*, *AB 에서 AB 가 나오지 않게 하기 위해서
	middle_wordid = bucket->slot[start].wordid;
	get_word( word_db, middle_wordid, str );
	if ( (strcmp( str, word ) == 0) && (start != MAX_SLOT_NUM_IN_BUCKET-1 ) ) 
		start++;
	
	*startpos = start;	
		
	return SUCCESS;
}


// return find word number
static int find_truncation_word_num(word_db_t *word_db, 
									truncation_bucket_t *bucket, 
									char *word, int type, int *startpos)		
{
	int position, ret, word_num=0, flag;		
	uint32_t wordid;
	char str[MAX_WORD_LENGTH];

	ret = find_truncation_position(word_db, bucket, word, type, startpos);
	if (ret != SUCCESS) return word_num; // word num = 0;
	position= *startpos;
	//DEBUG("start position is [%d] postion [%d]", *startpos, position);
	//DEBUG("bucket->slot_count [%d] ", bucket->slot_count);	
	
	// 단어의 개수를 찾는 과정.
	for (;position < bucket->slot_count;)	
	{
		wordid = bucket->slot[position].wordid;
		get_word( word_db, wordid, str );
		//DEBUG("word:[%s]:  wordid [%u] str [%s]", word, wordid, str);
		
	    if (type == TYPE_RIGHT_TRUNCATION)
	        flag = strncmp( str, word, strlen(word) );
	    else if (type == TYPE_LEFT_TRUNCATION)
	        flag = reverse_strncmp( str, word, strlen(word) );
		else return 0;
		
		//DEBUG("flag is %d", flag);
		
		if (flag != 0) break;
		
		word_num++;
		position++;
	}
	
	//DEBUG(" find_word_num is %d", word_num);
	return word_num;
}

static int put_slot_to_bucket(truncation_bucket_t *bucket, int pos, truncation_slot_t slot, int type)
{
	if (bucket->slot_count == 0) {
		bucket->type = type;
	} 
	else if (bucket->type != type) {
		error("bucekt->type[%d] != type[%d]", bucket->type, type);
		return FAIL;
	}

	memmove(&(bucket->slot[pos+1]), &(bucket->slot[pos]), 
			(bucket->slot_count - pos) * sizeof(truncation_slot_t) );
	
	memcpy(&(bucket->slot[pos]), &slot, sizeof(truncation_slot_t));
	
	bucket->slot_count++;

	return SUCCESS;
}


// add memory block  
static int add_mem_block(truncation_db_t *truncation_db, int block_idx)
{
	char truncation_path[MAX_PATH_LEN];

	if (block_idx == MAX_MEM_BLOCK_NUM) {
		error("block_idx == MAX_MEM_BLOCK_NUM == %d", block_idx);
		return FAIL;
	}
	
	sprintf(truncation_path, "%s.trun_mem_block.%03d", 
			truncation_db->path, block_idx);
	INFO("alloc truncation mem block path [%s]", truncation_path);

	truncation_db->block[block_idx] =  
		alloc_data(truncation_path, sizeof(truncation_bucket_block_t));
	if (truncation_db->block[block_idx] == NULL) {
		error("truncation_db->block[truncation_db->shared->block_num] != NULL");	
		return FAIL;
	}
		
	INFO("truncation_db->block[%d] : [%p] ", block_idx,
			truncation_db->block[block_idx]);
	
	return SUCCESS;
}

// unload memory block
static int remove_mem_block(truncation_db_t *truncation_db, int block_idx)
{
	int ret;
	char truncation_path[MAX_PATH_LEN];

	if ( truncation_db->block[block_idx] == NULL ) return SUCCESS;

	sprintf(truncation_path, "%s.trun_mem_block.%03d", 
			truncation_db->path, block_idx);

	ret = free_mmap( truncation_db->block[block_idx],
						sizeof(truncation_bucket_block_t));
	if ( ret == SUCCESS ) {
		truncation_db->block[block_idx] = NULL;
		INFO("remove truncation mem block path [%s]", truncation_path);
	}
	else error("remove truncation mem block[%s] failed", truncation_path);

	return ret;
}

static int attach_ext_bucket(truncation_db_t *truncation_db, 
							 truncation_bucket_t *bucket, int type)
{
	int ret;

	// check need to alloc new mem block
	if ((truncation_db->shared->ext_bucket_idx % MAX_BUCKET_NUM_IN_BLOCK)==0)
	{
		// alloc new block
		INFO("alloc new memory block[%d] for ext_bucket [%d]",
			  truncation_db->shared->block_num , 
			  truncation_db->shared->ext_bucket_idx);
		ret = add_mem_block(truncation_db, truncation_db->shared->block_num);
		if (ret != SUCCESS) return FAIL;	
		truncation_db->shared->block_num++;	
	}
	
	bucket->ext_bucket_idx = truncation_db->shared->ext_bucket_idx;
	truncation_db->shared->ext_bucket_idx++;

	if (truncation_db->shared->ext_bucket_idx % 100 == 0) {
		INFO("truncation_db->shared->ext_bucket_idx increase[%d]", 
			  truncation_db->shared->ext_bucket_idx);	
	}

	//XXX need extbuckt inititialize? ex) slot_count=0, ext_bucekt_idx=0
	//    now alloc_data have memset(0);;;
	
	return SUCCESS;
}




/*****************************************************************************
 *
 * type : TYPE_RIGHT_TRUNCATION  || TYPE_LEFT_TRUNCATION 
 * ************************************************************************* */
int put_truncation_word(word_db_t *word_db, word_t word)
{
	int ret;

	ret = put_word( word_db, word, TYPE_RIGHT_TRUNCATION);
	if (ret != SUCCESS) return FAIL;
	
	ret = put_word( word_db, word, TYPE_LEFT_TRUNCATION);
	if (ret != SUCCESS) return FAIL;
	
	((truncation_db_t *)word_db->truncation_db)->shared->last_wordid = 
	word.id;	
	
	return SUCCESS;
}


static int put_word(word_db_t *word_db, word_t word, int type)
{
	int bucket_idx, ext_bucket_idx;
	truncation_bucket_t *bucket;
	truncation_slot_t	slot;
	int pos, ret;
	
	
	//DEBUG("word [%s] id [%d] type[%d]", word.string, word.id, type);
	bucket_idx = get_bucket_idx( word.string , type );
	//DEBUG("bucket index is [%d]", bucket_idx);
	
	// find node bucket_idx to insert wordid 
	while (1) 
	{
		bucket = get_bucket(word_db->truncation_db,	bucket_idx);	
		if (bucket == NULL) return FAIL;
		ext_bucket_idx = bucket->ext_bucket_idx;
		if (ext_bucket_idx == 0) break;
		bucket_idx = ext_bucket_idx;
	}
	
	pos = find_truncation_insert_slot(word_db, bucket, word.string ,type); 	
	if (pos <0) return FAIL;

	slot.wordid = word.id;
	ret = put_slot_to_bucket(bucket, pos, slot, type);
	if (ret != SUCCESS) return FAIL;

	// bucket full then attach bucket
	if (bucket->slot_count == MAX_SLOT_NUM_IN_BUCKET) {
		attach_ext_bucket(((truncation_db_t *)word_db->truncation_db), 
														bucket, type);
	}
	
	return SUCCESS;
}

static int truncation_search_one_bucket(word_db_t *word_db, char *word, 
										word_t words[], int words_pos,
										int start_bucket_idx, int type)
{
	truncation_bucket_t *bucket;
	int bucket_idx, ext_bucket_idx;	
	int find_word_num=0, startpos, num;
	int i, len;
	
	bucket_idx = start_bucket_idx;
	len = strlen(word);
	
	
	while(1) 
	{
		bucket = get_bucket(word_db->truncation_db, bucket_idx);
		if (bucket == NULL) return FAIL;
		ext_bucket_idx = bucket->ext_bucket_idx;
				
		num = find_truncation_word_num(word_db, bucket, word, type, &startpos);
			
		for (i=0 ; i<num ; i++) 
		{
			words[words_pos].id = bucket->slot[startpos].wordid;
			words_pos++;
			startpos++;
			find_word_num++;	
			if (words_pos == MAX_SEARCH_WORD_NUM) return find_word_num;
		}

		if (ext_bucket_idx == 0) break;
		else bucket_idx = ext_bucket_idx;
	}
	return find_word_num;
}
	
// fill word->string 
static int fill_word_string_by_wordid(word_db_t *word_db, word_t words[], int find_word_num)
{
	int i, ret;

	for (i=0 ; i< find_word_num; i++) {
		ret = get_word_by_wordid( word_db, &(words[i]));
		if (ret!= SUCCESS) {
			error (" get_word_by_wordid fail: ret[%d]", ret);
			return FAIL;
		}
		//DEBUG("wordid[%u], df[%u], word[%s]",
		//		words[i].id, words[i].word_attr.df, words[i].string);
	}

	return SUCCESS;
}

static int cmp_df(const void *dest, const void *sour){
	    return (int)((word_t *)sour)->word_attr.df - (int)((word_t *)dest)->word_attr.df;
}



// sort by df value
static int sort_result_by_df_value(word_t words[], int find_word_num)
{
	//DEBUG("qsort %d result", find_word_num);
	qsort(words, find_word_num, sizeof(word_t), cmp_df);
	return SUCCESS;
}
	
// 절단 검색 함수
int truncation_search(word_db_t *word_db, char *word, 
						word_t ret_word[], int max_search_num, int type) 
{
	int i, loop, ret, find_word_num=0;
	int base_bucket_idx, bucket_idx;
	word_t find_words[MAX_SEARCH_WORD_NUM];
	
	//DEBUG("truncation search word[%s]", word);
	
	loop = get_hash_loop(word);
	
	base_bucket_idx = get_bucket_idx (word, type);

	//DEBUG("base bucket_idx %d loop %d", base_bucket_idx, loop);
	
	// find truncation iterate loop
	for (i=0 ; i<loop; i++) 
	{
		bucket_idx = base_bucket_idx + i;	
		ret = truncation_search_one_bucket(word_db, word, find_words, 
										   find_word_num, bucket_idx, type);
		find_word_num += ret;
		if (find_word_num == MAX_SEARCH_WORD_NUM) break;
	}

	// fill word->string 
	fill_word_string_by_wordid(word_db, find_words, find_word_num);	

	// sort by df value
	sort_result_by_df_value(find_words, find_word_num);
	
	// find_word_num must less then max_search_num
	find_word_num = find_word_num < max_search_num ? find_word_num : max_search_num; 
	
	// copy value
	memcpy(ret_word, find_words, (find_word_num * sizeof(word_t)));
	
	return find_word_num;	
}

#define print_bucket_size	(10)
static int print_ext_bucket_link(word_db_t *word_db)
{
	truncation_bucket_t *bucket;
	int i, fd, bucket_idx, ext_bucket_idx, buf_size;
	char *buf, temp[print_bucket_size], path[MAX_PATH_LEN];

	buf_size = 
	(((truncation_db_t *)word_db->truncation_db)->shared->ext_bucket_idx) 
	* print_bucket_size;
	
	buf = (char *)sb_malloc(buf_size);
	if (buf == NULL) return FAIL;

	sprintf(path, "%s.ext_bucket_link",
				((truncation_db_t *)word_db->truncation_db)->path);
	
	fd = sb_open(path, FILE_CREAT_FLAG, FILE_CREAT_MODE);

	buf[0]='\0';
	for (i=0; i<MAX_WORD_BUCKET*2 ; i++) {
		bucket_idx = i;
		while(1) 
		{
			bucket = get_bucket(word_db->truncation_db, bucket_idx);
			if (bucket == NULL) {
				close( fd );
				return FAIL;
			}
				
			ext_bucket_idx = bucket->ext_bucket_idx;

			sprintf(temp, "[%4d]->",bucket_idx);
			strcat(buf, temp);
			
			if (ext_bucket_idx == 0) break;
			bucket_idx = ext_bucket_idx;
		}
		//line change
		sprintf(temp, "\n");
		strcat(buf, temp);
	}
	strcat(buf,temp);
		
	write(fd, buf, strlen(buf));
	sb_free(buf);
	close(fd);
	return SUCCESS;
}


int print_truncation_bucket(word_db_t *word_db, int bucket_idx)
{
	truncation_bucket_t *bucket;
	int i;
	uint32_t wordid;
	char str[MAX_WORD_LENGTH];

	if (bucket_idx == -1) 
		return print_ext_bucket_link(word_db);
	
	bucket = get_bucket(word_db->truncation_db, bucket_idx);
	if (bucket == NULL) return FAIL;

	crit("bucket [%d] type [%d] slot_count[%d] ext_bucket_num[%d]", 
		  bucket_idx, bucket->type, bucket->slot_count, bucket->ext_bucket_idx);

	for (i=0; i<bucket->slot_count ; i++){
		wordid = bucket->slot[i].wordid;
		get_word( word_db, wordid, str );
		{
			int len, i;
			len = strlen(str);
			for (i=0; i< len ; i++) {
				fprintf(stderr,"[%d]",str[i]);
			}
			fprintf(stderr," :: %s \n", str);
		}
		INFO(":: (%d) [%s:%u] ", i, str, wordid);
	}
	return SUCCESS;
}


static int alloc_mmap_truncation(truncation_db_t *truncation_db)
{
	ipc_t mmap;
	int i, ret;
	
	INFO("truncation db path [%s]", truncation_db->path);
	
    mmap.type        = IPC_TYPE_MMAP;
    mmap.pathname    = truncation_db->path;
    mmap.size        = sizeof(truncation_shared_t);

	if (alloc_mmap(&mmap, 0) != SUCCESS) {
		crit("error while allocation mmap for lexicon truncation_db");
		return FAIL;
	}
	truncation_db->shared = mmap.addr;
	
	if (mmap.attr == MMAP_CREATED) {
		INFO("truncation_db->shared init");
		// load_truncation_data_block이 제대로 작동하려면 하지 말아야 하지만
		// mmap으로 바꾸면서 넣어도 괜찮은 것으로 판단했다.
		truncation_db->shared->magic            = 1;
		////////////////////////////////////////////////////////////////////

		truncation_db->shared->last_wordid 		= 0;
		truncation_db->shared->ext_bucket_idx 	= MAX_WORD_BUCKET * 2;
		truncation_db->shared->block_num 		= 0;
			
		// mem block size larger then (MAX_WORD_BUCKET * 2 ) 
		ret = add_mem_block(truncation_db, 0);
		if (ret != SUCCESS) return FAIL;
		truncation_db->shared->block_num++;
	}
	else if (mmap.attr == MMAP_ATTACHED) {
		INFO("truncation_db->shared attached");
		for (i=0; i<truncation_db->shared->block_num ; i++) {
			ret = add_mem_block(truncation_db, i);
			if(ret != SUCCESS) return FAIL;
		}
	}
	else {
		error("unknown mmap.attr[%d]", mmap.attr);
		return FAIL;
	}

	DEBUG("truncation_db->shared [%p]", truncation_db->shared);

	INFO("truncation_db->shared->magic			[%d],\n"
		 "truncation_db->shared->last_wordid    [%u],\n"
		 "truncation_db->shared->ext_bucket_idx [%d],\n"
		 "truncation_db->shared->block_num		[%d]",
		  truncation_db->shared->magic, 
		  truncation_db->shared->last_wordid, 
		  truncation_db->shared->ext_bucket_idx,
		  truncation_db->shared->block_num);

	return SUCCESS;
}

static int free_mmap_truncation(truncation_db_t *truncation_db)
{
	int i, ret;

	info("...");

	for (i=0; i<truncation_db->shared->block_num ; i++) {
		remove_mem_block(truncation_db, i);
	}

	ret = free_mmap(truncation_db->shared, sizeof(truncation_shared_t));
	if ( ret == SUCCESS ) truncation_db->shared = NULL;

	info("completed");

	return ret;
}

static int save_mmap_truncation(truncation_db_t *truncation_db)
{
	int ret;

	truncation_db->shared->magic = 1;
	
	INFO("save truncation path [%s]", truncation_db->path);
	
	ret = save_data(truncation_db->path, truncation_db->shared, 
									sizeof(truncation_shared_t));
	if (ret != SUCCESS) return FAIL;
	
	INFO("truncation_db->shared->magic			[%d],\n"
		 "truncation_db->shared->last_wordid    [%u],\n"
		 "truncation_db->shared->ext_bucket_idx [%d],\n"
		 "truncation_db->shared->block_num		[%d]",
		  truncation_db->shared->magic, 
		  truncation_db->shared->last_wordid, 
		  truncation_db->shared->ext_bucket_idx,
		  truncation_db->shared->block_num);

	return SUCCESS;
}

	
static int load_truncation_data_block(truncation_db_t *truncation_db)
{
	int i, ret;
	char data_path[MAX_PATH_LEN];

//  이거 막아도 어차피 load_data에서 하는 일은 없으므로 안심.
//	if (truncation_db->shared->magic == 0) return SUCCESS;
	
	for ( i=0 ; i<(truncation_db->shared->block_num) ; i++ ) 
	{
		sprintf(data_path, "%s.trun_mem_block.%03d", truncation_db->path, i);
		INFO("load data [%s]", data_path);
		ret = load_data(data_path, truncation_db->block[i], 
						sizeof(truncation_bucket_block_t));
		if (ret != SUCCESS) return FAIL;	
		else truncation_db->block[i] = NULL;
	}

	INFO("load [%d] mem block for truncation", i);

	return SUCCESS;
}

static int save_truncation_data_block(truncation_db_t *truncation_db)
{
	int i, ret;
	char data_path[MAX_PATH_LEN];

	for ( i=0 ; i<(truncation_db->shared->block_num) ; i++ ) 
	{
		sprintf(data_path, "%s.trun_mem_block.%03d", truncation_db->path, i);
		INFO("save data [%s]", data_path);
		
		if ( truncation_db->block[i] == NULL ) {
			if ( add_mem_block( truncation_db, i ) != SUCCESS ) return FAIL;
		}
		ret = save_data(data_path, truncation_db->block[i], 
						sizeof(truncation_bucket_block_t));
		if (ret != SUCCESS) return FAIL;	
	}

	INFO("save [%d] mem block for truncation", i);

	return SUCCESS;
}

static int synchronous_word_db_and_truncation_db(word_db_t *word_db)
{
	int ret;
	word_t word;	
	uint32_t i, start, finish;

	start  = ((truncation_db_t *)word_db->truncation_db)->shared->last_wordid;
	finish = word_db->shared->last_wordid;
	
	for (i=start+1 ; i<=finish ; i++) {
		word.id = i;
		get_word( word_db, i, word.string );
		ret = put_truncation_word( word_db, word );
		if (ret != SUCCESS) return FAIL;
	}

	INFO("put_truncation_word [%u] word", finish-start);

	if (word_db->shared->last_wordid != 
		((truncation_db_t *)word_db->truncation_db)->shared->last_wordid) {
		error(" after synchronous word_db->shared->last_wordid [%u] != "
			  "word_db->truncation_db->shared->last_wordid [%u]",
				word_db->shared->last_wordid, 
				((truncation_db_t *)word_db->truncation_db)->shared->last_wordid);
		return FAIL;	
	}
	
	return SUCCESS;
}

static void clear_truncation_db(truncation_db_t *truncation_db)
{
	uint32_t i, ext_bucket_idx;
	truncation_bucket_t *bucket;

	ext_bucket_idx = truncation_db->shared->ext_bucket_idx;
	
	for (i=0; i<ext_bucket_idx; i++) {
		bucket = get_bucket(truncation_db, i);
		if (bucket == NULL) {
			error("bucekt [%d] == NULL", i);	
			return;
		}
		bucket->slot_count=0;
		bucket->ext_bucket_idx=0;
	}
	
	truncation_db->shared->last_wordid = 0;
	truncation_db->shared->ext_bucket_idx = MAX_WORD_BUCKET * 2;
	
	return;
}

static int synchronous_check_word_db_and_truncation_db(word_db_t *word_db)
{
	uint32_t diff;
	int ret;

	INFO("word_db->shared->last_wordid [%u], truncation->shared->last_wordid [%u]",
			word_db->shared->last_wordid, 
			((truncation_db_t *)word_db->truncation_db)->shared->last_wordid);

	diff = word_db->shared->last_wordid - 
		   ((truncation_db_t *)word_db->truncation_db)->shared->last_wordid;

	if (diff == 0) {
		INFO("word_db and truncation db synchronous well");
		return SUCCESS;
	}
	else if (diff >0) {
		CRIT("not error this condition: diff is [%d]",diff);
		ret = synchronous_word_db_and_truncation_db( word_db );		
		if (ret != SUCCESS) return FAIL;
	}
	else if (diff <0) {
		error("word_db and truncation db not synchronous well. "
			  "clear truncation db and rebuild");
		clear_truncation_db(((truncation_db_t *)word_db->truncation_db));
		ret = synchronous_word_db_and_truncation_db( word_db );	
		if (ret != SUCCESS) return FAIL;
	}

	return SUCCESS;
}



int truncation_open( word_db_t *word_db )
{
	int ret;
	truncation_db_t *truncation_db;

	truncation_db = (truncation_db_t *)sb_malloc(sizeof(truncation_db_t));
	memset(truncation_db, 0, sizeof(truncation_db_t));

	// make name
	word_db->truncation_db = truncation_db;
	sprintf(((truncation_db_t *)word_db->truncation_db)->path, 
								"%s.truncation", word_db->path);

	// alloc mmap for truncation_db
	ret = alloc_mmap_truncation( word_db->truncation_db );
	if (ret != SUCCESS) return FAIL;
	DEBUG("word_db->truncation_db->shared [%p]",
			((truncation_db_t *)word_db->truncation_db)->shared);

	ret = load_truncation_data_block( word_db->truncation_db );
	if (ret != SUCCESS) return FAIL;

	ret = synchronous_check_word_db_and_truncation_db( word_db );
	if (ret != SUCCESS) return FAIL;
	
	return SUCCESS;
}

int truncation_sync( word_db_t *word_db )
{
	int ret;
	
	info("saving truncation[%s]...", word_db->path);

	ret = save_mmap_truncation( word_db->truncation_db );
	if (ret != SUCCESS) return FAIL;
	
	ret = save_truncation_data_block( word_db->truncation_db );
	if (ret != SUCCESS) return FAIL;

	info("truncaction[%s] saved", word_db->path);
	return SUCCESS;
}

int truncation_close( word_db_t *word_db )
{
	info("closing truncation[%s]...", word_db->path);

	// lexicon close에서 이미 했다.
//	if ( truncation_sync( word_db ) != SUCCESS ) return FAIL;

	if ( free_mmap_truncation( word_db->truncation_db ) != SUCCESS )
		error( "truncation shared free failed" );

	sb_free(word_db->truncation_db);

	info("truncation[%s] closed", word_db->path);
	return SUCCESS;
}

/* end of truncation.c */
