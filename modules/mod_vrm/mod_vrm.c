/* $Id$ */

#include "mod_vrm.h"
#include <unistd.h>
#include <sys/mman.h>

#ifndef MREMAP_MAYMOVE
	#define MREMAP_MAYMOVE 1 
#endif

#define THREAD_VER4

HOOK_STRUCT(
    HOOK_LINK(vrm_open)
    HOOK_LINK(vrm_close)
    HOOK_LINK(vrm_read)
    HOOK_LINK(vrm_add)
    HOOK_LINK(vrm_status)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrm_open, (char path[], vrm_t **vrm),(path, vrm),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrm_close, (vrm_t *vrm),(vrm),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrm_read, 
				(vrm_t *vrm, int key, void** data, int* size),(vrm, key, data, size),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrm_add, 
				(vrm_t *vrm, int key, void* data, int size),(vrm, key, data, size),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, vrm_status, 
				(vrm_t *vrm),(vrm),DECLINE)

static int page_size = 0; 
static int header_block_size = 0;

static int _lock_init(vrm_t* vrm, char* path);
static int extend_file(mmap_file_t *mmap_file);
static int extend_mmap(mmap_file_t *mmap_file);

static int file_open(char path[], mmap_file_t *mmap_file, int block_size)
{
	int fd, n, ret=SUCCESS;
	header_t header;

    mmap_file->header = &header;
	mmap_file->allocated_blocks = -1;

	fd = sb_open(path, O_RDWR, S_IREAD|S_IWRITE);
	if(fd > 0) {
	    n = read(fd, mmap_file->header, sizeof(header_t));
		if(n != sizeof(header_t)) {
			error("can't read header[%s] : %s, fd[%d]", path, strerror(errno), fd); 
			ret = FAIL;
			goto exit;
		}

		if(strncmp(mmap_file->header->magic, VRM_MAGIC, 4) != 0) {
			error("invalid vrm file[%s]", path); 
			ret = FAIL;
			goto exit;
		}
	} else {
		fd = sb_open(path, O_RDWR|O_CREAT, S_IREAD|S_IWRITE);

		if(fd) {
			strncpy(mmap_file->header->magic, VRM_MAGIC, 4);
			mmap_file->header->next_position = 0;
			mmap_file->header->allocated_blocks = 0;
			mmap_file->header->block_size = block_size; 
			write(fd, mmap_file->header, sizeof(header_t));
		} else {
			error("can't open file[%s] : %s", path, strerror(errno)); 
			ret = FAIL;
			goto exit;
		}
		info("first file open [%s]", path);
	}

	mmap_file->fd = fd;

    mmap_file->header  = mmap(0x00, sizeof(header_t), 
					            PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(mmap_file->header == MAP_FAILED)
	{
	    error("can't mmap file[%s] : %s", path, strerror(errno)); 
		ret = FAIL;
		goto exit;
	}

    if(extend_file(mmap_file) != SUCCESS) {
	    ret = FALSE; 
		goto exit;
	}

    if(extend_mmap(mmap_file) != SUCCESS) {
		ret = FALSE;
		goto exit;
	}

exit:

    return ret;
}

static int vrm_open(char path[], vrm_t **vrm)
{
	char index_path[MAX_PATH_LEN];
	char data_path[MAX_PATH_LEN];
	char lock_path[MAX_PATH_LEN];
	int ret, n;

	sprintf(index_path, "%s%s", path, ".idx");
	sprintf(data_path, "%s%s", path, ".data");
	sprintf(lock_path, "%s%s", path, ".lock");

	if(page_size == 0) page_size = getpagesize();

	header_block_size = page_size;
	n = sizeof(header_t);

	while(n > header_block_size) {
        header_block_size += page_size;
	}

	*vrm = (vrm_t*)sb_malloc(sizeof(vrm_t));
	if(*vrm == 0x00) {
		error("fail vrm sb_malloc![%s]", strerror(errno));
		return FAIL;
	}

    if(_lock_init(*vrm, lock_path) != SUCCESS)
        return FAIL;

	acquire_lock((*vrm)->lock_id);
	ret = file_open(index_path, &(*vrm)->index, MAX_BLOCK_SIZE);
    release_lock((*vrm)->lock_id);

    if(ret == FAIL) { return FAIL; }
	
	acquire_lock((*vrm)->lock_id);
	ret = file_open(data_path, &(*vrm)->data, MAX_BLOCK_SIZE);
    release_lock((*vrm)->lock_id);

    if(ret == FAIL) { return FAIL; }

	return SUCCESS;
}

static int file_close(mmap_file_t *mmap_file)
{
	int fd, i;

	fd = mmap_file->fd;

	for( i=0; i<=mmap_file->allocated_blocks; i++) {
        if(munmap(mmap_file->block[i], mmap_file->header->block_size) == -1) {
            error("fail munmap data[%p] i[%d], size[%d] : %s", 
        	mmap_file->header, i, mmap_file->header->block_size, strerror(errno)); 
            return FAIL;
        }
	}

    if(munmap((void*)mmap_file->header, sizeof(header_t)) == -1) {
	    error("fail munmap data[%p], size[%d] : %s", 
		mmap_file->header, mmap_file->header->block_size, strerror(errno)); 
	    return FAIL;
	}

	close(fd);

	return SUCCESS;
}

static int vrm_close(vrm_t *vrm)
{
	if(vrm == NULL) {
        warn("vrm ptr is NULL");
		return FAIL;
	}

    if(file_close(&vrm->index) == FAIL) { return FAIL; }
    if(file_close(&vrm->data) == FAIL) { return FAIL; }

	sb_free(vrm);

	return SUCCESS;
}

int vrm_status(vrm_t *vrm)
{
	if(vrm == NULL) {
		warn("vrm ptr NULL");
		return FAIL;
	}

#if 1
	info("index next_position : %d",vrm->index.header->next_position);
    info("index global allocated_blocks: %d",vrm->index.header->allocated_blocks);
    info("index local allocated_blocks : %d",vrm->index.allocated_blocks);
    info("index block_size : %d",vrm->index.header->block_size);
    info("index fd : %d",vrm->index.fd);

	info("data next_position : %d",vrm->data.header->next_position);
    info("data global allocated_size : %d",vrm->data.header->allocated_blocks);
    info("data local allocated_size : %d",vrm->data.allocated_blocks);
    info("data block_size : %d",vrm->data.header->block_size);
    info("data fd : %d",vrm->data.fd);
#endif

	return SUCCESS;
}

static int extend_file(mmap_file_t *mmap_file)
{
    int block_size, curr_pos, extend_pos;

	block_size = mmap_file->header->block_size;
	curr_pos = mmap_file->header->allocated_blocks * block_size + header_block_size; //XXX header size
	extend_pos = curr_pos + block_size;

    //info("block_size[%d], curr_pos[%d], extend_pos[%d]", block_size, curr_pos, extend_pos);
    if(lseek(mmap_file->fd, extend_pos, SEEK_SET) != extend_pos) {
	    error("fail lseek offset[%d], fd[%d] : %s", extend_pos, mmap_file->fd, strerror(errno));
	    return FAIL;		
	}

	if(write(mmap_file->fd, "\0", 1) != 1) {
        error("fail write offset[%d] : %s", extend_pos, strerror(errno));
	    return FAIL;		
	}

    return SUCCESS;
}

static int extend_mmap(mmap_file_t *mmap_file)
{
    int curr_pos, block_size, i;
    void* ptr;

	block_size = mmap_file->header->block_size;

    //info("local blocks[%d], global blocks[%d]", mmap_file->allocated_blocks,
    //                        mmap_file->header->allocated_blocks);
	
    for(i=mmap_file->allocated_blocks; i < mmap_file->header->allocated_blocks; i++) {
        mmap_file->allocated_blocks++;
	    curr_pos = mmap_file->allocated_blocks * block_size + header_block_size; //XXX header size
        ptr = mmap(0x00, mmap_file->header->block_size, PROT_READ|PROT_WRITE, MAP_SHARED, 
    					mmap_file->fd, curr_pos);
    	if(ptr == MAP_FAILED)
    	{
    		error("can't mmap : %s, curr_pos[%d]",  strerror(errno), curr_pos); 
    		return FAIL;
    	}

    	mmap_file->block[mmap_file->allocated_blocks] = ptr;
    }

    return SUCCESS;
}

static int allocate(mmap_file_t *mmap_file, int more_extend)
{
	SB_DEBUG_ASSERT(page_size != 0);
    //info("global[%d], local[%d]", mmap_file->header->allocated_blocks, mmap_file->allocated_blocks); 

    if(more_extend) {
		if(mmap_file->header->allocated_blocks >= MAX_BLOCK_NUM) {
            error("allocated block is full max block [%d], curr block[%d]",
					MAX_BLOCK_NUM, mmap_file->header->allocated_blocks);

			return FAIL;
		}

        mmap_file->header->next_position = 0;
        mmap_file->header->allocated_blocks++;
    }
    
    if(mmap_file->header->allocated_blocks > mmap_file->allocated_blocks) {
       if(extend_file(mmap_file) != SUCCESS) return FALSE; 
       if(extend_mmap(mmap_file) != SUCCESS) return FALSE; 
    }   

	return SUCCESS;
}

static int* get_save_index_position(mmap_file_t *mmap_file, int key)
{
    int curr_block, offset;

	curr_block = key*sizeof(int) / mmap_file->header->block_size;
	offset = key*sizeof(int) % mmap_file->header->block_size;

	while(mmap_file->header->allocated_blocks < curr_block) {
		if((mmap_file->header->allocated_blocks - curr_block) > 2) 
			warn("allocate more than 1 block allocated_blocks[%d] curr_block[%d]", 
							mmap_file->header->allocated_blocks, curr_block);

        if(allocate(mmap_file, 1) != SUCCESS) {
            return NULL;
		}
	}
	return (int*)(mmap_file->block[curr_block] + offset);
}

static int* get_save_data_position(mmap_file_t *mmap_file, block_info_t *block_info)
{
    int curr_block, *ptr;

	curr_block = mmap_file->header->allocated_blocks;

    ptr = (int*)(mmap_file->block[curr_block] + mmap_file->header->next_position);

    //info("curr_block[%d], offset[%d], ptr[%p]", curr_block, 
    //                      mmap_file->header->next_position, ptr);

    if(ptr == NULL) {
        error("fail! get data position"); 
        return NULL;
    } else {
        block_info->block_num = curr_block;
        block_info->offset = mmap_file->header->next_position;
        return ptr;
    }
}

static int get_remain_size(mmap_file_t *mmap_file)
{
    return mmap_file->header->block_size - mmap_file->header->next_position;	
}

static int extend_check(mmap_file_t *mmap_file)
{
    if(mmap_file->allocated_blocks < mmap_file->header->allocated_blocks ||
       mmap_file->allocated_blocks == -1) {
        if(allocate(mmap_file, 0) != SUCCESS) {
		    return FAIL;
		}
    }

    return SUCCESS;
}

static int vrm_add(vrm_t *vrm, int key, void* data, int size)
{
	int* ptr, ret;
	block_info_t block_info;

    ret = SUCCESS;

//info("add block[%d], size[%d]", vrm->data.header->block_size, size);

	if(vrm->data.header->block_size < size) {
		error(" data size is too big. block_size[%d], size[%d]",
				vrm->data.header->block_size, size);
		return FAIL;
	}

	acquire_lock(vrm->lock_id);

    if(extend_check(&vrm->index) != SUCCESS) return FAIL;
    if(extend_check(&vrm->data) != SUCCESS) return FAIL;

//info("data remain_size[%d], size[%d]", get_remain_size(&vrm->data), size);
	if(get_remain_size(&vrm->data) < size + sizeof(int)) {
        if(allocate(&vrm->data, 1) != SUCCESS) {
            ret = FAIL;
			goto exit;
		}
	}

	ptr = get_save_data_position(&vrm->data, &block_info);
    if(ptr == NULL) {
        ret = FAIL;
		goto exit;
    }

	//error("size : %d, ptr: %p, key: %d", size, ptr, key);
	
    *ptr = size; ptr++;

    //info("block_num[%d], offset[%d], ptr[%p]", block_info.block_num, block_info.offset, ptr);
    //info("local blocks[%d], global blocks[%d]", vrm->data.allocated_blocks, vrm->data.header->allocated_blocks);

	memcpy((void*)ptr, data, size);
	vrm->data.header->next_position += sizeof(int) + size;

    ptr = get_save_index_position(&vrm->index, key);
    memcpy((void*)ptr, &block_info, sizeof(block_info));	
	vrm->index.header->next_position += sizeof(int);

exit:
    release_lock(vrm->lock_id);
	return ret;
}

static int* get_data(vrm_t *vrm, int key)
{
    int curr_block, offset;
    block_info_t *block_info;

	curr_block = key*sizeof(int) / vrm->index.header->block_size;
	if(curr_block > vrm->index.allocated_blocks) {
        error("not allocated yet key[%d]", key);
		return NULL;
    }

    offset = key*sizeof(int) % vrm->index.header->block_size;
    block_info = (block_info_t*)(vrm->index.block[curr_block] + offset);

//info("block_info : block_num[%u], offset[%u]", block_info->block_num, block_info->offset);

	if(block_info->block_num == 0 && block_info->offset == 0) { 
		return NULL;
	}

    return (int*)(vrm->data.block[block_info->block_num] + block_info->offset);	
}

static int vrm_read(vrm_t *vrm, int key, void** data, int* size)
{
	int* ptr, ret=SUCCESS;

	acquire_lock(vrm->lock_id);

    if(extend_check(&vrm->index) != SUCCESS) {
		ret = FAIL;
		goto exit;
	}

    if(extend_check(&vrm->data) != SUCCESS) {
		ret = FAIL;
		goto exit;
	}

    ptr = get_data(vrm, key);

	if(ptr == NULL) {
        info("can't read data[%d]", key);
		*size = 0;
		*data = NULL;
		ret = FAIL;
		goto exit;
	}

	*size = *ptr;
	*data = (void*)(++ptr);

exit:
    release_lock(vrm->lock_id);
	return ret;
}

static int _lock_init(vrm_t* vrm, char* path)
{
    ipc_t vrm_lock;
    int fd;

	fd = sb_open(path, O_RDWR|O_CREAT, S_IREAD|S_IWRITE);
	if(fd > 0) {
        close(fd);
	} else {
        error("lock file open error [%s]", path);
        return FAIL;
    }

    vrm_lock.type = IPC_TYPE_SEM;
    vrm_lock.pid = SYS5_VRM;
    vrm_lock.pathname = path;

    get_sem(&vrm_lock);
    vrm->lock_id = vrm_lock.id;

    return SUCCESS;
}

static void register_hooks(void)
{
    sb_hook_vrm_open(vrm_open,NULL,NULL,HOOK_MIDDLE);
    sb_hook_vrm_close(vrm_close,NULL,NULL,HOOK_MIDDLE);
    sb_hook_vrm_read(vrm_read,NULL,NULL,HOOK_MIDDLE);
    sb_hook_vrm_add(vrm_add,NULL,NULL,HOOK_MIDDLE);
}

module vrm_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks,		/* register hook api */
};

