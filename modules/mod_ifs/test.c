#include "test.h"
#include "../mod_sfs/mod_sfs.h"
#include "../mod_sfs/shared_memory.h"
#include "mod_ifs_defrag.h"

static char path[]="../../dat/test/ifs";

/******************************************************************************/
char gSoftBotRoot[MAX_PATH_LEN] = SERVER_ROOT;
char gErrorLogFile[MAX_PATH_LEN] = DEFAULT_ERROR_LOG_FILE;
module *static_modules;
/******************************************************************************/

static void __init_test_data(ifs_test_input_t* f);
static void __allocate_test_input(ifs_test_input_t *t);
static void __show_test_input(ifs_test_input_t *t);
static void __make_test_data(char* data, int type);
static void __make_files(test_file_t* f);
static int __lock_init(ifs_local_t* local, char* path);

static void __init_test_data(ifs_test_input_t *t)
{
//	__make_test_data(t->local.data, t->shared->data_type);
//	__make_files(t->local.test_files);
}

static void __show_test_input(ifs_test_input_t *t)
{
	char shuffle[2][20]   = {"NO_SHUFFLE", "SHUFFLE"};
	char data_type[2][20] = {"ASCII_DATA", "BINARY_DATA"};

	info("append_size[%d][%d] read_size[%d][%d]",
		t->local.test.append_size.min, t->local.test.append_size.max,
		t->local.test.read_size.min, t->local.test.read_size.max);
	info("data_type[%s], append_shuffle[%s] read_shuffle[%s]",
		data_type[t->shared->data_type],
		shuffle[t->local.test.append_shuffle],
		shuffle[t->local.test.read_shuffle]);
}

