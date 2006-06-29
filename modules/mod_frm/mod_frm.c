/* $Id$ */

#include "mod_frm.h"
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifndef MREMAP_MAYMOVE
	#define MREMAP_MAYMOVE 1 
#endif

HOOK_STRUCT(
    HOOK_LINK(frm_open)
    HOOK_LINK(frm_close)
    HOOK_LINK(frm_read)
    HOOK_LINK(frm_add)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, frm_open,
	(char path[], mmap_file_t *mmap_file, int data_size),(path, mmap_file, data_size),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, frm_close, (mmap_file_t *mmap_file),(mmap_file),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, frm_read, 
	(mmap_file_t *mmap_file, int key, void** data),(mmap_file, key, data),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, frm_add, 
	(mmap_file_t *mmap_file, int key, void* data),(mmap_file, key, data),DECLINE)

static int page_size = 0; 
static int header_block_size = 0;
static int fixed_data_size = 0;

static int extend_file(mmap_file_t *mmap_file);
static int extend_mmap(mmap_file_t *mmap_file);

static int file_open(char path[], mmap_file_t *mmap_file, int block_size)
{
	int fd, n, ret=SUCCESS;
	header_t header;

    mmap_file->header = &header;
	mmap_file->allocated_blocks = -1;

//EnterCriticalScetion*************************************************
	fd = sb_open(path, O_RDWR, S_IREAD|S_IWRITE);
	if(fd > 0) {
	    n = read(fd, mmap_file->header, sizeof(header_t));
		if(n != sizeof(header_t)) {
			error("can't read header[%s] : %s, fd[%d]", path, strerror(errno), fd); 
			ret = FAIL;
			goto exit;
		}

		if(strncmp(mmap_file->header->magic, FRM_MAGIC, 4) != 0) {
			error("invalid frm file[%s]", path); 
			ret = FAIL;
			goto exit;
		}
	} else {
		fd = sb_open(path, O_RDWR|O_CREAT, S_IREAD|S_IWRITE);

		if(fd) {
			strncpy(mmap_file->header->magic, FRM_MAGIC, 4);
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

exit:
//LeaveCriticalScetion*************************************************
    return ret;
}

static int frm_open(char path[], mmap_file_t *mmap_file, int data_size)
{
	char data_path[MAX_PATH_LEN];
	int ret, n;

	sprintf(data_path, "%s%s", path, ".data");

	if(data_size <= 0 || data_size%2 != 0) {
		error("data size wrong, must 2^ value [%d]", data_size);
		return FAIL;
	}

	fixed_data_size = data_size;

	if(page_size == 0) page_size = getpagesize();

	header_block_size = page_size;
	n = sizeof(header_t);

	while(n > header_block_size) {
        header_block_size += page_size;
	}

	ret = file_open(data_path, mmap_file, MAX_BLOCK_SIZE);
    if(ret == FAIL) { return FAIL; }

    if(extend_file(mmap_file) != SUCCESS) return FALSE; 
    if(extend_mmap(mmap_file) != SUCCESS) return FALSE; 

	return SUCCESS;
}

static int frm_close(mmap_file_t *mmap_file)
{
	int fd, i;

	fd = mmap_file->fd;

	for( i=0; i<=mmap_file->header->allocated_blocks; i++) {
        if(munmap(mmap_file->block[i], mmap_file->header->block_size) == -1) {
            error("fail munmap data[%p], size[%d] : %s", 
        	mmap_file->header, mmap_file->header->block_size, strerror(errno)); 
            return FAIL;
        }
	}

    if(munmap((void*)mmap_file->header, sizeof(header_t)) == -1) {
	    error("fail munmap data[%p], size[%d] : %s", 
		mmap_file->header, mmap_file->header->block_size, strerror(errno)); 
	    return FAIL;
	}

	close(fd);

	memset(mmap_file, 0x00, sizeof(mmap_file_t));

	return SUCCESS;
}

static void frm_status(mmap_file_t *mmap_file)
{
#if 1
    info("global allocated_size : %d", mmap_file->header->allocated_blocks);
    info("local allocated_size : %d", mmap_file->allocated_blocks);
    info("block_size : %d", mmap_file->header->block_size);
    info("fd : %d", mmap_file->fd);
#endif
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

        mmap_file->header->allocated_blocks++;
    }
    
    if(mmap_file->header->allocated_blocks > mmap_file->allocated_blocks) {
       if(extend_file(mmap_file) != SUCCESS) return FALSE; 
       if(extend_mmap(mmap_file) != SUCCESS) return FALSE; 
    }   

	return SUCCESS;
}

static void* get_save_data_position(mmap_file_t *mmap_file, int key)
{
    int curr_block, offset;

	curr_block = key*fixed_data_size / mmap_file->header->block_size;
	offset = key*fixed_data_size % mmap_file->header->block_size;

	while(mmap_file->header->allocated_blocks < curr_block) {
		if((mmap_file->header->allocated_blocks - curr_block) > 2) 
			warn("allocate more than 1 block allocated_blocks[%d] curr_block[%d]", 
							mmap_file->header->allocated_blocks, curr_block);

        if(allocate(mmap_file, 1) != SUCCESS) {
            return NULL;
		}
	}
	return (void*)(mmap_file->block[curr_block] + offset);
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

static int frm_add(mmap_file_t *mmap_file, int key, void* data)
{
	void* ptr;

	if(mmap_file->header->block_size < fixed_data_size) {
		error(" data size is too big. block_size[%d]",
				mmap_file->header->block_size);
		return FAIL;
	}

    if(extend_check((mmap_file_t*)mmap_file) != SUCCESS) return FAIL;

/*EnterCriticalSection**************************************************************/

	ptr = get_save_data_position(mmap_file, key);
    if(ptr == NULL) {
        return FAIL;
    }

	memcpy(ptr, data, fixed_data_size);

/*LeaveCriticalSection**************************************************************/

	return SUCCESS;
}

static void* get_data(mmap_file_t *mmap_file, int key)
{
    int curr_block, offset;

	curr_block = key*fixed_data_size / mmap_file->header->block_size;
	if(curr_block > mmap_file->allocated_blocks) {
        error("not allocated yet key[%d]", key);
		return NULL;
    }

    offset = key*fixed_data_size % mmap_file->header->block_size;
    return (void*)(mmap_file->block[curr_block] + offset);
}

static int frm_read(mmap_file_t *mmap_file, int key, void** data)
{
	int* ptr;

    if(extend_check(mmap_file) != SUCCESS) return FAIL;

    ptr = get_data(mmap_file, key);

	if(ptr == NULL) {
        notice("can't read data[%d]", key);
		*data = NULL;
        return FAIL;
	}

	*data = ptr;

	return SUCCESS;
}

static void register_hooks(void)
{
    sb_hook_frm_open(frm_open,NULL,NULL,HOOK_MIDDLE);
    sb_hook_frm_close(frm_close,NULL,NULL,HOOK_MIDDLE);
    sb_hook_frm_read(frm_read,NULL,NULL,HOOK_MIDDLE);
    sb_hook_frm_add(frm_add,NULL,NULL,HOOK_MIDDLE);
}

module frm_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks,		/* register hook api */
};

/** test stuff ***************************************************************/
#define TEST_FILE "dat/test/test_frm"

#if 0
static void swap(int* i, int* j) {
    int k;

	k = *i;
	*i = *j;
	*j = k;
}

static void make_random_sequence(int array[], int max_id) {
	int i;

	for(i=0; i < max_id; i++) {
		array[i] = i;
	}

    for (i = 0; i < max_id; i++) 
		swap(&array[i], &array[rand() % (i + 1)]); 
}

static char* make_random_string(int key)
{
	static char sz[1024];
	int j, i, d;

//	if(strlen(sz) == 0) {
		for(j=0; j<1024; )
			for(i=33; i<126 && j<1024; i++, j++)
				sz[j] = i;
//	}
    
	d = key%30;
	sz[d+256] = 0x00;
	return sz+(d);
}

static int random_read(int max_id, mmap_file_t* mmap_file, int start, int end) {
	int *array;
	int i, key, lp, lr = 0, error_count = 0;
	char *p, *r;

	array = (int*)sb_malloc(sizeof(int)*max_id);

    make_ramdom_sequence(array, max_id);
			
	for(i=0; i<max_id; i++) {
		key = array[i];
		p = make_ramdom_string(key);

		lp = strlen(p);

    	if (error_count > 10) return FAIL;

    	if (frm_read(mmap_file, key, (void*)&r) == FAIL) {
			if(key < start || key > end) {
    		    error("%d : frm_read(%d) failed but it should have succeeded", i, key);
				error_count++;
			}

    		continue;
    	} else if(key >= start && key <= end) {
    		if(memcmp(r, p, fixed_data_size) != 0) {
    			error_count++;
    			error("%d : memcmp fail %d [%p] frm[%p]", i, lr, p, r);
    		}
			continue;
		}
		
		if(lp != fixed_data_size) {
    		error_count++;
    		error("%d : string length mismatch key[%d], "
				  "len[%d] of data[%s] should be %d : [%s]",
							i, key, lr, r, lp, p);
		} else 
    		if(memcmp(r, p, fixed_data_size) != 0) {
    			error_count++;
    			error("%d : memcmp fail %d [%s]", i, lr, p);
    		}
	}

	sb_free(array);

	return SUCCESS;
}

static int random_write(int max_id, mmap_file_t* mmap_file, int start, int end) {
	int *array;
	int i, key;
	char* p;

	array = (int*)sb_malloc(sizeof(int)*max_id);

    make_ramdom_sequence(array, max_id);

	for(i=0; i<max_id; i++) {
	    key = array[i];

		if(key >= start && key <=end) continue;

		p = make_ramdom_string(key);

		if (frm_add(mmap_file, key, p) == FAIL) {
			error("frm_add(%d) failed", key);
	        sb_free(array);
			return 1;
		}
	}

	sb_free(array);
	return SUCCESS;
}
#endif

static int test_main(slot_t *slot) {
	int i, j, pid;
	mmap_file_t mmap_file;  
	char* r;
    char sz[9048];	

	for(j=0; j<9048; )
		for(i=33; i<126 && j<9048; i++, j++)
			sz[j] = i;
	sz[9048] = '0';
#if 0
	char* r;
	int lr, key;

	key = 100000;

	if ( frm_open(TEST_FILE, &mmap_file, 4) == FAIL ) {
       		error("frm_open failed");
       		return 1;
   	}

	if (frm_add(&mmap_file, key, "AAAA") == FAIL) {
		error("frm_add(%d) failed", key);
		return 1;
	}

	frm_status(&mmap_file);
	if (frm_read(&mmap_file, key, (void*)&r) == FAIL) {
	    error("frm_read(%d) failed but it should have succeeded", key);
	}

	info("string : %s", r);

    if (frm_close(&mmap_file) == FAIL) {
    	error("frm_open failed");
    	return 1;
    }
#endif
#if 1
    for(i=0; i < 3; i++) {
    	pid = fork();
    	if(pid > 0) { //parent
			usleep(1000*100);
			continue;
    	} else if(pid == 0) { //child
           	if ( frm_open(TEST_FILE, &mmap_file, sizeof(sz)) == FAIL ) {
           		error("frm_open failed");
           		return 1;
           	}
			frm_status(&mmap_file);
           
            switch (i) {
			    case 0:
					info("write start");
                	while (frm_add(&mmap_file, 100, sz) == SUCCESS) {
                		;
                	}
					error("out process 1");
#if 0
    				srand(5614123);

					info("random_write[100000] start(0)");
			    	random_write(100000, &mmap_file, 11, 11);
					info("random_write[100000] end(0)");

				    usleep(1000*1000*1);
					
					for(j=0; j < 5; j++) {
					info("random_write[100000] start(1)");
					random_write(100000, &mmap_file, 11, 11);
					info("random_write[100000] end(1)");
					}
#endif
					break;
			    case 1:
					info("read start");
                	while (frm_read(&mmap_file, 100, (void*)&r) == SUCCESS) {
    					if(memcmp(r, sz, fixed_data_size) != 0) {
    						error("memcmp error");
							break;
    					} else {
						}
                	}
					error("out process 2");
#if 0
					usleep(1000*1000*2);
    				srand(523423);

					for(j=0; j < 5; j++) {
					info("random_read start(1)");
					random_read(100000, &mmap_file, 11, 11);
					info("random_read end(1)");
					}
#endif
					break;
			    case 2:
#if 0
					usleep(1000*1000*2);
    				srand(151234);

					for(j=0; j < 5; j++) {
					info("random_read start(2)");
					random_read(100000, &mmap_file, 11, 11);
					info("random_read end(2)");
					}
#endif
					break;
			}

            if (frm_close(&mmap_file) == FAIL) {
           		error("frm_close failed");
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

module frm_test_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	test_main,     		/* child_main */
	NULL,				/* scoreboard */
	register_hooks,		/* register hook api */
};