/** test stuff ***************************************************************/
#define TEST_FILE "dat/test/test_vrm"

static void swap(int* i, int* j) {
    int k;

	k = *i;
	*i = *j;
	*j = k;
}

static void make_ramdom_sequence(int array[], int max_id) {
	int i;

	for(i=0; i < max_id; i++) {
		array[i] = i;
	}

    for (i = 0; i < max_id; i++) 
		swap(&array[i], &array[rand() % (i + 1)]); 
}

static char* make_ramdom_string(int key)
{
	static char sz[1024];
	int j, i;

	if(strlen(sz) == 0) {
		for(j=0; j<1024; )
			for(i=33; i<126 && j<1024; i++, j++)
				sz[j] = i;
	    sz[500] = 0x00;
	}
    
	return sz+(key%300);
}

static int random_read(int max_id, vrm_t *vrm, int start, int end) {
	int *array;
	int i, key, lp, lr, error_count = 0;
	char *p, *r;

	array = (int*)sb_malloc(sizeof(int)*max_id);

    make_ramdom_sequence(array, max_id);
			
	for(i=0; i<max_id; i++) {
		key = array[i];
		p = make_ramdom_string(key);

		lp = strlen(p);

    	if (error_count > 10) return FAIL;

    	if (vrm_read(vrm, key, (void*)&r, &lr) == FAIL) {
			if(key < start || key > end) {
    		    error("%d : vrm_read(%d) failed but it should have succeeded", i, key);
				error_count++;
			}

    		continue;
    	} else if(key >= start && key <= end) {
    		error("%d : vrm_read(%d) succeeded but it should have failed", i, key);
			error_count++;
			continue;
		}
		
		if(lp != lr) {
    		error_count++;
    		error("%d : string length mismatch key[%d], "
				  "len[%u] of data[%p] len[%d] should be %d",
							i, key, lr, r, (int)strlen(r), lp);
		} else 
    		if(memcmp(r, p, lr) != 0) {
    			error_count++;
    			error("%d : memcmp fail %d [%s][%s]", i, lr, p, r);
    		}
	}

	sb_free(array);

	return SUCCESS;
}