static void __allocate_test_input(ifs_test_input_t *t)
{
	int i;
	int offset = sizeof(ifs_shared_t);
	int size = DATA_SIZE * 2;

	t->local.data = t->p + offset;

	offset += size;
	size = sizeof(test_file_t)*MAX_FILE_ID;

	t->local.test_files = (test_file_t*)(t->p + offset);

	if (t->local.test_order == NULL) {
		t->local.test_order = (int *)malloc(sizeof(int) * MAX_FILE_ID);
		if(t->local.test_order == NULL) {
			error("test_order malloc(%d * %d) failed: %s", sizeof(int), MAX_FILE_ID, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

    for(i = 0; i < MAX_FILE_ID; i++) { 
        t->local.test_order[i] = (rand() % MAX_FILE_ID) + 1;
    }

	if (t->local.buffer == NULL)
		t->local.buffer = (char *)malloc(MAX_TEST_FILE_SIZE);
	if (t->local.buffer == NULL) {
		error("buffer malloc(%d) failed: %s", MAX_TEST_FILE_SIZE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	t->shared->data_type = DATA_TYPE;

	return;
}

static void __make_test_data(char* data, int type)
{
	int i;

	debug("started");
	srand( (unsigned)time( NULL ) );

	for(i = 0; i < DATA_SIZE; i++) {
		if (type == ASCII_DATA)
		  data[i] = (rand() % (0x7E - 0x21)) + 0x21;
		else
		  data[i] = (rand() % 0xFF);
	}
	memcpy(data + DATA_SIZE, data, DATA_SIZE);
}

static void __make_files(test_file_t* f)
{
	int i;

	debug("started");
	for(i = 0; i < MAX_FILE_ID; i++)
	{
		f[i].id = (rand() % MAX_FILE_ID) + 1;
		f[i].size = (rand() % MAX_TEST_FILE_SIZE) + 1;
		f[i].offset = rand() % DATA_SIZE;
		f[i].written_bytes = 0;
	}
}

static int __lock_init(ifs_local_t* local, char* path)
{
    ipc_t lock;

	local->fd = sb_open(path, O_RDWR|O_CREAT, S_IREAD|S_IWRITE);
	if(local->fd == -1) {
		error("can't open file[%s] : %s", path, strerror(errno));
		return FAIL;
	}

    lock.type = IPC_TYPE_SEM;
    lock.pid = SYS5_IFS;
    lock.pathname = path;

    get_sem(&lock);
    local->lock = lock.id;

    return SUCCESS;
}

ifs_test_input_t* test_load(ifs_test* test)
{
	char data_path[MAX_PATH_LEN];
	int shared_size = 0;
	ifs_test_input_t* t = (ifs_test_input_t*)sb_malloc(sizeof(ifs_test_input_t));
	memset(t, 0x00, sizeof(ifs_test_input_t));

	sprintf(data_path, "%s%c%s", path, PATH_SEP, "test_data");
	if(__lock_init(&t->local, data_path) == FAIL) {
		error("can not lock init");
		return NULL;
	}

	debug("data_path[%s], fd[%d]", data_path, t->local.fd);
	shared_size = sizeof(ifs_shared_t) + DATA_SIZE * 2 + sizeof(test_file_t)*MAX_FILE_ID;
	acquire_lock(t->local.lock);
	t->p = (char*)get_shared_memory(1002, t->local.fd, 0, shared_size);
	if(t->p == NULL) {
		release_lock(t->local.lock);
		error("can not get shared memory");
		return NULL;
	}

	t->shared = (ifs_shared_t*)t->p;

	__allocate_test_input(t);
	memcpy(&t->local.test, test, sizeof(ifs_test));
	t->local.max_append_count = MAX_FILE_ID;
    t->shared->append_count = 0;
	test_shuffle_order(t->local.test_order, t->local.test.append_shuffle, MAX_FILE_ID);

	ifs_open(&t->local.indexdb, 0);
	t->local.ifs = (ifs_t*)t->local.indexdb->db;

	table_print(&t->local.ifs->shared->mapping_table);

	if(memcmp(t->shared->magic, MAGIC, sizeof(MAGIC)) == 0) {
		release_lock(t->local.lock);
		info("already init test data");
		return t;
	} else {
		info("init test data");
		strncpy(t->shared->magic, MAGIC, sizeof(MAGIC));
		t->shared->data_type = DATA_TYPE;
	}

	__init_test_data(t);

	release_lock(t->local.lock);

	return t;
}

void test_unload(ifs_test_input_t* t)
{
	int size = 	sizeof(ifs_shared_t) + DATA_SIZE * 2 + sizeof(test_file_t)*MAX_FILE_ID;

	ifs_close(t->local.indexdb);

	unmmap_memory(t->p, size);
}

void test_get_file_info(ifs_test_input_t* t, int i, test_file_t* f)
{
	acquire_lock(t->local.lock);
	memcpy(f, &t->local.test_files[i], sizeof(test_file_t));
	release_lock(t->local.lock);
}

void test_set_file_info(ifs_test_input_t* t, int i, test_file_t* f)
{
	acquire_lock(t->local.lock);
	memcpy(&t->local.test_files[i], f, sizeof(test_file_t));
	release_lock(t->local.lock);
}

void test_update_append_count(ifs_test_input_t* t, int count)
{
	acquire_lock(t->local.lock);
	t->shared->append_count += count;
	release_lock(t->local.lock);
}

void test_shuffle_order(int *list, int opt, int count)
{
    int i, target, tmp;

	debug("started");

	if (opt == NO_SHUFFLE) return;
    
    /*
    for i = 1 to n-1
      h \in_U {i, i+1, ... , n}
      swap( a[i], a[h] )
    end for
     */
    for(i = 0; i < count-1; i++) { 
        target = i + (rand() % (count-1 - i));

		tmp = list[target];
		list[target] = list[i];
		list[i] = tmp;
    }
}
static int read_file(ifs_test_input_t *t, test_file_t *test_file)
{

        int size, bytes;
        int data_offset; /* offset in char *data */
        int file_offset; /* offset in a file */
        int offset        = test_file->offset;
        int written_bytes = test_file->written_bytes;

        if (t->local.test.read_size.max - t->local.test.read_size.min > 0) {
                size = t->local.test.read_size.min + (rand() % (t->local.test.read_size.max - t->local.test.read_size.min));
        } else {
                size = t->local.test.read_size.min;
        }
        file_offset = (written_bytes - size > 0) ? written_bytes - size : 0;
        data_offset = (offset + file_offset) % DATA_SIZE;

		memset( t->local.buffer, '*', MAX_TEST_FILE_SIZE );

        bytes = ifs_read(t->local.indexdb, test_file->id, file_offset, size, t->local.buffer);
        if (bytes != size && bytes != (written_bytes - file_offset))
        {
                error("ifs_read returned %d. file id[%d], read_size[%d]", bytes, test_file->id, size);
                error("written_bytes[%d] - file_offset[%d] = [%d]", written_bytes, file_offset, written_bytes-file_offset);
                
                return FAIL;
        }

        if (memcmp(t->local.buffer, t->local.data + data_offset, bytes) != 0) {
                error("memcmp returned non-zero value. ");
                error("ifs_read returned %d. file id[%d], read_size[%d]", bytes, test_file->id, size);
                error("written_bytes[%d] - file_offset[%d] = [%d]", written_bytes, file_offset, written_bytes-file_offset);

                t->local.buffer[bytes] = '\0';
                error("read    data[%s]", t->local.buffer);
                memcpy(t->local.buffer, t->local.data+data_offset, bytes); t->local.buffer[bytes] = '\0';
                error("written data[%s]", t->local.buffer);
/*
        memset(t->local.buffer, 0x00, DATA_SIZE);
        bytes = ifs_read(t->sfs, test_file->id, 0, MAX_FILE_SIZE, t->local.buffer);
        t->local.buffer[bytes] = '\0';
        error("full file data[%s]", t->local.buffer);
*/
                return FAIL;
        }

        return SUCCESS; 
}

static void __read(ifs_test_input_t* t)
{
	int i = 0;
	int id = 0;
	int ret = 0;
	test_file_t f;

    test_shuffle_order(t->local.test_order, t->local.test.append_shuffle, t->shared->append_count);
    
    for (i = 0; i < t->shared->append_count; i++) {
		info("read testing[%d]...", i);
        id = t->local.test_order[i];
        test_get_file_info(t, id-1, &f);
        ret = read_file(t, &f);
        if (ret == FAIL) {
                crit("read_file(id[%d]) failed.", id);
                break;
        }
    }
}

static int append_file(ifs_test_input_t *t, test_file_t *test_file)
{
        int size, offset, bytes;

        if (t->local.test.append_size.max - t->local.test.append_size.min > 0) {
                size = t->local.test.append_size.min + (rand() % (t->local.test.append_size.max - t->local.test.append_size.min));
        } else {
                size = t->local.test.append_size.min;
        }
        offset = (test_file->offset + test_file->written_bytes) % DATA_SIZE;

        memcpy(t->local.buffer, t->local.data + offset, size);

		bytes = ifs_append(t->local.indexdb, test_file->id, size, t->local.buffer);
        if (bytes != size)
        {
                error("ifs_append returned %d. file id[%d], append size[%d]", bytes, test_file->id, size);
                
                return FAIL;
        }

        test_file->written_bytes += bytes;

        return SUCCESS; 
}

static int __append(ifs_test_input_t* t)
{
    int i;
    int id = 0;
    int ret = 0;
    test_file_t f;
    int total_written_bytes = 0;

    //show_test_input(t);

    for (i = 0; i < t->local.max_append_count; i++) {
//		error("i = %d, max_append_count = %d", i, t->local.max_append_count);
		
			id = t->local.test_order[i];
            test_get_file_info(t, id-1, &f);
            ret = append_file(t, &f);
            if (ret == FAIL) {
                    error("%d'th append_file(id[%d]) failed. this may mean that segment is full.", i, id);
                    error("total written bytes[%d]", total_written_bytes);
                    return FAIL;
            }

            test_set_file_info(t, id-1, &f);
            total_written_bytes += f.written_bytes;
            test_update_append_count(t, 1);
    }

    return SUCCESS;
}

int main(int argc, char* argv[], char* envp[])
{
	int nRet = 0;
	ifs_test test;
	ifs_test_input_t* t = NULL;
	ifs_set_t local_ifs_set[MAX_INDEXDB_SET];

	init_set_proc_title(argc, argv, envp);
	log_setlevelstr("info");

	temp_alive_time = 0;

	// make ifs_set
	memset(local_ifs_set, 0x00, sizeof(local_ifs_set));
	ifs_set = local_ifs_set;
	ifs_set[0].set = 1;
	ifs_set[0].set_ifs_path = 1;
	strncpy( ifs_set[0].ifs_path, path, MAX_PATH_LEN-1 );
	ifs_set[0].set_segment_size = 1;
	ifs_set[0].segment_size = segment_size;
	ifs_set[0].set_block_size = 1;
	ifs_set[0].block_size = block_size;
    
	test.append_shuffle = SHUFFLE;
	test.append_size.min = 1024*1023;
	test.append_size.max = 1024*1023;
	test.read_shuffle = SHUFFLE;
	test.read_size.min = 0;
	test.read_size.max = 100;

	ifs_init();
	t = test_load(&test);
	if ( t == NULL ) return -1;
	crit("append start");
	__append(t);
	crit("append end");
	crit("read start");
	__read(t);
	crit("read end");
	nRet = ifs_defrag(t->local.ifs, NULL);
	if (nRet == FAIL)
		error("defragment fail");

	test_unload(t);

	return SUCCESS;

}