static int random_write(int max_id, vrm_t* vrm, int start, int end) {
	int *array;
	int i, key;
	char* p;

	array = (int*)sb_malloc(sizeof(int)*max_id);

    make_ramdom_sequence(array, max_id);

	for(i=0; i<max_id; i++) {
	    key = array[i];

		if(key >= start && key <=end) continue;

		p = make_ramdom_string(key);

		if (vrm_add(vrm, key, p, strlen(p)) == FAIL) {
			error("vrm_add(%d) failed", key);
	        sb_free(array);
			return 1;
		}
	}

	sb_free(array);
	return SUCCESS;
}

static int test_main(slot_t *slot) {
	vrm_t* vrm;  

#if 0
	int key;

	key = 1000000;

	if ( vrm_open(TEST_FILE, &vrm) == FAIL ) {
       		error("vrm_open failed");
       		return 1;
   	}
#if 0
	if (vrm_add(vrm, key, "AAAA", 4) == FAIL) {
		error("vrm_add(%d) failed", key);
		return 1;
	}

	vrm_status(vrm);
	if (vrm_read(vrm, key, (void*)&r, &lr) == FAIL) {
	    error("vrm_read(%d) failed but it should have succeeded", key);
	}

	info("string : %s", r);
#endif
    if (vrm_close(vrm) == FAIL) {
    	error("vrm_open failed");
    	return 1;
    }
#endif
#if 1
	int i, j, pid;

    for(i=0; i < 4; i++) {
    	pid = fork();
    	if(pid > 0) { //parent
			usleep(1000*100);
			continue;
    	} else if(pid == 0) { //child
           	if ( vrm_open(TEST_FILE, &vrm) == FAIL ) {
           		error("vrm_open failed");
           		return 1;
           	}
			vrm_status(vrm);
           
            switch (i) {
			    case 0:
				case 1:
    				srand(5614123+i);

					info("random_write[10000] start(0)");
			    	random_write(10000, vrm, 11, 11);
					info("random_write[10000] end(0)");

				    usleep(1000*1000*1);
#if 1	
					for(j=0; j < 5; j++) {
    					info("random_write[10000] start(%d, %d)", i, j);
    					random_write(10000, vrm, 11, 11);
    					info("random_write[10000] end(%d, %d)", i, j);
					}
#endif
				
					break;
			    case 2:
					usleep(1000*1000);
    				srand(523423);

					for(j=0; j < 5; j++) {
    					info("random_read start(%d)", j);
    					random_read(10000, vrm, 11, 11);
    					info("random_read end(%d)", j);
					}
					break;
			    case 3:
					usleep(1000*1000);
    				srand(151234);

					for(j=0; j < 5; j++) {
    					info("random_read start(%d)", j);
    					random_read(10000, vrm, 11, 11);
    					info("random_read end(%d)", j);
					}
					break;
			}

            if (vrm_close(vrm) == FAIL) {
           		error("vrm_open failed");
           		return 1;
           	}

            return 0; 
    	} else {
    		error("fail create child process!");
    	}
	}
#endif

	return 0;
}

module vrm_test_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	test_main,			/* child_main */
	NULL,				/* scoreboard */
	register_hooks,		/* register hook api */
};

